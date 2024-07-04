#pragma once

#include <array>
#include <vector>
#include <bit>
#include <limits>
#include <cmath>

#include "SIA/internals/types.hpp"
#include "SIA/container/tuple.hpp"

namespace sia
{
    template <size_t, size_t, size_t, typename, size_t>
    struct memory_shelf;

    namespace memory_shelf_detail
    {
        using byte_t = unsigned_interger_t<1>;

        template <size_t, size_t, typename, size_t>
        struct memory_book;

        struct used_info
        {
            size_t pos;
            size_t able_letter_num;
        };

        struct page_info
        {
            size_t able_min_size;
            size_t able_max_size;
            size_t able_min_size_count;
            size_t able_max_size_count;
            size_t writable_pos;
            size_t writable_size;
            std::vector<used_info> used_log;
        };

        template <typename T>
        struct book_info
        {
            T* begin;
            T* end;
        };

        // memory_page
        template <size_t WordNum, typename LetterType, size_t LetterSize>
        struct memory_page
        {
            template <size_t, size_t, size_t, typename, size_t>
            friend class memory_shelf;
            template <size_t, size_t, typename, size_t>
            friend class memory_book;

        private:
            using word_t = LetterType;

            page_info pinfo;
            std::array<word_t, WordNum> words;

            constexpr word_t& operator[](const size_t pos) noexcept { return words[pos]; }

            constexpr tuple<bool, size_t> writable(const size_t byte_size) noexcept
            {
                size_t ret{ };
                size_t req_letter_num = fittable_letter_num(byte_size);
                if(pinfo.writable_pos == 0)
                {
                    pinfo.writable_size = WordNum;
                }
                if (req_letter_num > pinfo.writable_size)
                {
                    return {false, ret};
                }
                ret = pinfo.writable_pos;
                pinfo.writable_size -= req_letter_num;
                pinfo.writable_pos += req_letter_num;
                return {true, ret};
            }

            constexpr void update_usable_info_max(const size_t size) noexcept
            {
                if (pinfo.able_max_size < size)
                {
                    pinfo.able_max_size = size;
                    pinfo.able_max_size_count = 1;
                }
                else if (pinfo.able_max_size == size)
                {
                    --pinfo.able_max_size_count;
                    
                    if (pinfo.able_max_size_count == 0)
                    {
                        pinfo.able_max_size = std::numeric_limits<size_t>::max();
                    }
                }
            }

            constexpr void update_usable_info_min(const size_t size) noexcept
            {
                if (pinfo.able_min_size > size)
                {
                    pinfo.able_min_size = size;
                    pinfo.able_min_size_count = 1;
                }
                else if (pinfo.able_min_size == size)
                {
                    --pinfo.able_min_size_count;
                    
                    if (pinfo.able_min_size_count == 0)
                    {
                        pinfo.able_min_size = 0;
                    }
                }
            }

            constexpr void update_usable_info_fixer() noexcept
            {
                bool min_null = (pinfo.able_min_size_count == 0);
                bool max_null = (pinfo.able_max_size_count == 0);
                bool both_null = min_null && max_null;
                if (!both_null)
                {
                    if (min_null)
                    {
                        pinfo.able_min_size = pinfo.able_max_size;
                        pinfo.able_min_size_count = pinfo.able_max_size_count;
                    }
                    else
                    {
                        pinfo.able_max_size = pinfo.able_min_size;
                        pinfo.able_max_size_count = pinfo.able_min_size_count;
                    }
                }
            }

