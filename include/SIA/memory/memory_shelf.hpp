#pragma once

#include <type_traits>
#include <array>
#include <vector>
#include <bit>
#include <cmath>
#include <algorithm>
#include <tuple>

#include "SIA/internals/types.hpp"
#include "SIA/container/tuple.hpp"

// tools
namespace sia
{
    namespace memory_shelf_tools
    {
        template <typename T>
        concept Allocatable = (!std::is_const_v<T>) && (!std::is_function_v<T>) && (!std::is_reference_v<T>) && (!std::is_void_v<T>);
    } // namespace memory_shelf_tools

    namespace memory_shelf_tag
    {
        enum class policy { seq_reuse, reuse_seq, seq, reuse };
    } // namespace memory_shelf_tag
    
} // namespace sia

// memory_page
namespace sia
{
    namespace memory_page_detail
    {
        struct reuse_info
        {
            size_t pos;
            size_t size;
        };

        struct page_info
        {
            size_t max_size;
            size_t seq_cursor;
            std::vector<reuse_info> reinfo;
        };
    } // namespace memory_page_detail

    template <size_t WordSize, typename Word_t>
    struct memory_page
    {
    private:
        using policy_t = memory_shelf_tag::policy;
        using pinfo_t = memory_page_detail::page_info;

        pinfo_t pinfo;
        std::array<Word_t, WordSize> page;

        constexpr size_t sequence_size() noexcept { return WordSize - pinfo.seq_cursor; }
        constexpr void gather_reuse_info() noexcept
        {
            constexpr auto greater = [] (const memory_page_detail::reuse_info& arg0, const memory_page_detail::reuse_info& arg1) { return arg0.pos < arg1.pos; };
            std::sort(pinfo.reinfo.begin(), pinfo.reinfo.end(), greater);
            for (auto reuse_iter = pinfo.reinfo.begin(); reuse_iter < pinfo.reinfo.end(); ++reuse_iter)
            {
                for (auto inner_iter = ++reuse_iter; inner_iter < pinfo.reinfo.end(); ++inner_iter)
                {
                    bool is_concat_able = ((reuse_iter->pos + reuse_iter->size) == inner_iter->pos);
                    if (is_concat_able)
                    {
                        inner_iter->pos = reuse_iter->pos;
                        inner_iter->size += reuse_iter->size;
                        reuse_iter->size = 0;
                        reuse_iter = inner_iter;
                    }
                }   
            }
            constexpr auto eraser = [] (const memory_page_detail::reuse_info& arg) { return arg.size == 0; };
            std::erase_if(pinfo.reinfo, eraser);
            auto re_writable = [&] (const memory_page_detail::reuse_info& arg) { return (arg.pos + arg.size) == pinfo.seq_cursor; };
            auto rewrite_iter = std::find_if(pinfo.reinfo.begin(), pinfo.reinfo.end(), re_writable);
            if (rewrite_iter != pinfo.reinfo.end())
            {
                pinfo.seq_cursor = rewrite_iter->pos;
                pinfo.reinfo.erase(rewrite_iter);
            }
        }
        constexpr void update_sequence_alloc(size_t size) noexcept { pinfo.seq_cursor += size; }
        constexpr void update_info_max() noexcept
        {
            constexpr auto comp = [] (const memory_page_detail::reuse_info& arg0, const memory_page_detail::reuse_info& arg1) { return arg0.size < arg1.size; };
            auto iter = std::max_element(pinfo.reinfo.begin(), pinfo.reinfo.end(), comp);
            if (iter != pinfo.reinfo.end())
            {
                pinfo.max_size = iter->size;
            }
            else
            {
                pinfo.max_size = 0;
            }
        }
        constexpr void update_reuse_alloc(auto iter, size_t pop_size) noexcept
        {
            bool is_remain = (iter->size > pop_size);
            if (is_remain)
            {
                iter->size -= pop_size;
                iter->pos += pop_size;
            }
            else
            {
                pinfo.reinfo.erase(iter);
            }
        }
        constexpr void update_info_dealloc(Word_t* ptr, size_t size) noexcept
        {
            if (pinfo.max_size < size)
            {
                pinfo.max_size = size;
            }
            pinfo.reinfo.emplace_back(pos(ptr), size);
        }

