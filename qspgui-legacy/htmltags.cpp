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

#include <wx/wx.h>
#include <wx/html/htmlcell.h>
#include <wx/html/m_templ.h>
#include <wx/html/styleparams.h>

namespace
{
    class QSPAnchorCell : public wxHtmlCell
    {
    public:
        QSPAnchorCell(const wxHtmlTag &tag, const wxString &name) : wxHtmlCell(tag), m_name(name) {}

        void Draw(wxDC &, int, int, int, int, wxHtmlRenderingInfo &) override {}

        const wxHtmlCell *Find(const int condition, const void *param) const override
        {
            if (CheckIsAnchor(condition, param, m_name) || CheckIsAnchor(condition, param))
                return this;
            return wxHtmlCell::Find(condition, param);
        }

    private:
        wxString m_name;
    };

    class QSPLinkTagHandler : public wxHtmlWinTagHandler
    {
    public:
        wxString GetSupportedTags() override { return "A"; }

        bool HandleTag(const wxHtmlTag &tag) override
        {
            wxString name;
            if (tag.GetParamAsString("NAME", &name) || tag.HasParam("ID"))
                m_WParser->GetContainer()->InsertCell(new QSPAnchorCell(tag, name));

            wxString href;
            if (!tag.GetParamAsString("HREF", &href)) return false;

            const wxHtmlLinkInfo oldLink = m_WParser->GetLink();
            const wxColour oldColor = m_WParser->GetActualColor();
            const wxColour oldBackColor = m_WParser->GetActualBackgroundColor();
            const int oldBackMode = m_WParser->GetActualBackgroundMode();
            const int oldSize = m_WParser->GetFontSize();
            const int oldBold = m_WParser->GetFontBold();
            const int oldItalic = m_WParser->GetFontItalic();
            const int oldUnderlined = m_WParser->GetFontUnderlined();
            const wxString oldFontFace = m_WParser->GetFontFace();

            m_WParser->SetActualColor(m_WParser->GetLinkColor());
            m_WParser->GetContainer()->InsertCell(new wxHtmlColourCell(m_WParser->GetLinkColor()));
            if (!tag.GetParam("CLASS").IsSameAs("PLAIN", false))
                m_WParser->SetFontUnderlined(true);
            m_WParser->GetContainer()->InsertCell(new wxHtmlFontCell(m_WParser->CreateCurrentFont()));
            m_WParser->SetLink(wxHtmlLinkInfo(href, tag.GetParam("TARGET")));

            const wxHtmlStyleParams styleParams(tag);
            ApplyStyle(styleParams);

            ParseInner(tag);

            m_WParser->SetLink(oldLink);
            m_WParser->SetFontSize(oldSize);
            m_WParser->SetFontBold(oldBold);
            m_WParser->SetFontFace(oldFontFace);
            m_WParser->SetFontItalic(oldItalic);
            m_WParser->SetFontUnderlined(oldUnderlined);
            m_WParser->GetContainer()->InsertCell(new wxHtmlFontCell(m_WParser->CreateCurrentFont()));
            m_WParser->SetActualColor(oldColor);
            m_WParser->GetContainer()->InsertCell(new wxHtmlColourCell(oldColor));

            if (oldBackMode != m_WParser->GetActualBackgroundMode() || oldBackColor != m_WParser->GetActualBackgroundColor())
            {
                m_WParser->SetActualBackgroundMode(oldBackMode);
                m_WParser->SetActualBackgroundColor(oldBackColor);
                m_WParser->GetContainer()->InsertCell(new wxHtmlColourCell(oldBackColor,
                    oldBackMode == wxBRUSHSTYLE_TRANSPARENT ? wxHTML_CLR_TRANSPARENT_BACKGROUND : wxHTML_CLR_BACKGROUND));
            }
            return true;
        }
    };
}

class QSPHtmlTagsModule : public wxHtmlTagsModule
{
public:
    QSPHtmlTagsModule()
    {
        AddDependency("wxHTML_ModuleLinks");
    }

    void FillHandlersTable(wxHtmlWinParser *parser) override
    {
        parser->AddTagHandler(new QSPLinkTagHandler);
    }

private:
    wxDECLARE_DYNAMIC_CLASS(QSPHtmlTagsModule);
};

wxIMPLEMENT_DYNAMIC_CLASS(QSPHtmlTagsModule, wxHtmlTagsModule);