            constexpr void update_usable_info(const size_t req, const size_t rem) noexcept
            {
                bool is_same_infomation = (pinfo.able_min_size == pinfo.able_max_size);
                bool is_max_last = (pinfo.able_max_size_count == 1);
                bool is_min_last = (pinfo.able_max_size_count == 1);
                bool is_need_update = ((pinfo.able_max_size < req) || (pinfo.able_min_size > req));

                if (is_same_infomation)
                {
                    if (is_min_last || is_max_last)
                    {
                        if (is_need_update)
                        {
                            pinfo.able_min_size = req;
                            pinfo.able_max_size = req;
                            pinfo.able_min_size_count = 1;
                            pinfo.able_max_size_count = 1;
                        }
                    }
                    else if (is_need_update)
                    {
                        update_usable_info_min(req);
                        update_usable_info_max(req);
                        update_usable_info_fixer();
                    }
                }
                else
                {
                    update_usable_info_min(req);
                    update_usable_info_max(req);
                    update_usable_info_fixer();
                }

                if (rem != 0)
                {
                    bool is_need_update_rem = ((pinfo.able_max_size < rem) || (pinfo.able_min_size > rem));
                    if (is_same_infomation)
                    {
                        if (is_min_last || is_max_last)
                        {
                            if (is_need_update_rem)
                            {
                                pinfo.able_min_size = rem;
                                pinfo.able_max_size = rem;
                                pinfo.able_min_size_count = 1;
                                pinfo.able_max_size_count = 1;
                            }
                        }
                        else if (is_need_update_rem)
                        {
                            update_usable_info_min(rem);
                            update_usable_info_max(rem);
                            update_usable_info_fixer();
                        }
                    }
                    else
                    {
                        update_usable_info_min(rem);
                        update_usable_info_max(rem);
                        update_usable_info_fixer();
                    }
                }
            }

            constexpr void update_used(auto used_iter, const size_t req_letter_num) noexcept
            {
                const size_t rem_let_num = used_iter->able_letter_num - req_letter_num;
                update_usable_info(req_letter_num, rem_let_num);
                if(rem_let_num != 0)
                {
                    size_t new_pos = used_iter->pos + req_letter_num;
                    used_iter->pos = new_pos;
                    used_iter->able_letter_num = rem_let_num;
                }
                else
                {
                    pinfo.used_log.erase(used_iter);
                }
            }

            constexpr tuple<bool, size_t> usable(const size_t byte_size)
            {
                const size_t need_letter_num = fittable_letter_num(byte_size);
                for (auto used_iter {pinfo.used_log.begin()}; used_iter < pinfo.used_log.end(); ++used_iter)
                {
                    if (used_iter->able_letter_num >= need_letter_num)
                    {
                        size_t ret {used_iter->pos};
                        update_used(used_iter, need_letter_num);
                        return {true, ret};
                    }
                }
                return {false, 0};
            }

            constexpr tuple<bool, size_t> request(const size_t byte_size) noexcept
            {
                tuple<bool, size_t> ret{usable(byte_size)};
                if (ret.at<0>() == false)
                {
                    return writable(byte_size);
                }
                else
                {
                    return ret;
                }
            }
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

            constexpr void update_refusal_info_min(const size_t size) noexcept
            {
                if (pinfo.able_min_size > size)
                {
                    pinfo.able_min_size = size;
                    pinfo.able_min_size_count = 1;
                }
                else if (pinfo.able_min_size == size)
                {
                    ++pinfo.able_min_size_count;
                }
            }

            constexpr void update_refusal_info_max(const size_t size) noexcept
            {
                if (pinfo.able_max_size < size)
                {
                    pinfo.able_max_size = size;
                    pinfo.able_max_size_count = 1;
                }
                else if (pinfo.able_max_size == size)
                {
                    ++pinfo.able_max_size_count;
                }
            }

            constexpr void update_refusal_info(const size_t pos, const size_t size) noexcept
            {
                bool is_same_infomation = (pinfo.able_min_size == pinfo.able_max_size);
                bool is_empty = ((pinfo.able_min_size == 0) && (pinfo.able_max_size == 0));
                
                if (is_same_infomation)
                {
                    if (is_empty)
                    {
                        pinfo.able_min_size = size;
                        pinfo.able_max_size = size;
                        pinfo.able_min_size_count = 1;
                        pinfo.able_max_size_count = 1;
                    }
                    else
                    {
                        update_refusal_info_min(size);
                        update_refusal_info_max(size);
                    }
                }
                else
                {
                    update_refusal_info_min(size);
                    update_refusal_info_max(size);
                }
                pinfo.used_log.emplace_back(pos, size);
            }

