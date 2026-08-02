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
#include <wx/filename.h>
#include "animwin.h"

class QSPImgCanvas : public wxWindow
{
public:
    QSPImgCanvas(wxWindow *parent, wxWindowID id);
    ~QSPImgCanvas() override = default;

    bool OpenFile(const wxString& fullPath);
    void RefreshUI();

    bool SetBackgroundColour(const wxColour& color) override;

protected:
    void OnSize(wxSizeEvent& event);
    void OnPaintEvent(wxPaintEvent &event);

private:
    bool m_isAnim{false};
    wxImage m_image{};
    wxBitmap m_cachedBitmap{};
    QSPAnimWin* m_animation{nullptr};
    wxString m_path{};
    int m_posX{0};
    int m_posY{0};

    DECLARE_CLASS(QSPImgCanvas)
};
