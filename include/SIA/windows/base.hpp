#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include "SIA/windows/flag.hpp"

#include "Windows.h"

namespace sia
{
    namespace window_base_detail
    {
        constexpr LPCWSTR main_window_class_name = L"MAIN_CLASS";

        enum class window_type { console, window };

        struct param
        {
            DWORD ex_style;
            LPCWSTR target_class_name;
            LPCWSTR bar_name;
            DWORD style;
            int x;
            int y;
            int width;
            int height;
            HWND parent_handle;
            HMENU menu_handle;
            HINSTANCE hinst;
            LPVOID lp_param;
        };
    } // namespace base_detail
    
    template <auto Callable>
    LRESULT CALLBACK entry_wndproc(HWND handle, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        return Callable(handle, msg, wparam, lparam);
    }

    struct window
    {
        HWND handle;
        WNDCLASSEXW winclass;
        window_base_detail::param param;

        constexpr void init_class(unsigned int a_style, WNDPROC a_wndproc, HINSTANCE a_hinst, HICON a_hicon, HCURSOR a_hcursor, HBRUSH a_hbrush_bg, LPCWSTR a_menu_name, LPCWSTR a_class_name, HICON a_hicon_sm) noexcept
        {
            winclass.cbSize = sizeof(WNDCLASSEXW);
            winclass.style = a_style;
            winclass.lpfnWndProc = a_wndproc;
            winclass.cbClsExtra = 0;
            winclass.cbWndExtra = 0;
            winclass.hInstance = a_hinst;
            winclass.hIcon = a_hicon;
            winclass.hCursor = a_hcursor;
            winclass.hbrBackground = a_hbrush_bg;
            winclass.lpszMenuName = a_menu_name;
            winclass.lpszClassName = a_class_name;
        }

        constexpr void init_param(DWORD a_ex_style, LPCWSTR a_target_class_name, LPCWSTR a_bar_name, DWORD a_style, int a_x, int a_y, int a_width, int a_height, HINSTANCE a_hinst, LPVOID a_lp_param) noexcept
        {
            param.ex_style = a_ex_style;
            param.target_class_name = a_target_class_name;
            param.bar_name = a_bar_name;
            param.style = a_style;
            param.x = a_x;
            param.y = a_y;
            param.width = a_width;
            param.height = a_height;
            param.hinst = a_hinst;
            param.lp_param = a_lp_param;
        }        

        auto regist() { return RegisterClassExW(&winclass); }
        auto create() { handle = CreateWindowExW(param.ex_style, param.target_class_name, param.bar_name, param.style, param.x, param.y, param.width, param.height, NULL, param.menu_handle, param.hinst, param.lp_param); }
        auto console_handle() { return GetConsoleWindow(); }
        auto console_instance(HWND h) { return reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(console_handle(), GWLP_HINSTANCE)); }
        auto module_handle(LPCWSTR name, HMODULE ret) { return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, name, &ret); }
        void show(DWORD flag) { ShowWindow(handle, flag); }
    };
} // namespace sia