            constexpr void refusal(LetterType* ptr, const size_t byte_size) noexcept
            {
                const size_t letter_num = fittable_letter_num(byte_size);
                update_refusal_info(addr_pos(ptr), letter_num);
            }

            constexpr size_t fittable_letter_num(const size_t byte_size) noexcept
            {
                return std::ceil((double)(byte_size)/(double)(LetterSize));
            }
        };

        // memory_book
        template <size_t PageNum, size_t WordNum, typename LetterType, size_t LetterSize>
        struct memory_book
        {
            template <size_t, size_t, size_t, typename, size_t>
            friend class memory_shelf;

        private:
            using page_t = memory_page<WordNum, LetterType, LetterSize>;

            book_info<LetterType> binfo;
            std::array<page_t, PageNum> pages;

            constexpr page_t& operator[](const size_t pos) noexcept { return pages[pos]; }

            constexpr LetterType* request(const size_t byte_size) noexcept
            {
                for (auto book_iter {pages.begin()}; book_iter < pages.end(); ++book_iter)
                {
                    tuple<bool, size_t> result = book_iter->request(byte_size);
                    if (result.at<0>() == true)
                    {
                        return &(book_iter->words[result.at<1>()]);
                    }
                }
                return nullptr;
            }

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

            constexpr void refusal(LetterType* ptr, const size_t byte_size) noexcept
            {
                pages[addr_pos(ptr)].refusal(ptr, byte_size);
            }
        };
    } // namespace memory_shelf_detail

    template <size_t ShelfInitNum, size_t PageNum, size_t WordNum, typename LetterType = memory_shelf_detail::byte_t, size_t LetterSize = sizeof(LetterType)>
    struct memory_shelf
    {
    private:
        using book_t = memory_shelf_detail::memory_book<PageNum, WordNum, LetterType, LetterSize>;

        constexpr book_t& operator[](const size_t pos) noexcept { return books[pos]; }

        std::vector<book_t> books;

    public:
        constexpr memory_shelf() : books {ShelfInitNum} { }

        template <typename C>
        constexpr C* allocate(const size_t& size = 1) noexcept
        {
            if (size == 0) { return nullptr; }
            if (sizeof(C) * size > sizeof(book_t::page_t)) { return nullptr; }
            size_t req_byte_size = sizeof(C) * size;
            for (auto shelf_iter {books.begin()}; shelf_iter < books.end(); ++shelf_iter)
            {
                LetterType* ptr = shelf_iter->request(req_byte_size);
                return std::bit_cast<C*>(ptr);
            }
            return nullptr;
        }

        template <typename C>
        constexpr void deallocate(C* ptr, const size_t size = 1) noexcept
        {
            LetterType* conv_ptr = std::bit_cast<LetterType*>(ptr);
            if((ptr == nullptr) || (size == 0)) { return ; }
            for (auto shelf_iter {books.begin()}; shelf_iter < books.end(); ++shelf_iter)
            {
                if ((shelf_iter->binfo.begin == nullptr) || (shelf_iter->binfo.end == nullptr))
                {
                    shelf_iter->binfo.begin = static_cast<LetterType*>(&(shelf_iter->pages[0][0]));
                    shelf_iter->binfo.end = static_cast<LetterType*>(&(shelf_iter->pages[PageNum-1][WordNum]));
                }
                if (shelf_iter->binfo.begin <= conv_ptr && conv_ptr < shelf_iter->binfo.end)
                {
                    return shelf_iter->refusal(conv_ptr, sizeof(C) * size);
                }
            }
        }
    };
} // namespace sia