        constexpr Word_t* allocate_sequence(size_t size) noexcept
        {
            bool is_sequence_able = (size <= sequence_size());
            if (is_sequence_able)
            {
                size_t ret_pos = pinfo.seq_cursor;
                update_sequence_alloc(size);
                return address(ret_pos);
            }
            else { return nullptr; }
        }
        constexpr Word_t* allocate_reuse(size_t size) noexcept
        {
            bool is_reuse_able = (size <= pinfo.max_size) && (pinfo.max_size != 0);
            if (is_reuse_able)
            {
                auto comp = [=] (const memory_page_detail::reuse_info& arg) { return size <= arg.size; };
                auto iter = std::find_if(pinfo.reinfo.begin(), pinfo.reinfo.end(), comp);
                size_t ret_pos = iter->pos;
                update_reuse_alloc(iter, size);
                return address(ret_pos);
            }
            else { return nullptr; }
        }
    public:
        constexpr std::tuple<size_t, size_t> allocatable_info() noexcept { return {pinfo.max_size, sequence_size()}; }
        constexpr bool allocatable(size_t size) noexcept
        {
            if ((pinfo.max_size >= size) || (sequence_size() >= size)) { return true; }
            else { return false; }
        }
        constexpr Word_t* begin() noexcept { return page.data(); }
        constexpr Word_t* end() noexcept { return page.data() + WordSize; }
        constexpr bool is_own(Word_t* ptr) noexcept
        {
            if ((begin() <= ptr) && (ptr < end())) { return true; }
            else { return false; }
        }
        constexpr size_t pos(Word_t* ptr) noexcept { return std::distance(page.data(), ptr); }
        constexpr Word_t* address(size_t pos) noexcept { return page.data() + pos; }
        constexpr Word_t& operator[](size_t pos) noexcept { return page[pos]; }
        constexpr void deallocate(Word_t* ptr, size_t size) noexcept
        {
            update_info_dealloc(ptr, size);
            gather_reuse_info();
            update_info_max();
        }
        constexpr Word_t* allocate(size_t size, policy_t pol = policy_t::reuse_seq) noexcept
        { 
            Word_t* ret{ };
            if (pol == policy_t::reuse)
            {
                ret = allocate_reuse(size);
            }
            else if (pol == policy_t::reuse_seq)
            {
                ret = allocate_reuse(size);
                if (ret == nullptr) { ret = allocate_sequence(size); }
            }
            else if (pol == policy_t::seq)
            {
                ret = allocate_sequence(size);
            }
            else if (pol == policy_t::seq_reuse)
            {
                ret = allocate_sequence(size);
                if (ret == nullptr) { ret = allocate_reuse(size); }
            }
            else { ret = nullptr; }
            return ret;
        }
    };
} // namespace sia

//memory_book
namespace sia
{
    namespace memory_book_detail
    {
        template <typename T>
        struct book_info
        {
            T* begin;
            T* end;
        };
    } // namespace memory_book_detail
    
    template <size_t PageSize, size_t WordSize, typename Word_t>
    struct memory_book
    {
    private:
        using policy_t = memory_shelf_tag::policy;
        using page_t = memory_page<WordSize, Word_t>;
        using binfo_t = memory_book_detail::book_info<Word_t>;

        binfo_t binfo;
        std::array<page_t, PageSize> book;

    public:
        constexpr Word_t* first() noexcept { return book[0].begin(); }
        constexpr Word_t* last() noexcept { return book[PageSize-1].end(); }
        constexpr bool is_own(Word_t* ptr) noexcept
        {
            if (binfo.begin != book[0].begin())
            {
                binfo.begin = first();
                binfo.end = last();
            }
            return (binfo.begin <= ptr) && (ptr < binfo.end);
        }
        constexpr bool allocactable(size_t size) noexcept
        {
            for (page_t& iter : book)
            {
                if (iter.allocatable(size))
                {
                    return true;
                }
            }
            return false;
        }
        constexpr page_t& operator[](size_t pos) noexcept { return book[pos]; }
        constexpr Word_t* allocate(size_t size, policy_t policy) noexcept
        {
            for (page_t& iter : book)
            {
                if (iter.allocatable(size))
                {
                    return iter.allocate(size, policy);
                }
            }
            return nullptr;
        }
        constexpr bool deallocate(Word_t* ptr, size_t size) noexcept
        {
            for (page_t& iter : book)
            {
                if (iter.is_own(ptr))
                {
                    iter.deallocate(ptr, size);
                    return true;
                }
            }
            return false;
        }
    };
} // namespace sia

