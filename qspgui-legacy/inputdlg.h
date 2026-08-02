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
#include <wx/statline.h>
#include <wx/valgen.h>
#include "textbox.h"
#include "pathprovider.h"

constexpr int ID_INPUT_DESC = wxID_HIGHEST + 20;

class QSPInputDlg : public wxDialog
{
public:
    QSPInputDlg(wxWindow* parent,
                wxWindowID id,
                const wxColour& backColor,
                const wxColour& fontColor,
                const wxFont& font,
                const wxString& caption,
                const wxString& text,
                bool isHtml,
                PathProvider *pathProvider);

    [[nodiscard]] wxString GetText() const { return m_text; }

protected:
    void OnInitDialog(wxInitDialogEvent& event);
    void OnLinkClicked(wxHtmlLinkEvent& event);

private:
    // Константы делаем static constexpr
    static constexpr int MinWidth = 420;
    static constexpr int MaxWidth = 550;
    static constexpr int MinHeight = 150;
    static constexpr int MaxHeight = 350;

    QSPTextBox* m_desc{nullptr};
    wxString m_text{};

    DECLARE_CLASS(QSPInputDlg)
};

