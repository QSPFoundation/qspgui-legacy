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

#include "imgcanvas.h"
#include <algorithm>

wxIMPLEMENT_CLASS(QSPImgCanvas, wxWindow);

QSPImgCanvas::QSPImgCanvas(wxWindow *parent, const wxWindowID id)
    : wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE)
{
    Bind(wxEVT_SIZE, &QSPImgCanvas::OnSize, this);
    Bind(wxEVT_PAINT, &QSPImgCanvas::OnPaintEvent, this);

    auto forwardEvent = [](auto& event) {
        event.Skip();
        event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
    };
    Bind(wxEVT_KEY_UP, forwardEvent);
    Bind(wxEVT_MOUSEWHEEL, forwardEvent);
    Bind(wxEVT_LEFT_DOWN, forwardEvent);

    m_animation = new QSPAnimWin{this};
    m_animation->Move(0, 0);
    m_animation->Hide();
}

bool QSPImgCanvas::OpenFile(const wxString &fullPath)
{
    if (m_path == fullPath) return true;

    m_animation->Stop();
    m_isAnim = false;

    if (wxFileExists(fullPath))
    {
        bool ret = false;

        if ((m_isAnim = m_animation->LoadFile(fullPath)))
        {
            m_animation->Show();
            ret = true;
        }
        else
        {
            m_animation->Hide();
            ret = m_image.LoadFile(fullPath);
        }

        if (ret)
        {
            wxSizeEvent e;
            OnSize(e);

            if (m_isAnim) m_animation->Play();
            else Refresh();

            m_path = fullPath;
            return true;
        }
    }

    return false;
}

void QSPImgCanvas::RefreshUI()
{
    if (m_isAnim) m_animation->RefreshUI();
    else Refresh();
}

bool QSPImgCanvas::SetBackgroundColour(const wxColour &color)
{
    wxWindow::SetBackgroundColour(color);
    m_animation->SetBackgroundColour(color);
    return true;
}

void QSPImgCanvas::OnSize(wxSizeEvent &event)
{
    if (!m_isAnim && !m_image.IsOk())
    {
        event.Skip();
        return;
    }

    const wxSize size = GetClientSize();
    const int w = std::max(size.GetWidth(), 1);
    const int h = std::max(size.GetHeight(), 1);

    if (m_isAnim)
    {
        m_animation->SetSize(w, h);
    }
    else
    {
        const int srcW = m_image.GetWidth();
        const int srcH = m_image.GetHeight();

        if (srcW == 0 || srcH == 0) return;

        int destW = srcW * h / srcH;
        int destH = srcH * w / srcW;

        if (destW > w) destW = w;
        else destH = h;

        m_posX = (w - destW) / 2;
        m_posY = (h - destH) / 2;

        if (destW > 0 && destH > 0)
            m_cachedBitmap = wxBitmap(m_image.Scale(destW, destH, wxIMAGE_QUALITY_BILINEAR));
        else
            m_cachedBitmap = wxNullBitmap;
    }
}

void QSPImgCanvas::OnPaintEvent(wxPaintEvent& event)
{
    if (m_isAnim || !m_image.IsOk())
    {
        event.Skip();
        return;
    }

    wxPaintDC dc(this);
    if (m_cachedBitmap.IsOk())
    {
        dc.DrawBitmap(m_cachedBitmap, m_posX, m_posY, true);
    }
}