//memory_shelf
namespace sia
{
    template <typename Default_t, size_t PageSize, size_t WordSize, memory_shelf_tools::Allocatable Word_t = unsigned_interger_t<1>>
    struct memory_shelf
    {
    private:
        using policy_t = memory_shelf_tag::policy;
        using page_t = memory_page<WordSize, Word_t>;
        using book_t = memory_book<PageSize, WordSize, Word_t>;

        std::vector<book_t> shelf;

    public:
        constexpr memory_shelf(size_t assign_size) : shelf(assign_size) { }
        constexpr void expand(size_t size) noexcept
        {
            shelf.reserve(size);
            shelf.assign(size, { });
        }

        template <typename C = Default_t>
        constexpr C* allocate(size_t book_pos, size_t page_pos, size_t size, policy_t policy = policy_t::reuse_seq) noexcept
        {
            bool is_over_size = ((sizeof(Word_t) * WordSize) < (sizeof(C) * size));
            bool is_non_size = (size == 0);
            constexpr auto oversize = [] (size_t target, size_t elem) { return std::ceil(static_cast<double>(target)/static_cast<double>(elem)); };
            if (is_over_size || is_non_size) { return nullptr; }
            size_t oversize_size = oversize(sizeof(C) * size, sizeof(Word_t));
            return std::bit_cast<C*>(shelf[book_pos][page_pos].allocate(oversize_size, policy));
        }

        template <typename C = Default_t>
        constexpr C* allocate(size_t book_pos, size_t size, policy_t policy = policy_t::reuse_seq) noexcept
        {
            bool is_over_size = ((sizeof(Word_t) * WordSize) < (sizeof(C) * size));
            bool is_non_size = (size == 0);
            constexpr auto oversize = [] (size_t target, size_t elem) { return std::ceil(static_cast<double>(target)/static_cast<double>(elem)); };
            if (is_over_size || is_non_size) { return nullptr; }
            size_t oversize_size = oversize(sizeof(C) * size, sizeof(Word_t));
            return std::bit_cast<C*>(shelf[book_pos].allocate(oversize_size, policy));
        }

        template <typename C = Default_t>
        constexpr C* allocate(size_t size = 1, policy_t policy = policy_t::reuse_seq) noexcept
        {
            bool is_over_size = ((sizeof(Word_t) * WordSize) < (sizeof(C) * size));
            bool is_non_size = (size == 0);
            constexpr auto oversize = [] (size_t target, size_t elem) { return std::ceil(static_cast<double>(target)/static_cast<double>(elem)); };
            if (is_over_size || is_non_size) { return nullptr; }
            size_t oversize_size = oversize(sizeof(C) * size, sizeof(Word_t));
            for (book_t& iter : shelf)
            {
                if (iter.allocactable(oversize_size))
                {
                    return std::bit_cast<C*>(iter.allocate(oversize_size, policy));
                }
            }
            return nullptr;
        }

        template <typename C = Default_t>
        constexpr bool deallocate(C* ptr, size_t size = 1) noexcept
        {
            bool is_vaild = (ptr != nullptr) && (size != 0);
            if (!is_vaild) { return false; }
            constexpr auto oversize = [] (size_t target, size_t elem) { return std::ceil(static_cast<double>(target)/static_cast<double>(elem)); };
            size_t oversize_size = oversize(sizeof(C) * size, sizeof(Word_t));
            Word_t* target = std::bit_cast<Word_t*>(ptr);
            for (book_t& iter : shelf)
            {
                if (iter.is_own(target))
                {
                    ptr->~C();
                    return iter.deallocate(target, oversize_size);
                }
            }
            return false;
        }
    };
} // namespace sia

