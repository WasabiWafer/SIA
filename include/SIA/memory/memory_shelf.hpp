#pragma once

#include <type_traits>
#include <array>
#include <vector>
#include <bit>
#include <limits>
#include <cmath>
#include <algorithm>

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

    namespace memory_shelf_policy
    {
        enum class policy { none, rich, poor, thrifty };
    } // namespace memory_shelf_policy
    
} // namespace sia

// memory_page
namespace sia
{
    namespace memory_page_detail
    {
        struct used_info
        {
            size_t pos;
            size_t able_letter;
        };

        struct page_info
        {
            size_t able_min_letter;
            size_t able_max_letter;
            size_t able_min_count;
            size_t able_max_count;
            size_t writable_pos;
            size_t writable_letter;
            std::vector<used_info> used_log;
        };
    } // namespace memory_page_detail
    
    template <size_t WordNum, typename LetterType, size_t LetterSize>
    struct memory_page
    {
    private:
        using word_t = LetterType;
        memory_page_detail::page_info pinfo;
        std::array<word_t, WordNum> words;

        constexpr size_t fittable_size(const size_t byte_size) noexcept
        { return static_cast<size_t>(std::ceil(static_cast<double>(byte_size)/static_cast<double>(LetterSize))); }

        constexpr void recover() noexcept
        {
            using used_t = memory_page_detail::used_info;
            auto cond = [=] (const used_t& arg) { return ((arg.pos + arg.able_letter) == pinfo.writable_pos); };
            const auto iter = std::find_if(pinfo.used_log.begin(), pinfo.used_log.end(), cond);
            if (!(iter == pinfo.used_log.end()))
            {
                pinfo.writable_pos = iter->pos;
                pinfo.writable_letter += iter->able_letter;
                pinfo.used_log.erase(iter);
            }
        }

        constexpr void update_page() noexcept
        {
            size_t min {std::numeric_limits<size_t>::max()};
            size_t min_count { };
            size_t max { };
            size_t max_count { };
            for (auto& elem : pinfo.used_log)
            {
                if (elem.able_letter < min)
                {
                    min = elem.able_letter;
                    min_count = 0;
                }
                if (elem.able_letter < max)
                {
                    max = elem.able_letter;
                    max_count = 0;
                }
                if (elem.able_letter == min)
                { ++min_count; }
                if (elem.able_letter == max)
                { ++max_count; }
            }
            pinfo.able_min_letter = ((min != std::numeric_limits<size_t>::max()) ? min : 0);
            pinfo.able_max_letter = max;
            pinfo.able_min_count = min_count;
            pinfo.able_max_count = max_count;
        }

        constexpr void update_usable(const size_t req_letter, const size_t rem_letter) noexcept
        {
            if (rem_letter != 0)
            { update_release(rem_letter); }

            bool is_same_point = (pinfo.able_min_letter == pinfo.able_max_letter);
            bool hit_min = (pinfo.able_min_letter == (req_letter + rem_letter));
            bool hit_max = (pinfo.able_max_letter == (req_letter + rem_letter));
            bool is_min_last = (pinfo.able_min_count == 1); 
            bool is_max_last = (pinfo.able_max_count == 1);

            if (is_same_point)
            {
                if (hit_min || hit_max)
                {
                    if (is_min_last || is_max_last)
                    {
                        pinfo.able_min_letter = 0;
                        pinfo.able_max_letter = 0;
                        pinfo.able_min_count = 0;
                        pinfo.able_max_count = 0;
                    }
                    else
                    {
                        --pinfo.able_min_count;
                        --pinfo.able_max_count;
                    }
                }
            }
            else
            {
                if (is_min_last || is_max_last)
                {
                    if (hit_min || hit_max)
                    { update_page(); }
                }
                else
                {
                    if (hit_min)
                    { --pinfo.able_min_count; }
                    else if (hit_max)
                    { --pinfo.able_max_count; }
                }
            }
        }

        constexpr void update_used(auto used_iter, const size_t req_letter) noexcept
        {
            const size_t remain_letter = used_iter->able_letter - req_letter;
            if(remain_letter != 0)
            {
                used_iter->pos = used_iter->pos + req_letter;
                used_iter->able_letter = remain_letter;
            }
            else
            { pinfo.used_log.erase(used_iter); }
            update_usable(req_letter, remain_letter);
        }

