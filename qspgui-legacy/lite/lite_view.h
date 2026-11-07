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

#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <memory>
#include "lite_container.h"
#include "../htmlview.h"

class QSPLiteView : public wxScrolledCanvas, public QSPHtmlView, private QSPLiteHost
{
public:
    QSPLiteView(wxWindow *parent, wxWindowID id);
    ~QSPLiteView() override;

    // QSPHtmlView
    [[nodiscard]] wxWindow *GetWindow() override { return this; }
    [[nodiscard]] const wxWindow *GetWindow() const override { return this; }
    void SetPathProvider(PathProvider *provider) override;
    void SetIsHtml(bool isHtml) override;
    [[nodiscard]] bool IsHtml() const override { return m_toUseHtml; }
    void SetText(const wxString &text, bool toScroll = false) override;
    [[nodiscard]] wxString GetText() const override { return m_text; }
    void SetTextFont(const wxFont &font) override;
    [[nodiscard]] wxFont GetTextFont() const override { return m_font; }
    void SetLinkColor(const wxColour &color) override;
    [[nodiscard]] const wxColour &GetLinkColor() const override { return m_linkColor; }
    void LoadBackImage(const wxString &imagePath) override;
    [[nodiscard]] wxString GetBackImagePath() const override { return m_imagePath; }
    void ScrollToAnchor(const wxString &anchor) override;
    void RefreshUI() override;

private:
    // QSPLiteHost
    void OnLinkClicked(const wxString &href) override;
    void OnCursorChanged(bool isLink) override;
    [[nodiscard]] litehtml::position GetViewport() const override;

    [[nodiscard]] double GetScale() const;
    [[nodiscard]] double GetFontSizeInPixels() const;
    [[nodiscard]] wxPoint ToDocument(const wxPoint &pos) const;
    void LayoutDocument();
    void UpdateBackImage();
    void UpdateAnimationTimer();

    void OnPaint(wxPaintEvent &event);
    void OnSize(wxSizeEvent &event);
    void OnMouseMove(wxMouseEvent &event);
    void OnMouseDown(wxMouseEvent &event);
    void OnMouseUp(wxMouseEvent &event);
    void OnMouseLeave(wxMouseEvent &event);
    void OnMouseWheel(wxMouseEvent &event);
    void OnKeyUp(wxKeyEvent &event);
    void OnDPIChanged(wxDPIChangedEvent &event);
    void OnAnimationTimer(wxTimerEvent &event);

    QSPLiteContainer m_container;
    litehtml::document::ptr m_document;
    PathProvider *m_pathProvider{nullptr};
    bool m_toUseHtml{false};
    wxString m_text;
    wxFont m_font{*wxNORMAL_FONT};
    wxColour m_linkColor{0, 0, 0xFF};
    int m_layoutWidth{-1};
    wxString m_imagePath;
    wxBitmap m_backImage;
    wxBitmap m_scaledBackImage;
    wxPoint m_backImagePos;
    wxTimer m_animationTimer;
};
