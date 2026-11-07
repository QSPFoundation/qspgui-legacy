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
#include <wx/htmllbox.h>
#include "listview.h"

class QSPListBox : public QSPListBoxBase<wxHtmlListBox>
{
public:
    QSPListBox(wxWindow *parent, wxWindowID id, ListBoxType type = ListBoxType::Normal);

    void SetStandardFonts(int size = -1,
                          const wxString& normal_face = wxEmptyString,
                          const wxString& fixed_face = wxEmptyString);

protected:
    [[nodiscard]] wxString OnGetItem(size_t n) const override;
    wxHtmlOpeningStatus OnHTMLOpeningURL(wxHtmlURLType type, const wxString& url, wxString *redirect) const override;
    void OnFontChanged() override;

    void CreateHTMLParser() const;
    [[nodiscard]] double GetPixelScale() const;

    void OnDPIChanged(wxDPIChangedEvent& event);

private:
    wxString m_outFormat{};

    DECLARE_CLASS(QSPListBox)
};
