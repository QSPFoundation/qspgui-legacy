// Copyright (C) 2001-2025 Val Argunov (byte AT qsp DOT org)
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#pragma once

#include <wx/wx.h>
#include "pathprovider.h"

inline constexpr const char *RENDERER_WXHTML = "wxhtml";
inline constexpr const char *RENDERER_LITEHTML = "litehtml";

class QSPHtmlView
{
public:
    virtual ~QSPHtmlView() = default;

    [[nodiscard]] virtual wxWindow *GetWindow() = 0;
    [[nodiscard]] virtual const wxWindow *GetWindow() const = 0;

    virtual void SetPathProvider(PathProvider *provider) = 0;
    virtual void SetIsHtml(bool isHtml) = 0;
    [[nodiscard]] virtual bool IsHtml() const = 0;
    virtual void SetText(const wxString &text, bool toScroll = false) = 0;
    [[nodiscard]] virtual wxString GetText() const = 0;
    virtual void SetTextFont(const wxFont &font) = 0;
    [[nodiscard]] virtual wxFont GetTextFont() const = 0;
    virtual void SetLinkColor(const wxColour &color) = 0;
    [[nodiscard]] virtual const wxColour &GetLinkColor() const = 0;
    virtual void LoadBackImage(const wxString &imagePath) = 0;
    [[nodiscard]] virtual wxString GetBackImagePath() const = 0;
    virtual void ScrollToAnchor(const wxString &anchor) = 0;
    virtual void RefreshUI() = 0;

    [[nodiscard]] wxColour GetBackColor() const { return GetWindow()->GetBackgroundColour(); }
    [[nodiscard]] wxColour GetFontColor() const { return GetWindow()->GetForegroundColour(); }
};
