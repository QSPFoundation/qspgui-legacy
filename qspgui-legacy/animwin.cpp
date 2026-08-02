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

#include "animwin.h"

wxIMPLEMENT_CLASS(QSPAnimWin, wxGenericAnimationCtrl);

QSPAnimWin::QSPAnimWin(wxWindow *parent) : wxGenericAnimationCtrl(
    parent,
    wxID_ANY,
    wxNullAnimation,
    wxDefaultPosition,
    wxDefaultSize,
    wxNO_BORDER | wxAC_NO_AUTORESIZE
)
{
    auto forwardEvent = [](auto &event)
    {
        event.Skip();
        event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
    };

    Bind(wxEVT_KEY_UP, forwardEvent);
    Bind(wxEVT_MOUSEWHEEL, forwardEvent);
    Bind(wxEVT_LEFT_DOWN, forwardEvent);
}

void QSPAnimWin::RefreshUI()
{
    IncrementalUpdateBackingStore();
    Refresh();
}

bool QSPAnimWin::LoadFile(const wxString &filename, wxAnimationType type)
{
    if (IsPlaying()) { Stop(); }

    return wxGenericAnimationCtrl::LoadFile(filename, type) && GetAnimation().GetFrameCount() > 1;
}
