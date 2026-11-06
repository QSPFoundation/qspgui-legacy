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
#include <wx/dcbuffer.h>
#include <wx/wfstream.h>

wxIMPLEMENT_CLASS(QSPAnimWin, wxWindow);

QSPAnimWin::QSPAnimWin(wxWindow *parent) :
    wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE),
    m_timer(this)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    Bind(wxEVT_PAINT, &QSPAnimWin::OnPaint, this);
    Bind(wxEVT_SIZE, &QSPAnimWin::OnSize, this);
    Bind(wxEVT_TIMER, &QSPAnimWin::OnTimer, this);

    auto forwardEvent = [](auto &event)
    {
        event.Skip();
        event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
    };

    Bind(wxEVT_KEY_UP, forwardEvent);
    Bind(wxEVT_MOUSEWHEEL, forwardEvent);
}

bool QSPAnimWin::LoadFile(const wxString &filename)
{
    Stop();
    m_scaledFrame = wxNullBitmap;

    wxFileInputStream stream(filename);
    if (!stream.IsOk() || !m_animation.Load(stream)) return false;

    UpdateScaledFrame();
    return true;
}

void QSPAnimWin::Play()
{
    if (!m_animation.IsOk()) return;
    m_animation.Rewind();
    UpdateScaledFrame();
    m_isPlaying = true;
    m_timer.StartOnce(m_animation.GetDelay());
}

void QSPAnimWin::Stop()
{
    m_timer.Stop();
    m_isPlaying = false;
}

void QSPAnimWin::RefreshUI()
{
    Refresh();
}

void QSPAnimWin::UpdateScaledFrame()
{
    const wxSize window = GetClientSize();
    const wxSize source = m_animation.IsOk() ? m_animation.GetSize() : wxSize();
    if (window.x <= 0 || window.y <= 0 || source.x <= 0 || source.y <= 0)
    {
        m_scaledFrame = wxNullBitmap;
        Refresh();
        return;
    }

    wxSize target(source.x * window.y / source.y, source.y * window.x / source.x);
    if (target.x > window.x)
        target.x = window.x;
    else
        target.y = window.y;
    target.IncTo(wxSize(1, 1));

    m_scaledPos = wxPoint((window.x - target.x) / 2, (window.y - target.y) / 2);
    const wxImage &frame = m_animation.GetFrame();
    m_scaledFrame = wxBitmap(target == source ? frame : frame.Scale(target.x, target.y, wxIMAGE_QUALITY_BILINEAR));

    Refresh();
}

void QSPAnimWin::OnPaint(wxPaintEvent &)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    if (m_scaledFrame.IsOk())
        dc.DrawBitmap(m_scaledFrame, m_scaledPos, true);
}

void QSPAnimWin::OnSize(wxSizeEvent &event)
{
    event.Skip();
    UpdateScaledFrame();
}

void QSPAnimWin::OnTimer(wxTimerEvent &)
{
    if (!m_isPlaying || !m_animation.IsOk()) return;
    m_animation.Advance();
    UpdateScaledFrame();
    m_timer.StartOnce(m_animation.GetDelay());
}
