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

#include "app.h"
#include "comtools.h"

wxIMPLEMENT_APP(QSPLegacyApp); // NOLINT

bool QSPLegacyApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    wxLog::EnableLogging(true);
    delete wxLog::SetActiveTarget(new wxLogStderr());

    wxInitAllImageHandlers();
    QSPInit();
    InitUI();
    return true;
}

int QSPLegacyApp::OnExit()
{
    QSPTerminate();
    QSPCallbacks::DeInit();
    wxTheClipboard->Flush();
    return wxApp::OnExit();
}

void QSPLegacyApp::OnInitCmdLine(wxCmdLineParser &parser)
{
    wxApp::OnInitCmdLine(parser);

    parser.AddParam("game file",
                    wxCMD_LINE_VAL_STRING,
                    wxCMD_LINE_PARAM_OPTIONAL);
}

bool QSPLegacyApp::OnCmdLineParsed(wxCmdLineParser &parser)
{
    if (!wxApp::OnCmdLineParsed(parser))
        return false;

    if (parser.GetParamCount() > 0)
        m_gameFile = parser.GetParam();

    return true;
}

void QSPLegacyApp::InitUI()
{
    wxString configPath = QSPTools::GetAppPath(wxEmptyString, QSP_CONFIG);
    if (!wxFileExists(configPath) && !wxFileName::IsDirWritable(QSPTools::GetAppPath()))
        configPath = QSPTools::GetConfigPath(wxEmptyString, QSP_CONFIG);

    wxString langsPath = QSPTools::GetResourcePath(QSP_TRANSLATIONS);
    m_transHelper = std::make_unique<QSPTranslationHelper>(QSP_APPNAME, langsPath);

    auto * frame = new QSPFrame(configPath, m_transHelper.get());
    QSPCallbacks::Init(frame);
    frame->LoadSettings();
    frame->EnableControls(false);

    if (auto bootFile = GetAutoRunPath(); bootFile.has_value())
    {
        frame->CallAfter(&QSPFrame::OnInit, bootFile.value());
    }
    else
    {
        if (frame->ToCheckUpdates())
            frame->CheckLatestVersion(UPDATE_SHOW_ONLY_NEW);
    }
}

std::optional<wxString> QSPLegacyApp::GetAutoRunPath() const
{
    if (!m_gameFile.IsEmpty())
    {
        wxFileName path{m_gameFile};
        path.MakeAbsolute();
        return path.GetFullPath();
    }

    wxFileName autoPath{"auto.qsp"};
    autoPath.MakeAbsolute();

    if (wxString autoPathString = autoPath.GetFullPath(); wxFileExists(autoPathString))
    {
        return autoPathString;
    }

    return std::nullopt;
}