        constexpr void update_release(const size_t letter_size) noexcept
        {
            bool is_empty = ((pinfo.able_min_letter == 0) && (pinfo.able_max_letter == 0));
            bool is_same_point = (pinfo.able_min_letter == pinfo.able_max_letter);
            bool need_max_update = (letter_size > pinfo.able_max_letter);
            bool need_min_update = (letter_size < pinfo.able_min_letter);
            bool hit_max = (letter_size == pinfo.able_max_letter);
            bool hit_min = (letter_size == pinfo.able_max_letter);

            if (is_empty)
            {
                pinfo.able_min_letter = letter_size;    
                pinfo.able_max_letter = letter_size;
                pinfo.able_min_count = 1;
                pinfo.able_max_count = 1;
            }
            else if (is_same_point)
            {
                if (hit_min || hit_max)
                {
                    ++pinfo.able_min_count;
                    ++pinfo.able_max_count;
                }
                else if (need_min_update)
                {
                    pinfo.able_min_letter = letter_size;
                    pinfo.able_min_count = 1;
                }
                else if (need_max_update)
                {
                    pinfo.able_max_letter = letter_size;
                    pinfo.able_max_count = 1;
                }
            }
            else
            {
                if (hit_min)
                {
                    ++pinfo.able_min_count;
                }
                else if (need_min_update)
                {
                    pinfo.able_min_letter = letter_size;
                    pinfo.able_min_count = 1;
                }
                else if (hit_max)
                {
                    ++pinfo.able_max_count;
                }
                else if (need_max_update)
                {
                    pinfo.able_max_letter = letter_size;
                    pinfo.able_max_count = 1;
                }
            }
        }

        constexpr tuple<bool, LetterType*> writable(const size_t byte_size) noexcept
        {
            if(pinfo.writable_pos == 0) { pinfo.writable_letter = WordNum; }
            size_t pos{ };
            const size_t req_letter_num = fittable_size(byte_size);
            if (pinfo.writable_letter < req_letter_num) { return {false, nullptr}; }
            else
            {
                pos = pinfo.writable_pos;
                pinfo.writable_pos += req_letter_num;
                pinfo.writable_letter -= req_letter_num;
                return {true, &(words[pos])};
            }
        }

        constexpr tuple<bool, LetterType*> usable(const size_t byte_size)
        {
            const size_t req_letter = fittable_size(byte_size);
            for (auto used_iter {pinfo.used_log.begin()}; used_iter < pinfo.used_log.end(); ++used_iter)
            {
                if (used_iter->able_letter >= req_letter)
                {
                    size_t pos {used_iter->pos};
                    update_used(used_iter, req_letter);
                    return {true, &(words[pos])};
                }
            }
            return {false, nullptr};
        }

    public:
        constexpr word_t& operator[](const size_t pos) noexcept { return words[pos]; }
        constexpr word_t* begin() noexcept { return &(words[0]); }
        constexpr word_t* end() noexcept { return &(words[WordNum]); }

        constexpr size_t addr_pos(LetterType* ptr) noexcept
        {
            constexpr auto minus = [] (const size_t arg0, const size_t arg1) { return (arg0 >= arg1) ? arg0 - arg1 : arg1 - arg0; };
            const size_t conv_begin = reinterpret_cast<size_t>(&(words[0]));
            const size_t conv_ptr = reinterpret_cast<size_t>(ptr);
            const size_t ret = minus(conv_begin, conv_ptr);
            if (ret == 0)
            { return ret; }
            else
            { return ret/LetterSize; }
        }

        constexpr void restore() noexcept
        {
            using used_t = memory_page_detail::used_info;
            constexpr auto comp_g = [] (const used_t& arg0, const used_t& arg1) { return arg0.pos < arg1.pos; };
            std::sort(pinfo.used_log.begin(), pinfo.used_log.end(), comp_g);
            for (auto top_iter{pinfo.used_log.begin()}; top_iter < pinfo.used_log.end(); ++top_iter)
            {
                for (auto cat_iter{top_iter + 1}; cat_iter < pinfo.used_log.end(); ++cat_iter)
                {
                    bool catable = ((top_iter->pos + top_iter->able_letter) == cat_iter->pos);
                    if (catable)
                    {
                        cat_iter->pos = top_iter->pos;
                        cat_iter->able_letter += top_iter->able_letter;
                        top_iter->able_letter = 0;
                        top_iter = cat_iter;
                    }
                    else
                    { break; }
                }
            }
            constexpr auto cond_del = [] (const used_t& arg) { return arg.able_letter == 0; };
            std::erase_if(pinfo.used_log, cond_del);
            recover();
            update_page();
        }

