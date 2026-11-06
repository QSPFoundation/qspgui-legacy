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
#include <wx/animate.h>
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
    m_decoder.reset();
    m_canvas = m_scaledFrame = wxNullBitmap;

    wxFileInputStream stream(filename);
    if (!stream.IsOk()) return false;

    for (auto node = wxAnimation::GetHandlers().GetFirst(); node; node = node->GetNext())
    {
        const wxAnimationDecoder *handler = node->GetData();
        if (!handler->CanRead(stream)) continue;

        std::unique_ptr<wxAnimationDecoder> decoder(handler->Clone());
        const wxSize size = decoder->Load(stream) ? decoder->GetAnimationSize() : wxSize();
        if (decoder->GetFrameCount() < 2 || size.x <= 0 || size.y <= 0) return false;
        if (!m_canvas.Create(size)) return false;

        m_decoder = std::move(decoder);
        ResetToFrame(0);
        return true;
    }
    return false;
}

void QSPAnimWin::Play()
{
    if (!m_decoder) return;
    ResetToFrame(0);
    m_isPlaying = true;
    const long delay = m_decoder->GetDelay(0);
    m_timer.StartOnce(delay > 0 ? delay : 1);
}

void QSPAnimWin::Stop()
{
    m_timer.Stop();
    m_isPlaying = false;
}

void QSPAnimWin::RefreshUI()
{
    if (m_decoder) ResetToFrame(m_currentFrame);
}

void QSPAnimWin::ResetToFrame(const unsigned int frame)
{
    wxMemoryDC dc(m_canvas);
    ClearArea(dc, wxPoint(), m_canvas.GetSize());
    for (unsigned int i = 0; i < frame; ++i)
    {
        switch (m_decoder->GetDisposalMethod(i))
        {
        case wxANIM_DONOTREMOVE:
        case wxANIM_UNSPECIFIED:
            DrawFrame(dc, i);
            break;
        case wxANIM_TOBACKGROUND:
            ClearArea(dc, m_decoder->GetFramePosition(i), m_decoder->GetFrameSize(i));
            break;
        case wxANIM_TOPREVIOUS:
            break;
        }
    }
    DrawFrame(dc, frame);
    dc.SelectObject(wxNullBitmap);

    m_currentFrame = frame;
    UpdateScaledFrame();
}

void QSPAnimWin::AdvanceFrame()
{
    const unsigned int previous = m_currentFrame;
    const unsigned int next = previous + 1 < m_decoder->GetFrameCount() ? previous + 1 : 0;
    if (next == 0 || m_decoder->GetDisposalMethod(previous) == wxANIM_TOPREVIOUS)
    {
        ResetToFrame(next);
        return;
    }

    wxMemoryDC dc(m_canvas);
    if (m_decoder->GetDisposalMethod(previous) == wxANIM_TOBACKGROUND)
        ClearArea(dc, m_decoder->GetFramePosition(previous), m_decoder->GetFrameSize(previous));
    DrawFrame(dc, next);
    dc.SelectObject(wxNullBitmap);

    m_currentFrame = next;
    UpdateScaledFrame();
}

void QSPAnimWin::DrawFrame(wxDC &dc, const unsigned int frame) const
{
    if (wxImage image; m_decoder->ConvertToImage(frame, &image))
        dc.DrawBitmap(wxBitmap(image), m_decoder->GetFramePosition(frame), true);
}

void QSPAnimWin::ClearArea(wxDC &dc, const wxPoint &pos, const wxSize &size) const
{
    dc.SetBrush(wxBrush(GetBackgroundColour()));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(pos, size);
}

void QSPAnimWin::UpdateScaledFrame()
{
    const wxSize window = GetClientSize();
    const wxSize source = m_canvas.IsOk() ? m_canvas.GetSize() : wxSize();
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
    m_scaledFrame = target == source
        ? m_canvas
        : wxBitmap(m_canvas.ConvertToImage().Scale(target.x, target.y, wxIMAGE_QUALITY_BILINEAR));
    Refresh();
}

void QSPAnimWin::OnPaint(wxPaintEvent &)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    if (m_scaledFrame.IsOk())
        dc.DrawBitmap(m_scaledFrame, m_scaledPos);
}

void QSPAnimWin::OnSize(wxSizeEvent &event)
{
    event.Skip();
    UpdateScaledFrame();
}

void QSPAnimWin::OnTimer(wxTimerEvent &)
{
    if (!m_isPlaying || !m_decoder) return;
    AdvanceFrame();
    const long delay = m_decoder->GetDelay(m_currentFrame);
    m_timer.StartOnce(delay > 0 ? delay : 1);
}
