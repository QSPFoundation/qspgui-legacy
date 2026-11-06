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

#include "animation.h"
#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/filesys.h>
#include <wx/html/htmlcell.h>
#include <wx/html/htmlwin.h>
#include <wx/html/m_templ.h>
#include <wx/html/styleparams.h>
#include <algorithm>
#include <cmath>
#include <memory>

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

    class QSPImageCell;

    class QSPImageTimer : public wxTimer
    {
    public:
        explicit QSPImageTimer(QSPImageCell *cell) : m_cell(cell) {}
        void Notify() override;

    private:
        QSPImageCell *m_cell;
    };

    class QSPImageCell : public wxHtmlCell
    {
    public:
        QSPImageCell(const wxHtmlTag &tag, wxHtmlWindowInterface *windowIface, wxFSFile *input, double scaleHDPI,
                     int width, bool isWidthPercent, int height, double scale, int align, const wxString &mapName) :
            wxHtmlCell(tag), m_windowIface(windowIface), m_timer(this), m_scaleHDPI(scaleHDPI),
            m_width(width), m_isWidthPercent(isWidthPercent), m_height(height), m_isHeightPresent(height != wxDefaultCoord),
            m_scale(scale), m_align(align), m_mapName(mapName)
        {
            SetCanLiveOnPagebreak(false);
            // Pages on the web use zero-sized images as spacers
            if (m_width == 0 || m_height == 0) return;

            if (wxInputStream *stream = input ? input->GetStream() : nullptr)
            {
                const wxFileOffset start = stream->TellI();
                if (m_windowIface && m_animation.Load(*stream))
                {
                    m_image = m_animation.GetFrame();
                    m_timer.StartOnce(m_animation.GetDelay());
                }
                else if (stream->SeekI(start) != wxInvalidOffset)
                {
                    m_image.LoadFile(*stream, wxBITMAP_TYPE_ANY);
                }
            }
            if (!m_image.IsOk())
            {
                m_image = wxArtProvider::GetBitmap(wxART_MISSING_IMAGE).ConvertToImage();
                m_scaleHDPI = 1.0;
            }

            const int naturalWidth = std::max(1, (int)(m_image.GetWidth() / m_scaleHDPI));
            const int naturalHeight = std::max(1, (int)(m_image.GetHeight() / m_scaleHDPI));
            m_naturalSize = wxSize(naturalWidth, naturalHeight);
            if (m_width == wxDefaultCoord && m_height == wxDefaultCoord)
            {
                m_width = naturalWidth;
                m_height = naturalHeight;
            }
            else if (m_width == wxDefaultCoord)
                m_width = naturalWidth * m_height / naturalHeight;
            else if (m_height == wxDefaultCoord && !m_isWidthPercent)
                m_height = naturalHeight * m_width / naturalWidth;
        }

        void SetAlt(const wxString &alt) { m_alt = alt; }
        wxString ConvertToText(wxHtmlSelection *) const override { return m_alt; }

        void Layout(int w) override
        {
            if (m_isWidthPercent)
            {
                m_Width = w * m_width / 100;
                m_Height = m_isHeightPresent ? (int)(m_scale * m_height) : m_naturalSize.y * m_Width / std::max(1, m_naturalSize.x);
            }
            else
            {
                m_Width = static_cast<int>(m_scale * m_width);
                m_Height = static_cast<int>(m_scale * m_height);
            }
            m_Width = std::max(m_Width, 0);
            m_Height = std::max(m_Height, 0);

            switch (m_align)
            {
            case wxHTML_ALIGN_TOP: m_Descent = m_Height; break;
            case wxHTML_ALIGN_CENTER: m_Descent = m_Height / 2; break;
            default: m_Descent = 0; break;
            }

            UpdateBitmap();
            wxHtmlCell::Layout(w);
        }

        void Draw(wxDC &dc, int x, int y, int, int, wxHtmlRenderingInfo &) override
        {
            if (m_bitmap.IsOk())
                dc.DrawBitmap(m_bitmap, x + m_PosX, y + m_PosY, true);
        }

        wxHtmlLinkInfo *GetLink(int x = 0, int y = 0) const override
        {
            if (m_mapName.IsEmpty()) return wxHtmlCell::GetLink(x, y);
            if (!m_imageMap)
            {
                const wxHtmlCell *root = this;
                while (root->GetParent()) root = root->GetParent();
                m_imageMap = root->Find(wxHTML_COND_ISIMAGEMAP, &m_mapName);
                if (!m_imageMap)
                {
                    m_mapName.Clear();
                    return wxHtmlCell::GetLink(x, y);
                }
            }
            return m_imageMap->GetLink(x, y);
        }

        void OnTimer()
        {
            m_animation.Advance();
            m_image = m_animation.GetFrame();
            if (m_windowIface && m_Width > 0 && m_Height > 0)
            {
                const wxRect rect(m_windowIface->HTMLCoordsToWindow(this, GetAbsPos()), wxSize(m_Width, m_Height));
                wxWindow *window = m_windowIface->GetHTMLWindow();
                if (window && window->GetClientRect().Intersects(rect))
                {
                    UpdateBitmap();
                    window->RefreshRect(rect);
                }
            }
            m_timer.StartOnce(m_animation.GetDelay());
        }

    private:
        void UpdateBitmap()
        {
            if (!m_image.IsOk() || m_Width <= 0 || m_Height <= 0)
            {
                m_bitmap = wxNullBitmap;
                return;
            }
            // Scale once here instead of on every paint; keep the extra pixels of @2x images
            const int width = std::max(1, static_cast<int>(std::lround(m_Width * m_scaleHDPI)));
            const int height = std::max(1, static_cast<int>(std::lround(m_Height * m_scaleHDPI)));
            const wxImage image = m_image.GetWidth() == width && m_image.GetHeight() == height
                ? m_image
                : m_image.Scale(width, height, wxIMAGE_QUALITY_HIGH);
            m_bitmap = wxBitmap(image, -1, m_scaleHDPI);
        }

        wxHtmlWindowInterface *m_windowIface;
        QSPAnimation m_animation;
        QSPImageTimer m_timer;
        wxImage m_image;   // the image or the current frame at its own size
        wxBitmap m_bitmap; // the same at the size of the cell
        wxSize m_naturalSize;
        double m_scaleHDPI;
        int m_width;
        bool m_isWidthPercent;
        int m_height;
        bool m_isHeightPresent;
        double m_scale;
        int m_align;
        mutable wxString m_mapName;
        mutable const wxHtmlCell *m_imageMap{nullptr};
        wxString m_alt;
    };

    void QSPImageTimer::Notify()
    {
        m_cell->OnTimer();
    }

    class QSPImageTagHandler : public wxHtmlWinTagHandler
    {
    public:
        wxString GetSupportedTags() override { return "IMG"; }

        bool HandleTag(const wxHtmlTag &tag) override
        {
            wxString src;
            if (!tag.GetParamAsString("SRC", &src)) return false;

            wxFSFile *input = nullptr;
            double scaleHDPI = 1.0;
#if defined(__WXOSX_COCOA__)
            // Prefer the 2x version of the image on Retina screens
            wxWindow *window = m_WParser->GetWindowInterface() ? m_WParser->GetWindowInterface()->GetHTMLWindow() : nullptr;
            if (!window) window = wxApp::GetMainTopWindow();
            if (window && window->GetContentScaleFactor() > 1.0 && src.Find('.') != wxNOT_FOUND)
            {
                input = m_WParser->OpenURL(wxHTML_URL_IMAGE, src.BeforeLast('.') + "@2x." + src.AfterLast('.'));
                if (input) scaleHDPI = 2.0;
            }
#endif
            if (!input) input = m_WParser->OpenURL(wxHTML_URL_IMAGE, src);
            const std::unique_ptr<wxFSFile> inputHolder(input);

            int width = wxDefaultCoord, height = wxDefaultCoord;
            bool isWidthPercent = false;
            if (tag.GetParamAsIntOrPercent("WIDTH", &width, isWidthPercent) && isWidthPercent)
                width = std::clamp(width, 0, 100);
            if (!tag.GetParamAsInt("HEIGHT", &height))
                height = wxDefaultCoord;

            int align = wxHTML_ALIGN_BOTTOM;
            wxString alignValue;
            if (tag.GetParamAsString("ALIGN", &alignValue))
            {
                alignValue.MakeUpper();
                if (alignValue == "TEXTTOP")
                    align = wxHTML_ALIGN_TOP;
                else if (alignValue == "CENTER" || alignValue == "ABSCENTER")
                    align = wxHTML_ALIGN_CENTER;
            }

            wxString mapName;
            if (tag.GetParamAsString("USEMAP", &mapName) && mapName.StartsWith("#"))
                mapName.Remove(0, 1);

            auto *cell = new QSPImageCell(tag, m_WParser->GetWindowInterface(), input, scaleHDPI,
                width, isWidthPercent, height, m_WParser->GetPixelScale(), align, mapName);
            m_WParser->ApplyStateToCell(cell);
            m_WParser->StopCollapsingSpaces();
            cell->SetAlt(tag.GetParam("ALT"));
            m_WParser->GetContainer()->InsertCell(cell);

            return false;
        }
    };
}

class QSPHtmlTagsModule : public wxHtmlTagsModule
{
public:
    QSPHtmlTagsModule()
    {
        AddDependency("wxHTML_ModuleLinks");
        AddDependency("wxHTML_ModuleImage");
    }

    void FillHandlersTable(wxHtmlWinParser *parser) override
    {
        parser->AddTagHandler(new QSPLinkTagHandler);
        parser->AddTagHandler(new QSPImageTagHandler);
    }

private:
    wxDECLARE_DYNAMIC_CLASS(QSPHtmlTagsModule);
};

wxIMPLEMENT_DYNAMIC_CLASS(QSPHtmlTagsModule, wxHtmlTagsModule);
