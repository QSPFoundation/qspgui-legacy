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

#include "animation.h"
#include <wx/animate.h>
#include <algorithm>
#include <cstring>

namespace
{
    constexpr int MAX_ANIMATION_SIZE = 8192;
}

bool QSPAnimation::Load(wxInputStream &stream)
{
    m_decoder.reset();
    m_canvas = wxImage();

    for (auto node = wxAnimation::GetHandlers().GetFirst(); node; node = node->GetNext())
    {
        const wxAnimationDecoder *handler = node->GetData();
        if (!handler->CanRead(stream)) continue;

        std::unique_ptr<wxAnimationDecoder> decoder(handler->Clone());
        if (!decoder->Load(stream) || decoder->GetFrameCount() < 2) return false;
        const wxSize size = decoder->GetAnimationSize();
        if (size.x <= 0 || size.y <= 0 || size.x > MAX_ANIMATION_SIZE || size.y > MAX_ANIMATION_SIZE) return false;

        m_canvas.Create(size, false);
        if (!m_canvas.IsOk()) return false;
        m_canvas.SetAlpha();
        m_decoder = std::move(decoder);
        Rewind();
        return true;
    }
    return false;
}

long QSPAnimation::GetDelay() const
{
    const long delay = m_decoder ? m_decoder->GetDelay(m_frame) : 0;
    return delay > 0 ? delay : 1;
}

void QSPAnimation::Rewind()
{
    if (m_decoder) ComposeUpTo(0);
}

void QSPAnimation::Advance()
{
    if (!m_decoder) return;
    const unsigned int previous = m_frame;
    const unsigned int next = previous + 1 < m_decoder->GetFrameCount() ? previous + 1 : 0;
    if (next == 0 || m_decoder->GetDisposalMethod(previous) == wxANIM_TOPREVIOUS)
    {
        ComposeUpTo(next);
        return;
    }
    if (m_decoder->GetDisposalMethod(previous) == wxANIM_TOBACKGROUND)
        ClearArea(m_decoder->GetFramePosition(previous), m_decoder->GetFrameSize(previous));
    DrawFrame(next);
    m_frame = next;
}

void QSPAnimation::ComposeUpTo(const unsigned int frame)
{
    ClearArea(wxPoint(), m_canvas.GetSize());
    for (unsigned int i = 0; i < frame; ++i)
    {
        switch (m_decoder->GetDisposalMethod(i))
        {
        case wxANIM_DONOTREMOVE:
        case wxANIM_UNSPECIFIED:
            DrawFrame(i);
            break;
        case wxANIM_TOBACKGROUND:
            ClearArea(m_decoder->GetFramePosition(i), m_decoder->GetFrameSize(i));
            break;
        case wxANIM_TOPREVIOUS:
            break;
        }
    }
    DrawFrame(frame);
    m_frame = frame;
}

void QSPAnimation::DrawFrame(const unsigned int frame)
{
    wxImage image;
    if (!m_decoder->ConvertToImage(frame, &image) || !image.IsOk()) return;
    if (!image.HasAlpha()) image.InitAlpha(); // turns the transparent color of GIF frames into alpha

    const wxPoint pos = m_decoder->GetFramePosition(frame);
    const int left = std::max(pos.x, 0), top = std::max(pos.y, 0);
    const int right = std::min(pos.x + image.GetWidth(), m_canvas.GetWidth());
    const int bottom = std::min(pos.y + image.GetHeight(), m_canvas.GetHeight());

    const unsigned char *srcRgb = image.GetData(), *srcAlpha = image.GetAlpha();
    unsigned char *dstRgb = m_canvas.GetData(), *dstAlpha = m_canvas.GetAlpha();
    for (int y = top; y < bottom; ++y)
    {
        for (int x = left; x < right; ++x)
        {
            const size_t src = (size_t)(y - pos.y) * image.GetWidth() + (x - pos.x);
            const size_t dst = (size_t)y * m_canvas.GetWidth() + x;
            const unsigned int a = srcAlpha[src];
            if (a == 0) continue;
            if (a == 255)
            {
                std::memcpy(dstRgb + dst * 3, srcRgb + src * 3, 3);
                dstAlpha[dst] = 255;
                continue;
            }
            const unsigned int da = dstAlpha[dst] * (255 - a) / 255;
            const unsigned int outA = a + da;
            for (int c = 0; c < 3; ++c)
                dstRgb[dst * 3 + c] = (unsigned char)((srcRgb[src * 3 + c] * a + dstRgb[dst * 3 + c] * da) / outA);
            dstAlpha[dst] = (unsigned char)outA;
        }
    }
}

void QSPAnimation::ClearArea(const wxPoint &pos, const wxSize &size)
{
    const wxRect area = wxRect(pos, size).Intersect(wxRect(m_canvas.GetSize()));
    unsigned char *alpha = m_canvas.GetAlpha();
    for (int y = area.GetTop(); y <= area.GetBottom(); ++y)
        std::memset(alpha + (size_t)y * m_canvas.GetWidth() + area.GetLeft(), 0, area.GetWidth());
}
