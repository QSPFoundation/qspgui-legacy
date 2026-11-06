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

#include "listbox.h"
#include "comtools.h"
#include <wx/wupdlock.h>

wxIMPLEMENT_CLASS(QSPListBox, wxHtmlListBox);

wxHtmlOpeningStatus QSPListBox::OnHTMLOpeningURL(
    [[maybe_unused]] wxHtmlURLType type,
    const wxString &url,
    wxString *redirect
) const
{
    if (!m_pathProvider) return wxHTML_OPEN;

    const wxString resourceUrl = QSPTools::GetGameResourceURL(*m_pathProvider, url);
    if (resourceUrl.IsEmpty()) return wxHTML_BLOCK;
    if (resourceUrl == url) return wxHTML_OPEN;

    *redirect = resourceUrl;
    return wxHTML_REDIRECT;
}

QSPListBox::QSPListBox(
    wxWindow *parent,
    const wxWindowID id,
    const ListBoxType type
) : wxHtmlListBox(parent, id, wxDefaultPosition, wxDefaultSize, wxNO_BORDER), m_type(type)
{
    Bind(wxEVT_MOTION, &QSPListBox::OnMouseMove, this);
    Bind(wxEVT_LEFT_DOWN, &QSPListBox::OnMouseClick, this);
    Bind(wxEVT_CHAR, &QSPListBox::OnChar, this);
    Bind(wxEVT_KEY_UP, &QSPListBox::OnKeyUp, this);
    Bind(wxEVT_MOUSEWHEEL, &QSPListBox::OnMouseWheel, this);
    Bind(wxEVT_DPI_CHANGED, &QSPListBox::OnDPIChanged, this);

    const wxString commonPart = wxString::Format(
        "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=%s\">"
        "<BODY LINK=#%%%%s><FONT COLOR=#%%%%s><TABLE CELLSPACING=4 CELLPADDING=0><TR>%%s</TR></TABLE></FONT></BODY>",
        wxFontMapper::GetEncodingName(wxLocale::GetSystemEncoding())
    );

    m_outFormat = wxString::Format(commonPart, "<TD WIDTH=100%%>%s</TD>");
    m_outFormatNums = wxString::Format(commonPart, "<TD>[%d]</TD><TD WIDTH=100%%>%s</TD>");
    m_outFormatImage = wxString::Format(commonPart, "<TD><IMG SRC=\"%s\"></TD><TD WIDTH=100%%>%s</TD>");
    m_outFormatImageNums = wxString::Format(commonPart, "<TD>[%d]</TD><TD><IMG SRC=\"%s\"></TD><TD WIDTH=100%%>%s</TD>");

    const wxString fontName = m_font.GetFaceName();
    SetStandardFonts(m_font.GetPointSize(), fontName, fontName);
    SetSelectionBackground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
}

void QSPListBox::SetStandardFonts(
    const int size,
    const wxString &normal_face,
    const wxString &fixed_face
)
{
    CreateHTMLParser();
    m_htmlParser->SetStandardFonts(size, normal_face, fixed_face);
    RefreshUI();
}

void QSPListBox::RefreshUI()
{
    wxWindowUpdateLocker noUpdates{this};
    RefreshAll();
}

void QSPListBox::BeginItems()
{
    m_newImages.Clear();
    m_newDescs.Clear();
}

void QSPListBox::AddItem(const wxString &image, const wxString &desc)
{
    m_newImages.Add(image);
    m_newDescs.Add(desc);
}

void QSPListBox::EndItems()
{
    if (m_images == m_newImages && m_descs == m_newDescs) return;

    m_images = m_newImages;
    m_descs = m_newDescs;

    wxWindowUpdateLocker noUpdates{this};

    const size_t count = m_descs.GetCount();
    SetItemCount(count);
    RefreshAll();

    if (count > 0) ScrollToRow(0);
}

void QSPListBox::SetIsHtml(const bool isHtml)
{
    if (m_toUseHtml == isHtml) return;

    m_toUseHtml = isHtml;
    RefreshUI();
}

