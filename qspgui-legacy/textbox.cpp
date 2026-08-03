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

#include "textbox.h"
#include "comtools.h"
#include <wx/wupdlock.h>
#include <limits>
#include <algorithm>

wxIMPLEMENT_CLASS(QSPTextBox, wxHtmlWindow);

wxHtmlOpeningStatus QSPTextBox::OnHTMLOpeningURL(
    [[maybe_unused]] wxHtmlURLType type,
    const wxString &url,
    wxString *redirect) const
{
    if (!m_pathProvider) return wxHTML_OPEN;
    if (m_pathProvider->IsValidFullPath(url)) return wxHTML_OPEN;

    *redirect = m_pathProvider->ComposeGamePath(url);

    return wxHTML_REDIRECT;
}

QSPTextBox::QSPTextBox(wxWindow *parent, const wxWindowID id) : wxHtmlWindow(parent, id)
{
    Bind(wxEVT_SIZE, &QSPTextBox::OnSize, this);
    Bind(wxEVT_ERASE_BACKGROUND, &QSPTextBox::OnEraseBackground, this);
    Bind(wxEVT_KEY_UP, &QSPTextBox::OnKeyUp, this);
    Bind(wxEVT_MOUSEWHEEL, &QSPTextBox::OnMouseWheel, this);
    Bind(wxEVT_LEFT_DOWN, &QSPTextBox::OnMouseClick, this);

    SetBorders(FromDIP(5));

    m_outFormat = wxString::Format(
        "<HTML><META HTTP-EQUIV = \"Content-Type\" CONTENT = \"text/html; charset=%s\">"
        "<BODY><FONT COLOR = #%%s>%%s</FONT></BODY></HTML>",
        wxFontMapper::GetEncodingName(wxLocale::GetSystemEncoding())
    );

    const wxString fontName = m_font.GetFaceName();
    SetStandardFonts(m_font.GetPointSize(), fontName, fontName);
}

void QSPTextBox::SetIsHtml(const bool isHtml)
{
    if (m_toUseHtml == isHtml) return;

    m_toUseHtml = isHtml;

    RefreshUI();
}

void QSPTextBox::RefreshUI()
{
    const wxString color = QSPTools::GetHexColor(GetForegroundColour());
    const wxString text = QSPTools::HtmlizeWhitespaces(m_toUseHtml ? m_text : QSPTools::ProceedAsPlain(m_text));

    wxWindowUpdateLocker noUpdates{this};

    SetPage(wxString::Format(m_outFormat, color, text));
}

void QSPTextBox::LoadBackImage(const wxString &imagePath)
{
    if (m_imagePath == imagePath) return;

    m_imagePath = imagePath;

    if (
        const wxString fullImagePath = m_pathProvider ? m_pathProvider->ComposeGamePath(imagePath) : imagePath;
        wxFileExists(fullImagePath)
    )
    {
        if (wxImage image; image.LoadFile(fullImagePath))
        {
            SetBackgroundImage(wxBitmap(image));
            Refresh();
            return;
        }
    }

    SetBackgroundImage(wxNullBitmap);
    Refresh();
}

void QSPTextBox::SetText(const wxString &text, bool toScroll)
{
    if (m_text == text) return;

    if (toScroll && (m_text.IsEmpty() || !text.StartsWith(m_text)))
    {
        toScroll = false;
    }
    m_text = text;
    RefreshUI();

    Scroll(0, toScroll ? std::numeric_limits<int>::max() : 0);
}

void QSPTextBox::SetTextFont(const wxFont &font)
{
    const int fontSize = font.GetPointSize();

    if (
        const wxString fontName = font.GetFaceName();
        !m_font.GetFaceName().IsSameAs(fontName, false) || m_font.GetPointSize() != fontSize
    )
    {
        m_font = font;
        wxWindowUpdateLocker noUpdates{this};
        SetStandardFonts(fontSize, fontName, fontName);
    }
}

void QSPTextBox::SetLinkColor(const wxColour &clr)
{
    m_Parser->SetLinkColor(clr);
    RefreshUI();
}

void QSPTextBox::OnKeyUp(wxKeyEvent &event)
{
    event.Skip();
    wxKeyEvent keyEvent{event};
    keyEvent.ResumePropagation(wxEVENT_PROPAGATE_MAX);
    TryAfter(keyEvent);
}

void QSPTextBox::OnMouseWheel(wxMouseEvent &event)
{
    event.Skip();
    if (wxFindWindowAtPoint(wxGetMousePosition()) != this)
        event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
}

void QSPTextBox::OnMouseClick(wxMouseEvent &event)
{
    event.Skip();
    event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
}

void QSPTextBox::OnSize(wxSizeEvent &event)
{
    CalcImageSize();
    wxHtmlWindow::OnSize(event);
}

void QSPTextBox::OnEraseBackground(const wxEraseEvent &event)
{
    wxDC *dc = event.GetDC();
    dc->SetBackground(wxBrush(GetBackgroundColour(), wxBRUSHSTYLE_SOLID));
    dc->Clear();

    if (m_bmpBg.IsOk() && m_bmpRealBg.IsOk())
    {
        const wxPoint pt = dc->GetDeviceOrigin();
        dc->DrawBitmap(m_bmpRealBg, m_posX - pt.x, m_posY - pt.y, true);
    }
}

void QSPTextBox::SetBackgroundImage(const wxBitmap &bmpBg)
{
    m_bmpBg = bmpBg;
    CalcImageSize();
}

void QSPTextBox::CalcImageSize()
{
    if (!m_bmpBg.IsOk()) return;

    const wxSize size = GetClientSize();
    const int w = std::max(size.GetWidth(), 1);
    const int h = std::max(size.GetHeight(), 1);

    const int srcW = m_bmpBg.GetWidth();
    const int srcH = m_bmpBg.GetHeight();

    if (srcW == 0 || srcH == 0) return;

    int destW = srcW * h / srcH;
    int destH = srcH * w / srcW;

    if (destW > w)
        destW = w;
    else
        destH = h;

    m_posX = (w - destW) / 2;
    m_posY = (h - destH) / 2;

    if (destW > 0 && destH > 0)
        m_bmpRealBg = wxBitmap(m_bmpBg.ConvertToImage().Scale(destW, destH));
    else
        m_bmpRealBg = wxNullBitmap;
}
