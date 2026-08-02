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

#include "updateappdialog.h"
#include "comtools.h"

wxIMPLEMENT_CLASS(UpdateAppDialog, wxDialog);

UpdateAppDialog::UpdateAppDialog(wxWindow *parent, const wxString &title,
    const wxString &newVersion, const wxString& releaseNotes, const wxString &updateUrl, const int style) :
    wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, style) // wxSize(400, 400)
{
    Bind(wxEVT_TEXT_URL, &UpdateAppDialog::OnTextUrl, this);

    auto* mainSizer = new wxBoxSizer{wxVERTICAL};

    auto* updateMessage = new wxStaticText{
        this,
        wxID_ANY,
        wxString::Format(_("A new version (%s) is available!"), newVersion)
    };

    wxFont updateMessageFont = updateMessage->GetFont();
    updateMessageFont.SetWeight(wxFONTWEIGHT_BOLD);
    updateMessageFont.SetPointSize(updateMessageFont.GetPointSize() + 2);
    updateMessage->SetFont(updateMessageFont);

    auto* link = new wxHyperlinkCtrl{
        this,
        wxID_ANY,
        _("Click here to download the update"), updateUrl
    };

    auto* releaseNotesLabel = new wxStaticText{
        this,
        wxID_ANY,
        _("Release notes:")
    };

    auto* releaseNotesText = new wxTextCtrl{
        this,
        wxID_ANY,
        releaseNotes,
        wxDefaultPosition,
        FromDIP(wxSize{380, 200}),
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxTE_AUTO_URL
    };

    auto* buttonSizer = new wxBoxSizer{wxHORIZONTAL};
    auto* updateBtn = new wxButton{this, wxID_OK, _("Get the latest version")};
    auto* laterBtn = new wxButton{this, wxID_CANCEL, _("Later")};

    buttonSizer->Add(updateBtn, 0, wxALL, FromDIP(5));
    buttonSizer->Add(laterBtn, 0, wxALL, FromDIP(5));

    mainSizer->Add(updateMessage, 0, wxLEFT | wxRIGHT | wxTOP | wxALIGN_CENTER, FromDIP(10));
    mainSizer->Add(link, 0, wxALL | wxALIGN_CENTER, FromDIP(5));
    mainSizer->Add(releaseNotesLabel, 0, wxALL, FromDIP(5));
    mainSizer->Add(releaseNotesText, 1, wxLEFT | wxRIGHT | wxEXPAND, FromDIP(5));
    mainSizer->Add(buttonSizer, 0, wxALL | wxALIGN_CENTER, FromDIP(5));

    SetSizerAndFit(mainSizer);
    SetAutoLayout(true);

    wxWindowBase::SetMinClientSize(FromDIP(wxSize{400, 350}));
}

void UpdateAppDialog::OnTextUrl(const wxTextUrlEvent &event)
{
    if (event.GetMouseEvent().LeftUp())
    {
        if (const auto* textCtrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject()))
        {
            const wxString fullText = textCtrl->GetValue();

            if (
                const wxString link = fullText.Mid(
                    event.GetURLStart(),
                    event.GetURLEnd() - event.GetURLStart()
                );
                !link.IsEmpty()
            ) {
                QSPTools::LaunchDefaultBrowser(link);
            }
        }
    }
}
