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
#include <wx/fontmap.h>
#include <wx/html/htmlwin.h>
#include "pathprovider.h"

class QSPTextBox : public wxHtmlWindow
{
public:
    QSPTextBox(wxWindow *parent, wxWindowID id);

    void RefreshUI();
    void LoadBackImage(const wxString& imagePath);
    void SetPathProvider(PathProvider *provider) { m_pathProvider = provider; }

    void SetIsHtml(bool isHtml);
    void SetText(const wxString& text, bool toScroll = false);
    void SetTextFont(const wxFont& font);

    [[nodiscard]] wxFont GetTextFont() const { return m_font; }
    [[nodiscard]] wxString GetText() const { return m_text; }

    void SetLinkColor(const wxColour& clr);
    [[nodiscard]] const wxColour& GetLinkColor() const { return m_Parser->GetLinkColor(); }
    void SetBackgroundImage(const wxBitmap& bmpBg);

protected:
    void CalcImageSize();

    wxHtmlOpeningStatus OnHTMLOpeningURL(wxHtmlURLType type, const wxString& url, wxString *redirect) const override;

    void OnSize(wxSizeEvent& event);
    void OnEraseBackground(const wxEraseEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnMouseClick(wxMouseEvent& event);

private:
    PathProvider* m_pathProvider{nullptr};
    bool m_toUseHtml{false};
    wxString m_outFormat{};
    wxString m_imagePath{};
    wxFont m_font{*wxNORMAL_FONT};
    wxString m_text{};
    wxBitmap m_bmpBg{};
    wxBitmap m_bmpRealBg{};
    int m_posX{0};
    int m_posY{0};

    DECLARE_CLASS(QSPTextBox)
};
