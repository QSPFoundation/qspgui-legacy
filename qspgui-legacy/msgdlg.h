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
#include "textbox.h"
#include "pathprovider.h"

constexpr int ID_MSG_DESC = wxID_HIGHEST + 1;

class QSPMsgDlg : public wxDialog
{
public:
    QSPMsgDlg(wxWindow *parent,
              wxWindowID id,
              const wxColour &backColor,
              const wxColour &fontColor,
              const wxFont &font,
              const wxString &caption,
              const wxString &text,
              bool isHtml,
              PathProvider *pathProvider);

protected:
    void OnInitDialog(wxInitDialogEvent &event);
    void OnLinkClicked(wxHtmlLinkEvent &event);

private:
    static constexpr int MinWidth = 450;
    static constexpr int MaxWidth = 550;
    static constexpr int MinHeight = 100;
    static constexpr int MaxHeight = 350;

    QSPTextBox *m_desc{nullptr};

    DECLARE_CLASS(QSPMsgDlg)
};
