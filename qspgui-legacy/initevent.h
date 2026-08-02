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

class wxInitEvent : public wxEvent
{
public:
    wxInitEvent();
    wxInitEvent(const wxInitEvent& event);

    [[nodiscard]] wxString GetInitString() const { return m_initString; }
    void SetInitString(wxString str) { m_initString = std::move(str); }

    [[nodiscard]] wxEvent* Clone() const override
    {
        return new wxInitEvent(*this);
    }
private:
    wxString m_initString{};

    DECLARE_DYNAMIC_CLASS(wxInitEvent)
};
