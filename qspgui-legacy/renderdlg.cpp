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

QSPRenderDlg::QSPRenderDlg(wxWindow *parent, const std::vector<wxString> &renderers, const bool toSmoothBackImage)
    : wxDialog(parent, wxID_ANY, _("HTML renderer"))
{
    wxArrayString names;
    m_renderers.Add(RENDERER_WXHTML);
    names.Add(_("wxHTML (classic)"));
#if QSP_LITEHTML
    m_renderers.Add(RENDERER_LITEHTML);
    names.Add(_("litehtml (HTML5 and CSS)"));
#endif

    const wxString labels[RENDER_PANES] =
    {
        _("Main description:"),
        _("Additional description:"),
        _("Actions:"),
        _("Objects:"),
        _("Dialogs:")
    };

    auto *grid = new wxFlexGridSizer(2, FromDIP(wxSize(8, 8)));
    grid->AddGrowableCol(1);
    for (size_t pane = 0; pane < RENDER_PANES; ++pane)
    {
        grid->Add(new wxStaticText(this, wxID_ANY, labels[pane]), 0, wxALIGN_CENTER_VERTICAL);
        auto *choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, names);
        const int selected = pane < renderers.size() ? m_renderers.Index(renderers[pane]) : wxNOT_FOUND;
        choice->SetSelection(selected == wxNOT_FOUND ? 0 : selected);
        choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent &) { UpdateControls(); });
        grid->Add(choice, 1, wxEXPAND);
        m_choices[pane] = choice;
    }

    m_smoothBackImage = new wxCheckBox(this, wxID_ANY, _("Smooth the enlarged background image (as in browsers)"));
    m_smoothBackImage->SetValue(toSmoothBackImage);

    auto *note = new wxStaticText(this, wxID_ANY,
        _("Games are made for wxHTML: some may look different with litehtml."));
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

wxString QSPRenderDlg::GetRenderer(const RenderPane pane) const
{
    const int selected = m_choices[static_cast<size_t>(pane)]->GetSelection();
    return selected == wxNOT_FOUND ? wxString(RENDERER_WXHTML) : m_renderers[selected];
}

std::vector<wxString> QSPRenderDlg::GetRenderers() const
{
    std::vector<wxString> renderers;
    for (size_t pane = 0; pane < RENDER_PANES; ++pane)
        renderers.push_back(GetRenderer(static_cast<RenderPane>(pane)));
    return renderers;
}

bool QSPRenderDlg::ToSmoothBackImage() const
{
    return m_smoothBackImage->GetValue();
}

void QSPRenderDlg::UpdateControls()
{
    m_smoothBackImage->Enable(GetRenderer(RenderPane::MainDesc) != RENDERER_WXHTML);
}
