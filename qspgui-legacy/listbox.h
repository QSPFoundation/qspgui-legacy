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
#include <wx/htmllbox.h>
#include "pathprovider.h"

enum class ListBoxType
{
    Normal,
    Extended
};

class QSPListBox : public wxHtmlListBox
{
public:
    QSPListBox(wxWindow *parent, wxWindowID id, ListBoxType type = ListBoxType::Normal);

    void SetStandardFonts(int size = -1,
                          const wxString& normal_face = wxEmptyString,
                          const wxString& fixed_face = wxEmptyString);
    void RefreshUI();
    void BeginItems();
    void AddItem(const wxString& image, const wxString& desc);
    void EndItems();
    void SetPathProvider(PathProvider *provider) { m_pathProvider = provider; }

    void SetIsHtml(bool isHtml);
    void SetToShowNums(bool toShow);
    void SetTextFont(const wxFont& font);

    [[nodiscard]] wxFont GetTextFont() const { return m_font; }
    void SetLinkColor(const wxColour& clr);
    [[nodiscard]] const wxColour& GetLinkColor() const;

protected:
    [[nodiscard]] wxString OnGetItem(size_t n) const override;
    wxHtmlOpeningStatus OnHTMLOpeningURL(wxHtmlURLType type, const wxString& url, wxString *redirect) const override;

    void CreateHTMLParser() const;

    void OnMouseMove(wxMouseEvent& event);
    void OnMouseClick(wxMouseEvent& event);
    void OnChar(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnMouseWheel(wxMouseEvent& event);

private:
    PathProvider *m_pathProvider{nullptr};

    wxString m_outFormat{};
    wxString m_outFormatNums{};
    wxString m_outFormatImage{};
    wxString m_outFormatImageNums{};

    ListBoxType m_type{ListBoxType::Normal};
    bool m_toUseHtml{false};
    bool m_toShowNums{false};
    wxFont m_font{*wxNORMAL_FONT};

    wxArrayString m_images{};
    wxArrayString m_descs{};
    wxArrayString m_newImages{};
    wxArrayString m_newDescs{};

    DECLARE_CLASS(QSPListBox)
};
