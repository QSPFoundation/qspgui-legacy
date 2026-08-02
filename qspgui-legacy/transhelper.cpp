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

#include "transhelper.h"

#include <utility>

QSPTranslationHelper::QSPTranslationHelper(wxString appName, wxString path)
    : m_appName(std::move(appName)), m_path(std::move(path))
{
}

void QSPTranslationHelper::Load(const wxConfigBase &config, const wxString &key)
{
    wxString langName;
    config.Read(key, &langName, wxEmptyString);

    if (langName.IsEmpty())
    {
        UpdateLocale(wxLANGUAGE_DEFAULT);
    }
    else
    {
        if (const wxLanguageInfo* langInfo = wxLocale::FindLanguageInfo(langName)) {
            UpdateLocale(langInfo->Language);
        } else {
            UpdateLocale(wxLANGUAGE_DEFAULT);
        }
    }
}

void QSPTranslationHelper::Save(wxConfigBase &config, const wxString &key) const
{
    config.Write(key, m_locale ? m_locale->GetCanonicalName() : wxString{});
}

bool QSPTranslationHelper::AskUserForLanguage()
{
    wxArrayString names;
    wxArrayInt identifiers;

    names.Add(_("Default"));
    identifiers.Add(wxLANGUAGE_DEFAULT);

    wxDir dir{m_path};
    if (dir.IsOpened())
    {
        wxString filename;
        bool hasFiles = dir.GetFirst(&filename, "*", wxDIR_DEFAULT);
        while (hasFiles)
        {
            if (const wxLanguageInfo* langinfo = wxLocale::FindLanguageInfo(filename))
            {
                names.Add(langinfo->Description);
                identifiers.Add(langinfo->Language);
            }
            hasFiles = dir.GetNext(&filename);
        }
    }

    if (
        const int index = wxGetSingleChoiceIndex(_("Select language"), _("Language"), names);
        index >= 0
    )
    {
        UpdateLocale(identifiers[index]);
        return true;
    }

    return false;
}

void QSPTranslationHelper::UpdateLocale(const int lang)
{
    m_locale = std::make_unique<wxLocale>();
    m_locale->Init(lang);

    wxLocale::AddCatalogLookupPathPrefix(m_path);

    if (!m_locale->AddCatalog(m_appName))
    {
        m_locale->AddCatalog(m_appName + "_" + m_locale->GetCanonicalName().BeforeFirst('_'));
    }
}
