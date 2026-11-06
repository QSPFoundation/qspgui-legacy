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
#include "animation.h"

class QSPAnimWin : public wxWindow
{
public:
    explicit QSPAnimWin(wxWindow *parent);

    [[nodiscard]] bool LoadFile(const wxString &filename);
    void Play();
    void Stop();
    void RefreshUI();

private:
    void UpdateScaledFrame();

    void OnPaint(wxPaintEvent &event);
    void OnSize(wxSizeEvent &event);
    void OnTimer(wxTimerEvent &event);

    QSPAnimation m_animation;
    wxBitmap m_scaledFrame; // the current frame fitted into the window
    wxPoint m_scaledPos;
    wxTimer m_timer;
    bool m_isPlaying{false};

    DECLARE_CLASS(QSPAnimWin)
};
