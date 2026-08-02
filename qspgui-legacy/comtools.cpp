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

#include "comtools.h"

void QSPTools::LaunchDefaultBrowser(const wxString &url)
{
    const wxURI uri{url};

    const bool canOpen = uri.HasScheme()
                             ? uri.GetScheme() == "http" || uri.GetScheme() == "https" || uri.GetScheme() == "mailto"
                             : !wxFileExists(url) && !wxDirExists(url);

    if (canOpen)
        wxLaunchDefaultBrowser(url);
}

wxString QSPTools::GetHexColor(const wxColour &color)
{
    return wxString::Format("%.2X%.2X%.2X",
                            static_cast<int>(color.Red()),
                            static_cast<int>(color.Green()),
                            static_cast<int>(color.Blue()));
}

wxString QSPTools::HtmlizeWhitespaces(const wxString &str)
{
    wxString out;
    out.reserve(str.length() * 1.2);

    size_t linepos = 0;
    bool isLastSpace = true;

    auto appendSpace = [&]
    {
        out << (isLastSpace ? "&nbsp;" : " ");
        isLastSpace = !isLastSpace;
    };

    for (auto i = str.begin(); i != str.end(); ++i)
    {
        if (const wxChar ch = *i; ch == '<')
        {
            out << ch;
            wxChar quote = 0;
            ++i;

            while (i != str.end())
            {
                wxChar inner_ch = *i;
                if (quote)
                {
                    if (inner_ch == '\\')
                    {
                        if (++i == str.end()) break;

                        inner_ch = *i;
                        if (inner_ch == quote)
                        {
                            out << (inner_ch == '"' ? "&quot;" : "&apos;");
                            ++i;
                            continue;
                        }
                        out << '\\';
                    }

                    if (inner_ch == '&') out << "&amp;";
                    else if (inner_ch == '<') out << "&lt;";
                    else if (inner_ch == '>') out << "&gt;";
                    else
                    {
                        if (inner_ch == quote) quote = 0;
                        out << inner_ch;
                    }
                } else
                {
                    out << inner_ch;
                    if (inner_ch == '>') break;
                    if (inner_ch == '"' || inner_ch == '\'') quote = inner_ch;
                }
                ++i;
            }

            if (i == str.end()) return out;
            isLastSpace = true;
        } else if (ch == ' ')
        {
            appendSpace();
            ++linepos;
        } else if (ch == '\r')
        {
            // ignore \r
        } else if (ch == '\n')
        {
            out << "<br />";
            isLastSpace = true;
            linepos = 0;
        } else if (ch == '\t')
        {
            const size_t spaces = 4 - (linepos % 4);
            for (size_t j = 0; j < spaces; ++j)
            {
                appendSpace();
            }
            linepos += spaces;
        } else
        {
            out << ch;
            isLastSpace = false;
            ++linepos;
        }
    }

    return out;
}

wxString QSPTools::ProceedAsPlain(const wxString &str)
{
    wxString out;
    out.reserve(str.length() * 1.1);

    for (const wxChar ch : str)
    {
        if (ch == '<') out << "&lt;";
        else if (ch == '>') out << "&gt;";
        else if (ch == '&') out << "&amp;";
        else out << ch;
    }

    return out;
}

wxString QSPTools::GetAppPath(const wxString &path, const wxString &file)
{
    wxFileName appFullPath{wxStandardPaths::Get().GetExecutablePath()};
    wxFileName appPath{appFullPath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + path, file};
    return appPath.GetFullPath();
}

wxString QSPTools::GetResourcePath(const wxString &path, const wxString &file)
{
    wxPathList resourcePathList;
    resourcePathList.AddEnvList("XDG_DATA_DIRS");
    resourcePathList.Add(wxStandardPaths::Get().GetResourcesDir());

    wxArrayString prefixes;
    prefixes.Add(QSP_APPNAME);
    prefixes.Add(wxEmptyString);

    for (const wxString& resDir : resourcePathList)
    {
        for (const wxString& prefix : prefixes)
        {
            wxFileName resourcePath{resDir, file};

            if (!prefix.IsEmpty())
                resourcePath.Assign(resourcePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + prefix, file);

            if (!path.IsEmpty())
                resourcePath.Assign(resourcePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + path, file);

            if (resourcePath.Exists())
                return resourcePath.GetFullPath();
        }
    }

    return GetAppPath(path, file);
}

wxString QSPTools::GetConfigPath(const wxString &path, const wxString &file)
{
    wxFileName configPath{wxStandardPaths::Get().GetUserDir(wxStandardPathsBase::Dir_Config), file};

    if (!path.IsEmpty())
        configPath.Assign(configPath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + path, file);

    return configPath.GetFullPath();
}

wxString QSPTools::GetPlatform()
{
    const wxOperatingSystemId osId = wxPlatformInfo::Get().GetOperatingSystemId();

    if (osId & wxOS_WINDOWS) return "Windows";
    if (osId & wxOS_MAC) return "MacOS";
    if (osId & wxOS_UNIX_LINUX) return "Linux";
    if (osId & wxOS_UNIX) return "Unix";

    return "Unknown";
}

wxString QSPTools::GetVersion(const wxString &libVersion)
{
    return wxString::Format("%s (classic)", libVersion);
}