        constexpr tuple<bool, LetterType*> request(const size_t byte_size, const memory_shelf_policy::policy policy = memory_shelf_policy::policy::none) noexcept
        {
            if (policy == memory_shelf_policy::policy::none)
            {
                tuple<bool, LetterType*> ret = usable(byte_size);
                if (ret.at<0>() == false) { return writable(byte_size); }
                else { return ret; }
            }
            else if (policy == memory_shelf_policy::policy::rich)
            {
                return writable(byte_size);
            }
            else if (policy == memory_shelf_policy::policy::poor)
            {
                return usable(byte_size);
            }
            else if (policy == memory_shelf_policy::policy::thrifty)
            {
                restore();
                return usable(byte_size);
                
            }
            else { return {false, nullptr}; }
        }

        constexpr void release(LetterType* ptr, const size_t byte_size) noexcept
        {
            const size_t letter_size = fittable_size(byte_size);
            update_release(letter_size);
            pinfo.used_log.emplace_back(addr_pos(ptr), letter_size);
        }
    };
} // namespace sia

// memory book
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
    
    template <size_t PageNum, size_t WordNum, typename LetterType, size_t LetterSize>
    struct memory_book
    {
    private:
        using page_t = memory_page<WordNum, LetterType, LetterSize>;
        memory_book_detail::book_info<LetterType> binfo;
        std::array<page_t, PageNum> pages;

        constexpr void update_book_info() noexcept
        {
            binfo.begin = pages[0].begin();
            binfo.end = pages[PageNum-1].end();
        }

    public:
        constexpr page_t& operator[](const size_t pos) noexcept { return pages[pos]; }
        constexpr bool owned(LetterType* ptr) noexcept { return ((binfo.begin <= ptr) && (ptr < binfo.end)); }

        constexpr size_t addr_pos(LetterType* ptr) noexcept
        {
            constexpr size_t page_size = sizeof(page_t);
            constexpr auto minus = [] (const size_t arg0, const size_t arg1) { return (arg0 >= arg1) ? arg0 - arg1 : arg1 - arg0; };
            const size_t conv_begin = reinterpret_cast<size_t>(binfo.begin);
            const size_t conv_ptr = reinterpret_cast<size_t>(ptr);      
            const size_t ret = minus(conv_begin, conv_ptr);
            if (ret == 0)
            { return ret; }
            else
            { return ret/page_size; }
        }

        constexpr bool release(LetterType* ptr, const size_t byte_size) noexcept
        {
            if (binfo.begin != pages[0].begin())
            { update_book_info(); }
            if (owned(ptr))
            {
                pages[addr_pos(ptr)].release(ptr, byte_size);
                return true;
            }
            return false;
        }

        constexpr LetterType* request(const size_t pos0, const size_t byte_size, const memory_shelf_policy::policy policy = memory_shelf_policy::policy::none) noexcept
        {
            tuple<bool, LetterType*> result = pages[pos0].request(byte_size, policy);
            if (result.at<0>() == true)
            { return result.at<1>(); }
            return nullptr;
        }

        constexpr LetterType* request(const size_t byte_size, const memory_shelf_policy::policy policy = memory_shelf_policy::policy::none) noexcept
        {
            for (auto book_iter {pages.begin()}; book_iter < pages.end(); ++book_iter)
            {
                tuple<bool, LetterType*> result = book_iter->request(byte_size, policy);
                if (result.at<0>() == true)
                { return result.at<1>(); }
            }
            return nullptr;
        }

        constexpr void restore(const size_t pos) noexcept { pages[pos].restore(); }
        constexpr void restore() noexcept
        {
            for (auto& elem : pages)
            { elem.restore(); }
        }
    };
} // namespace sia

