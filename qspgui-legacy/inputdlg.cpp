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

#include "inputdlg.h"
#include "comtools.h"
#include <algorithm>

wxIMPLEMENT_CLASS(QSPInputDlg, wxDialog);

QSPInputDlg::QSPInputDlg(wxWindow* parent,
                         const wxWindowID id,
                         const wxColour& backColor,
                         const wxColour& fontColor,
                         const wxFont& font,
                         const wxString& caption,
                         const wxString& text,
                         const bool isHtml,
                         PathProvider *pathProvider)
{
    if (!Create(parent, id, caption, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU)) return;

    Bind(wxEVT_HTML_LINK_CLICKED, &QSPInputDlg::OnLinkClicked, this, ID_INPUT_DESC);
    Bind(wxEVT_INIT_DIALOG, &QSPInputDlg::OnInitDialog, this);

    wxWindowBase::SetBackgroundColour(backColor);

    auto* sizerUp = new wxBoxSizer{wxVERTICAL};

    m_desc = new QSPTextBox{this, ID_INPUT_DESC};
    m_desc->SetPathProvider(pathProvider);
    m_desc->SetIsHtml(isHtml);
    m_desc->SetBackgroundColour(backColor);
    m_desc->SetForegroundColour(fontColor);
    m_desc->SetTextFont(font);
    m_desc->SetText(text);

    auto* inputStr = new wxTextCtrl{this, wxID_ANY};
    inputStr->SetBackgroundColour(backColor);
    inputStr->SetForegroundColour(fontColor);
    inputStr->SetFont(font);

    auto* line = new wxStaticLine{this, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL};

    sizerUp->Add(m_desc, 1, wxALL | wxGROW, FromDIP(2));
    sizerUp->Add(inputStr, 0, wxALL | wxGROW, FromDIP(2));
    sizerUp->Add(line, 0, wxALL | wxGROW, FromDIP(2));

    auto* sizerBottom = new wxBoxSizer{wxHORIZONTAL};
    auto* btnOk = new wxButton{this, wxID_OK, _("OK")};
    auto* btnCancel = new wxButton{this, wxID_CANCEL, _("Cancel")};

    btnOk->SetDefault();
    btnOk->SetFont(font);
    btnOk->SetBackgroundColour(backColor);
    btnOk->SetForegroundColour(fontColor);

    btnCancel->SetFont(font);
    btnCancel->SetBackgroundColour(backColor);
    btnCancel->SetForegroundColour(fontColor);

    sizerBottom->Add(btnOk, 0, wxALL, FromDIP(2));
    sizerBottom->Add(btnCancel, 0, wxALL, FromDIP(2));

    auto* sizerMain = new wxBoxSizer{wxVERTICAL};
    sizerMain->Add(sizerUp, 1, wxGROW, 0);
    sizerMain->Add(sizerBottom, 0, wxALIGN_RIGHT, 0);

    inputStr->SetValidator(wxGenericValidator(&m_text));

    sizerMain->SetMinSize(FromDIP(wxSize{MinWidth, MinHeight}));
    SetSizerAndFit(sizerMain);
    inputStr->SetFocus();
}

void QSPInputDlg::OnInitDialog([[maybe_unused]] wxInitDialogEvent& event)
{
    const int deltaH = GetClientSize().GetHeight() - m_desc->GetSize().GetHeight();
    const int deltaW = GetClientSize().GetWidth() - m_desc->GetSize().GetWidth();

    int height = m_desc->GetInternalRepresentation()->GetHeight() + m_desc->GetCharHeight() + deltaH;
    int width = m_desc->GetInternalRepresentation()->GetWidth() + deltaW;

    height = std::clamp(height, FromDIP(MinHeight), FromDIP(MaxHeight));
    width = std::clamp(width, FromDIP(MinWidth), FromDIP(MaxWidth));

    SetClientSize(width, height);
    Center();
}

void QSPInputDlg::OnLinkClicked(wxHtmlLinkEvent& event)
{
    if (const wxHtmlLinkInfo info{event.GetLinkInfo()}; info.GetEvent()->LeftUp())
    {
        if (wxString href = info.GetHref(); !href.empty() && href[0] == '#')
            m_desc->LoadPage(href);
        else
            QSPTools::LaunchDefaultBrowser(href);
    }
    else
    {
        event.Skip();
    }
}
