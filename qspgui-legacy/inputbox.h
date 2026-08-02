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

wxDECLARE_EVENT(wxEVT_ENTER, wxCommandEvent);

class QSPInputBox : public wxTextCtrl
{
public:
    QSPInputBox(wxWindow *parent, wxWindowID id);

    void SetText(const wxString& text, bool toChangeValue = true);
    [[nodiscard]] wxString GetText() const { return m_text; }

protected:
    void OnChar(wxKeyEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnMouseWheel(wxMouseEvent& event);

private:
    wxString m_text{};
    wxArrayString m_strings{};
    int m_selIndex{-1};

    DECLARE_CLASS(QSPInputBox)
};
