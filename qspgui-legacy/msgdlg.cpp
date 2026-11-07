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

#include "msgdlg.h"
#include "comtools.h"
#include <algorithm>

wxIMPLEMENT_CLASS(QSPMsgDlg, wxDialog);

QSPMsgDlg::QSPMsgDlg(wxWindow *parent,
                     const wxWindowID id,
                     const wxColour &backColor,
                     const wxColour &fontColor,
                     const wxFont &font,
                     const wxString &caption,
                     const wxString &text,
                     bool isHtml,
                     PathProvider *pathProvider,
                     const wxString &renderer)
{
    if (!Create(parent, id, caption, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER))
    {
        return;
    }

    Bind(wxEVT_INIT_DIALOG, &QSPMsgDlg::OnInitDialog, this);

    wxWindowBase::SetBackgroundColour(backColor);

    auto *sizerUp = new wxBoxSizer{wxVERTICAL};

    m_desc = CreateHtmlView(this, wxID_ANY, renderer);
    m_desc->GetWindow()->Bind(wxEVT_HTML_LINK_CLICKED, &QSPMsgDlg::OnLinkClicked, this);
    m_desc->SetPathProvider(pathProvider);
    m_desc->SetIsHtml(isHtml);
    m_desc->GetWindow()->SetBackgroundColour(backColor);
    m_desc->GetWindow()->SetForegroundColour(fontColor);
    m_desc->SetTextFont(font);
    m_desc->SetText(text);

    auto *line = new wxStaticLine{this, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL};
    sizerUp->Add(m_desc->GetWindow(), 1, wxALL | wxGROW, FromDIP(2));
    sizerUp->Add(line, 0, wxALL | wxGROW, FromDIP(2));

    auto *sizerBottom = new wxBoxSizer{wxHORIZONTAL};
    auto *btnOk = new wxButton{this, wxID_OK, _("OK")};
    btnOk->SetDefault();
    btnOk->SetFont(font);
    btnOk->SetBackgroundColour(backColor);
    btnOk->SetForegroundColour(fontColor);
    sizerBottom->Add(btnOk, 0, wxALL, FromDIP(2));

    auto *sizerMain = new wxBoxSizer{wxVERTICAL};
    sizerMain->Add(sizerUp, 1, wxGROW, 0);
    sizerMain->Add(sizerBottom, 0, wxALIGN_RIGHT, 0);

    sizerMain->SetMinSize(FromDIP(wxSize{MinWidth, MinHeight}));
    SetSizerAndFit(sizerMain);
    btnOk->SetFocus();
}

void QSPMsgDlg::OnInitDialog([[maybe_unused]] wxInitDialogEvent &event)
{
    const wxWindow *desc = m_desc->GetWindow();
    const int deltaH = GetClientSize().GetHeight() - desc->GetSize().GetHeight();
    const int deltaW = GetClientSize().GetWidth() - desc->GetSize().GetWidth();

    const wxSize content = m_desc->GetContentSize();
    int height = content.GetHeight() + desc->GetCharHeight() + deltaH;
    int width = content.GetWidth() + deltaW;

    height = std::clamp(height, FromDIP(MinHeight), FromDIP(MaxHeight));
    width = std::clamp(width, FromDIP(MinWidth), FromDIP(MaxWidth));

    SetClientSize(width, height);
    Center();
}

void QSPMsgDlg::OnLinkClicked(wxHtmlLinkEvent &event)
{
    if (const wxHtmlLinkInfo info{event.GetLinkInfo()}; info.GetEvent()->LeftUp())
    {
        if (wxString href = info.GetHref(); !href.empty() && href[0] == '#')
        {
            m_desc->ScrollToAnchor(href);
        } else
        {
            QSPTools::LaunchDefaultBrowser(href);
        }
    } else {
        event.Skip();
    }
}
