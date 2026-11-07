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
#include <wx/fontmap.h>

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
) : QSPListBoxBase(parent, id, type)
{
    Bind(wxEVT_DPI_CHANGED, &QSPListBox::OnDPIChanged, this);

    m_outFormat = wxString::Format(
        "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=%s\">"
        "<BODY LINK=#%%s><FONT COLOR=#%%s><TABLE CELLSPACING=4 CELLPADDING=0><TR>%%s</TR></TABLE></FONT></BODY>",
        wxFontMapper::GetEncodingName(wxLocale::GetSystemEncoding())
    );

    const wxString fontName = m_font.GetFaceName();
    SetStandardFonts(m_font.GetPointSize(), fontName, fontName);
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

void QSPListBox::OnFontChanged()
{
    const wxString fontName = m_font.GetFaceName();
    SetStandardFonts(m_font.GetPointSize(), fontName, fontName);
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
    return wxString::Format(m_outFormat, linkColor, color, GetItemCells(n, false));
}