void QSPListBox::SetToShowNums(const bool toShow)
{
    if (m_toShowNums == toShow) return;

    m_toShowNums = toShow;
    RefreshUI();
}

void QSPListBox::SetTextFont(const wxFont &font)
{
    const int fontSize = font.GetPointSize();

    if (
        const wxString fontName = font.GetFaceName();
        !m_font.GetFaceName().IsSameAs(fontName, false) || m_font.GetPointSize() != fontSize
    )
    {
        m_font = font;
        SetStandardFonts(fontSize, fontName, fontName);
    }
}

void QSPListBox::SetLinkColor(const wxColour &clr)
{
    m_linkColor = clr;
    RefreshUI();
}

void QSPListBox::CreateHTMLParser() const
{
    if (!m_htmlParser)
    {
        auto *self = const_cast<QSPListBox*>(this);
        self->m_htmlParser = new wxHtmlWinParser(self);
        m_htmlParser->SetDC(new wxClientDC(self), GetPixelScale(), 1.0);
        m_htmlParser->SetFS(&self->m_filesystem);
        m_htmlParser->SetStandardFonts();
    }
}

double QSPListBox::GetPixelScale() const
{
#ifdef wxHAS_DPI_INDEPENDENT_PIXELS
    return 1.0;
#else
    return GetDPIScaleFactor();
#endif
}

void QSPListBox::OnDPIChanged(wxDPIChangedEvent &event)
{
    event.Skip();
    if (m_htmlParser)
    {
        wxDC *dc = m_htmlParser->GetDC();
        m_htmlParser->SetDC(dc, GetPixelScale(), 1.0);
    }
    RefreshUI();
}

wxString QSPListBox::OnGetItem(const size_t n) const
{
    const wxString color = QSPTools::GetHexColor(GetForegroundColour());
    const wxString linkColor = QSPTools::GetHexColor(m_linkColor);
    const wxString text = QSPTools::HtmlizeWhitespaces(m_toUseHtml ? m_descs[n] : QSPTools::ProceedAsPlain(m_descs[n]));

    if (m_toShowNums && n < 9)
    {
        if (m_images[n].IsEmpty())
            return wxString::Format(m_outFormatNums, linkColor, color, static_cast<int>(n + 1), text);
        else
            return wxString::Format(m_outFormatImageNums, linkColor, color, static_cast<int>(n + 1), m_images[n], text);
    }
    else
    {
        if (m_images[n].IsEmpty())
            return wxString::Format(m_outFormat, linkColor, color, text);
        else
            return wxString::Format(m_outFormatImage, linkColor, color, m_images[n], text);
    }
}

void QSPListBox::OnMouseMove(wxMouseEvent &event)
{
    event.Skip();
    if (m_type == ListBoxType::Extended)
    {
        if (
            const int item = VirtualHitTest(event.GetY());
            item != wxNOT_FOUND
        )
            DoHandleItemClick(item, 0);
    }
}

void QSPListBox::OnMouseClick(wxMouseEvent &event)
{
    event.Skip();
    if (m_type == ListBoxType::Extended) OnLeftDClick(event);
}

void QSPListBox::OnChar(wxKeyEvent &event)
{
    event.Skip();
    if (m_type == ListBoxType::Extended && event.GetKeyCode() == WXK_RETURN && GetSelection() != wxNOT_FOUND)
    {
        wxCommandEvent clickEvent(wxEVT_LISTBOX_DCLICK, GetId());
        clickEvent.SetEventObject(this);
        clickEvent.SetInt(GetSelection());
        ProcessEvent(clickEvent);
        SetFocus();
    }
}

void QSPListBox::OnKeyUp(wxKeyEvent &event)
{
    event.Skip();
    event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
}

void QSPListBox::OnMouseWheel(wxMouseEvent &event)
{
    event.Skip();
    if (wxFindWindowAtPoint(wxGetMousePosition()) != this)
        event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
}
