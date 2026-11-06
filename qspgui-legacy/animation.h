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
#include <wx/animdecod.h>
#include <wx/stream.h>
#include <memory>

class QSPAnimation
{
public:
    bool Load(wxInputStream &stream);

    [[nodiscard]] bool IsOk() const { return m_decoder != nullptr; }
    [[nodiscard]] wxSize GetSize() const { return m_canvas.GetSize(); }
    [[nodiscard]] const wxImage &GetFrame() const { return m_canvas; }
    [[nodiscard]] long GetDelay() const;

    void Rewind();
    void Advance();

private:
    void ComposeUpTo(unsigned int frame);
    void DrawFrame(unsigned int frame);
    void ClearArea(const wxPoint &pos, const wxSize &size);

    std::unique_ptr<wxAnimationDecoder> m_decoder;
    wxImage m_canvas;
    unsigned int m_frame{0};
};
