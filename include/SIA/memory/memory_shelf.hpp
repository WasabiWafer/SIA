#pragma once

#include <array>
#include <vector>
#include <bit>
#include <limits>
#include <cmath>

#include "SIA/internals/types.hpp"
#include "SIA/container/tuple.hpp"

//  TO DO
//  add - merge used memory functionality
//  add - make choose allocate policy

// tools
namespace sia
{
    namespace memory_shelf_tools
    {
        // add concept
    } // namespace memory_shelf_tools
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

        constexpr word_t& operator[](const size_t pos) noexcept { return words[pos]; }

        constexpr size_t fittable_size(const size_t byte_size) noexcept
        { return std::ceil(static_cast<double>(byte_size)/static_cast<double>(LetterSize)); }

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

        constexpr void update_page_info() noexcept
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
            { update_refusal(rem_letter); }

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
                    { update_page_info(); }
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
                size_t new_pos = used_iter->pos + req_letter;
                used_iter->pos = new_pos;
                used_iter->able_letter = remain_letter;
            }
            else
            {
                pinfo.used_log.erase(used_iter);
            }
            update_usable(req_letter, remain_letter);
        }

        constexpr void update_refusal(const size_t letter_size) noexcept
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
        constexpr word_t* begin() noexcept { return &(words[0]); }
        constexpr word_t* end() noexcept { return &(words[WordNum]); }

        constexpr tuple<bool, LetterType*> request(const size_t byte_size) noexcept
        {
            tuple<bool, LetterType*> ret{usable(byte_size)};
            if (ret.at<0>() == false)
            { return writable(byte_size); }
            else { return ret; }
        }

        constexpr void refusal(LetterType* ptr, const size_t byte_size) noexcept
        {
            const size_t letter_size = fittable_size(byte_size);
            update_refusal(letter_size);
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

        constexpr void update_book_info() noexcept
        {
            binfo.begin = pages[0].begin();
            binfo.end = pages[PageNum-1].end();
        }

        constexpr page_t& operator[](const size_t pos) noexcept { return pages[pos]; }

    public:
        constexpr bool refusal(LetterType* ptr, const size_t byte_size) noexcept
        {
            if (binfo.begin != pages[0].begin())
            { update_book_info(); }
            if (binfo.begin <= ptr && ptr < binfo.end)
            {
                pages[addr_pos(ptr)].refusal(ptr, byte_size);
                return true;
            }
            return false;
        }

        constexpr LetterType* request(const size_t byte_size) noexcept
        {
            for (auto book_iter {pages.begin()}; book_iter < pages.end(); ++book_iter)
            {
                tuple<bool, LetterType*> result = book_iter->request(byte_size);
                if (result.at<0>() == true)
                { return result.at<1>(); }
                // { return &((*book_iter)[result.at<1>()]); }
            }
            return nullptr;
        }
    };
} // namespace sia


namespace sia
{
    namespace memory_shelf_detail
    {
        
    } // namespace memory_shelf_detail

    template <size_t PageNum, size_t WordNum, typename LetterType = unsigned_interger_t<1>, size_t LetterSize = sizeof(LetterType)>
    struct memory_shelf
    {
    private:
        using book_t = memory_book<PageNum, WordNum, LetterType, LetterSize>;
        std::vector<book_t> books;
        constexpr book_t& operator[](const size_t pos) noexcept { return books[pos]; }

    public:
        template <typename C>
        constexpr C* allocate(const size_t& size = 1) noexcept
        {
            if (size == 0) { return nullptr; }
            if (sizeof(C) * size > (sizeof(LetterType) * WordNum)) { return nullptr; }
            size_t req_byte_size = sizeof(C) * size;
            LetterType* ptr{ };
            for (auto shelf_iter {books.begin()}; shelf_iter < books.end(); ++shelf_iter)
            {
                ptr = shelf_iter->request(req_byte_size);
                return std::bit_cast<C*>(ptr);
            }
            books.emplace_back();
            ptr = books.back().request(req_byte_size);
            return std::bit_cast<C*>(ptr);
        }

        template <typename C>
        constexpr void deallocate(C* ptr, const size_t size = 1) noexcept
        {
            LetterType* conv_ptr = std::bit_cast<LetterType*>(ptr);
            if((ptr == nullptr) || (size == 0)) { return ; }
            for (auto shelf_iter {books.begin()}; shelf_iter < books.end(); ++shelf_iter)
            {
                if (shelf_iter->refusal(conv_ptr, sizeof(C) * size))
                { return; }
            }
        }
    };
} // namespace sia
