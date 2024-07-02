#pragma once

#include "Windows.h"

namespace sia
{
    namespace window_flag
    {
        namespace show_window
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
            enum : DWORD
            {
                hide            = SW_HIDE,
                normal          = SW_NORMAL,
                minimized       = SW_MINIMIZE,
                maximized       = SW_MAXIMIZE,
                noactivate      = SW_SHOWNOACTIVATE,
                show            = SW_SHOW,
                min_noactivate  = SW_SHOWMINNOACTIVE,
                show_na         = SW_SHOWNA,
                restore         = SW_RESTORE,
                show_default    = SW_SHOWDEFAULT,
                force_minimize  = SW_FORCEMINIMIZE
            };
        } // namespace show_window
        
        namespace window_class_style
        {
            // https://learn.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
            enum : DWORD
            {
                byte_align_client    = CS_BYTEALIGNCLIENT,
                byte_align_window    = CS_BYTEALIGNWINDOW,
                class_dc             = CS_CLASSDC,
                dblclks              = CS_DBLCLKS,
                drop_shadow          = CS_DROPSHADOW,
                global_class         = CS_GLOBALCLASS,
                h_redraw             = CS_HREDRAW,
                v_redraw             = CS_VREDRAW,
                no_close             = CS_NOCLOSE,
                own_dc               = CS_OWNDC,
                parent_dc            = CS_PARENTDC,
                save_bits            = CS_SAVEBITS
            };
        } // namespace window_class_style
        
        namespace window_style
        {
            // https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles
            enum : DWORD
            {
                boarder                 = WS_BORDER,
                caption                 = WS_CAPTION,
                child                   = WS_CHILD,
                child_window            = WS_CHILDWINDOW,
                clip_children           = WS_CLIPCHILDREN,
                clip_sublings           = WS_CLIPSIBLINGS,
                disabled                = WS_DISABLED,
                dlg_frame               = WS_DLGFRAME,
                group                   = WS_GROUP,
                h_scroll                = WS_HSCROLL,
                iconic                  = WS_ICONIC,
                maximize                = WS_MAXIMIZE,
                maximize_box            = WS_MAXIMIZEBOX,
                minimize                = WS_MINIMIZE,
                minimize_box            = WS_MINIMIZEBOX,
                overlapped              = WS_OVERLAPPED,
                overlapped_window       = WS_OVERLAPPEDWINDOW,
                popup                   = WS_POPUP,
                popup_window            = WS_POPUPWINDOW,
                sizebox                 = WS_SIZEBOX,
                sysmenu                 = WS_SYSMENU,
                tabstop                 = WS_TABSTOP,
                thick_frame             = WS_THICKFRAME,
                tiled                   = WS_TILED,
                tiled_window            = WS_TILEDWINDOW,
                visible                 = WS_VISIBLE,
                v_scroll                = WS_VSCROLL
            };
        } // namespace window_style

        namespace window_style_ex
        {
            // https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles
            enum : DWORD
            {
                accept_files            = WS_EX_ACCEPTFILES,
                app_window              = WS_EX_APPWINDOW,
                client_edge             = WS_EX_CLIENTEDGE,
                composited              = WS_EX_COMPOSITED,
                context_help            = WS_EX_CONTEXTHELP,
                control_parent          = WS_EX_CONTROLPARENT,
                dlg_modal_frame         = WS_EX_DLGMODALFRAME,
                layered                 = WS_EX_LAYERED,
                layout_rtl              = WS_EX_LAYOUTRTL,
                left                    = WS_EX_LEFT,
                left_scrollbar          = WS_EX_LEFTSCROLLBAR,
                ltr_reading             = WS_EX_LTRREADING,
                mdi_child               = WS_EX_MDICHILD,
                noactivate              = WS_EX_NOACTIVATE,
                no_ingerit_layout       = WS_EX_NOINHERITLAYOUT,
                no_parent_notify        = WS_EX_NOPARENTNOTIFY,
                no_redirection_bitmap   = WS_EX_NOREDIRECTIONBITMAP,
                overlapped_window_ex    = WS_EX_OVERLAPPEDWINDOW,
                palette_window          = WS_EX_PALETTEWINDOW,
                right                   = WS_EX_RIGHT,
                right_sctollbar         = WS_EX_RIGHTSCROLLBAR,
                rtl_reading             = WS_EX_RTLREADING,
                static_edge             = WS_EX_STATICEDGE,
                tool_window             = WS_EX_TOOLWINDOW,
                top_most                = WS_EX_TOPMOST,
                window_edge             = WS_EX_WINDOWEDGE
            };
        } // namespace window_style_ex
    } // namespace window_flag
} // namespace sia
