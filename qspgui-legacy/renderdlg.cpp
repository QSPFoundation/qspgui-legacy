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

#include "renderdlg.h"
#include "htmlview.h"

QSPRenderDlg::QSPRenderDlg(wxWindow *parent, const wxString &mainDescRenderer, const bool toSmoothBackImage)
    : wxDialog(parent, wxID_ANY, _("HTML renderer"))
{
    wxArrayString names;
    m_renderers.Add(RENDERER_WXHTML);
    names.Add(_("wxHTML (classic)"));
#if QSP_LITEHTML
    m_renderers.Add(RENDERER_LITEHTML);
    names.Add(_("litehtml (HTML5 and CSS)"));
#endif

    auto *grid = new wxFlexGridSizer(2, FromDIP(wxSize(8, 8)));
    grid->AddGrowableCol(1);
    grid->Add(new wxStaticText(this, wxID_ANY, _("Main description:")), 0, wxALIGN_CENTER_VERTICAL);
    m_mainDesc = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, names);
    const int selected = m_renderers.Index(mainDescRenderer);
    m_mainDesc->SetSelection(selected == wxNOT_FOUND ? 0 : selected);
    grid->Add(m_mainDesc, 1, wxEXPAND);
    m_mainDesc->Bind(wxEVT_CHOICE, [this](wxCommandEvent &) { UpdateControls(); });

    m_smoothBackImage = new wxCheckBox(this, wxID_ANY, _("Smooth the enlarged background image (as in browsers)"));
    m_smoothBackImage->SetValue(toSmoothBackImage);

    auto *note = new wxStaticText(this, wxID_ANY,
        _("The other panes use wxHTML. Games are made for wxHTML: some may look different with litehtml."));
    note->Wrap(FromDIP(360));

    auto *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(grid, 0, wxEXPAND | wxALL, FromDIP(10));
    sizer->Add(m_smoothBackImage, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));
    sizer->Add(note, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    sizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, FromDIP(10));
    SetSizerAndFit(sizer);
    UpdateControls();
    CentreOnParent();
}

wxString QSPRenderDlg::GetMainDescRenderer() const
{
    const int selected = m_mainDesc->GetSelection();
    return selected == wxNOT_FOUND ? wxString(RENDERER_WXHTML) : m_renderers[selected];
}

bool QSPRenderDlg::ToSmoothBackImage() const
{
    return m_smoothBackImage->GetValue();
}

void QSPRenderDlg::UpdateControls()
{
    m_smoothBackImage->Enable(GetMainDescRenderer() != RENDERER_WXHTML);
}
