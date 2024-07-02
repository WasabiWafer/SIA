#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include "SIA/windows/flag.hpp"

#include "Windows.h"

namespace sia
{
    template <auto Callable>
    LRESULT CALLBACK default_wndproc(HWND handle, UINT msg, WPARAM wparam, LPARAM lpram)
    {
        return Callable(handle, msg, wparam, lparam);
    }

    struct window
    {
        WNDCLASSEXW winclass;
        int param;
        /*
        UINT        cbSize;
        UINT        style;
        WNDPROC     lpfnWndProc;
        int         cbClsExtra;
        int         cbWndExtra;
        HINSTANCE   hInstance;
        HICON       hIcon;
        HCURSOR     hCursor;
        HBRUSH      hbrBackground;
        LPCWSTR     lpszMenuName;
        LPCWSTR     lpszClassName;
        HICON       hIconSm;
        */
        constexpr void InitClass(unsigned int a_style, WNDPROC a_wndproc, HINSTANCE a_hinst, HICON a_hicon, HCURSOR a_hcursor, HBRUSH a_hbrush_bg, LPCWSTR a_menu_name, LPCWSTR a_class_name, HICON a_hicon_sm) noexcept
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
    };
} // namespace sia
