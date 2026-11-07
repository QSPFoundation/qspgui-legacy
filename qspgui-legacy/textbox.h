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
#include "htmlview.h"

class QSPTextBox : public wxHtmlWindow, public QSPHtmlView
{
public:
    QSPTextBox(wxWindow *parent, wxWindowID id);

    [[nodiscard]] wxWindow *GetWindow() override { return this; }
    [[nodiscard]] const wxWindow *GetWindow() const override { return this; }

    void RefreshUI() override;
    void LoadBackImage(const wxString& imagePath) override;
    void SetPathProvider(PathProvider *provider) override { m_pathProvider = provider; }

    void SetIsHtml(bool isHtml) override;
    void SetText(const wxString& text, bool toScroll = false) override;
    void SetTextFont(const wxFont& font) override;

    [[nodiscard]] wxFont GetTextFont() const override { return m_font; }
    [[nodiscard]] wxString GetText() const override { return m_text; }

    void SetLinkColor(const wxColour& clr) override;
    [[nodiscard]] const wxColour& GetLinkColor() const override { return m_linkColor; }
    void ScrollToAnchor(const wxString& anchor) override { LoadPage(anchor); }
    void SetBackgroundImage(const wxBitmap& bmpBg);

    void ScrollWindow(int dx, int dy, const wxRect *rect = nullptr) override;

protected:
    void CalcImageSize();

    wxHtmlOpeningStatus OnHTMLOpeningURL(wxHtmlURLType type, const wxString& url, wxString *redirect) const override;

    void OnSize(wxSizeEvent& event);
    void OnEraseBackground(const wxEraseEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnMouseWheel(wxMouseEvent& event);

private:
    PathProvider* m_pathProvider{nullptr};
    bool m_toUseHtml{false};
    wxString m_outFormat{};
    wxString m_imagePath{};
    wxFont m_font{*wxNORMAL_FONT};
    wxColour m_linkColor{0, 0, 0xFF};
    wxString m_text{};
    wxBitmap m_bmpBg{};
    wxBitmap m_bmpRealBg{};
    int m_posX{0};
    int m_posY{0};

    DECLARE_CLASS(QSPTextBox)
};