// memory_shelf
namespace sia
{    
    template <size_t PageNum, size_t WordNum, memory_shelf_tools::Allocatable LetterType = unsigned_interger_t<1>, size_t LetterSize = sizeof(LetterType)>
    struct memory_shelf
    {
    private:
        using book_t = memory_book<PageNum, WordNum, LetterType, LetterSize>;
        std::vector<book_t> books;
        constexpr book_t& operator[](const size_t pos) noexcept { return books[pos]; }

    public:
        constexpr size_t word_size() noexcept { return LetterSize; }
        constexpr size_t page_size() noexcept { return word_size() * WordNum; }
        constexpr size_t book_size() noexcept { return page_size() * PageNum; }
        constexpr size_t shelf_size() noexcept { return book_size() * books.capacity(); }
        constexpr size_t capacity() noexcept { return books.capacity(); }
        constexpr void assign(const size_t size) { books.assign(size, { }); }
        template <typename C>
        constexpr tuple<bool, size_t, size_t, size_t> addr_pos(C* ptr) noexcept
        {
            LetterType* conv_ptr = std::bit_cast<LetterType*>(ptr);
            for (size_t pos0{ }; pos0 < capacity(); ++pos0)
            {
                if (books[pos0].owned(conv_ptr))
                {
                    size_t pos1 = books[pos0].addr_pos(conv_ptr);
                    size_t pos2 = books[pos0][pos1].addr_pos(conv_ptr);
                    return {true, pos0, pos1, pos2};
                }
            }
            return {false, 0, 0, 0};
        }

        template <typename C>
        [[nodiscard]] constexpr C* allocate(const size_t pos0, const size_t pos1, const size_t& size, const memory_shelf_policy::policy policy = memory_shelf_policy::policy::none) noexcept
        {
            if (size == 0) { return nullptr; }
            if (sizeof(C) * size > (sizeof(LetterType) * WordNum)) { return nullptr; }
            size_t req_byte_size = sizeof(C) * size;
            LetterType* ptr{ };
            ptr = books[pos0].request(pos1, req_byte_size);
            if (ptr == nullptr) { return nullptr; }
            return std::bit_cast<C*>(ptr);
        }

        template <typename C>
        [[nodiscard]] constexpr C* allocate(const size_t pos0, const size_t& size, const memory_shelf_policy::policy policy = memory_shelf_policy::policy::none) noexcept
        {
            if (size == 0) { return nullptr; }
            if (sizeof(C) * size > (sizeof(LetterType) * WordNum)) { return nullptr; }
            size_t req_byte_size = sizeof(C) * size;
            LetterType* ptr{ };
            ptr = books[pos0].request(req_byte_size);
            if (ptr == nullptr) { return nullptr; }
            return std::bit_cast<C*>(ptr);
        }

        template <typename C>
        [[nodiscard]] constexpr C* allocate(const size_t& size = 1, const memory_shelf_policy::policy policy = memory_shelf_policy::policy::none) noexcept
        {
            if (size == 0) { return nullptr; }
            if (sizeof(C) * size > (sizeof(LetterType) * WordNum)) { return nullptr; }
            size_t req_byte_size = sizeof(C) * size;
            LetterType* ptr{ };
            for (auto shelf_iter {books.begin()}; shelf_iter < books.end(); ++shelf_iter)
            {
                ptr = shelf_iter->request(req_byte_size, policy);
                return std::bit_cast<C*>(ptr);
            }
            return nullptr;
        }

        template <typename C>
        constexpr bool deallocate(C* ptr, const size_t size = 1) noexcept
        {
            LetterType* conv_ptr = std::bit_cast<LetterType*>(ptr);
            if((ptr == nullptr) || (size == 0)) { return false; }
            for (auto shelf_iter {books.begin()}; shelf_iter < books.end(); ++shelf_iter)
            {
                if (shelf_iter->release(conv_ptr, sizeof(C) * size))
                { return true; }
            }
            return false;
        }

        constexpr void restore(const size_t pos0, const size_t pos1) noexcept { books[pos0].restore(pos1); }
        constexpr void restore(const size_t pos0) noexcept { books[pos0].restore(); }
        constexpr void restore() noexcept
        {
            for (auto& elem : books)
            { elem.restore(); }
        }
    };
} // namespace sia
