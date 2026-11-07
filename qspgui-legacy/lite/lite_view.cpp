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

#include "lite_view.h"
#include "../comtools.h"
#include <wx/dcbuffer.h>
#include <wx/html/htmlwin.h>
#include <algorithm>
#include <cmath>

namespace
{
    constexpr int ANIMATION_TICK_MS = 20;
    constexpr int PAGE_MARGIN = 5;
}

QSPLiteView::QSPLiteView(wxWindow *parent, const wxWindowID id)
    : wxScrolledCanvas(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE),
      m_container(*this)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetScrollRate(FromDIP(16), FromDIP(16));
    m_animationTimer.SetOwner(this);

    Bind(wxEVT_PAINT, &QSPLiteView::OnPaint, this);
    Bind(wxEVT_SIZE, &QSPLiteView::OnSize, this);
    Bind(wxEVT_MOTION, &QSPLiteView::OnMouseMove, this);
    Bind(wxEVT_LEFT_DOWN, &QSPLiteView::OnMouseDown, this);
    Bind(wxEVT_LEFT_UP, &QSPLiteView::OnMouseUp, this);
    Bind(wxEVT_LEAVE_WINDOW, &QSPLiteView::OnMouseLeave, this);
    Bind(wxEVT_MOUSEWHEEL, &QSPLiteView::OnMouseWheel, this);
    Bind(wxEVT_KEY_UP, &QSPLiteView::OnKeyUp, this);
    Bind(wxEVT_DPI_CHANGED, &QSPLiteView::OnDPIChanged, this);
    Bind(wxEVT_TIMER, &QSPLiteView::OnAnimationTimer, this);

    RefreshUI();
}

QSPLiteView::~QSPLiteView()
{
    m_animationTimer.Stop();
    m_document.reset();
}

double QSPLiteView::GetScale() const
{
    return FromDIP(1000) / 1000.0;
}

wxPoint QSPLiteView::ToDocument(const wxPoint &pos) const
{
    const wxPoint unscrolled = CalcUnscrolledPosition(pos);
    const double scale = GetScale();
    return {static_cast<int>(unscrolled.x / scale), static_cast<int>(unscrolled.y / scale)};
}

litehtml::position QSPLiteView::GetViewport() const
{
    const double scale = GetScale();
    const wxPoint origin = CalcUnscrolledPosition(wxPoint(0, 0));
    const wxSize size = GetClientSize();
    return {static_cast<float>(origin.x / scale), static_cast<float>(origin.y / scale),
            static_cast<float>(size.x / scale), static_cast<float>(size.y / scale)};
}

void QSPLiteView::SetPathProvider(PathProvider *provider)
{
    m_pathProvider = provider;
    m_container.SetPathProvider(provider);
}

void QSPLiteView::SetIsHtml(const bool isHtml)
{
    if (m_toUseHtml == isHtml) return;
    m_toUseHtml = isHtml;
    RefreshUI();
}

void QSPLiteView::SetText(const wxString &text, bool toScroll)
{
    if (m_text == text) return;

    if (toScroll && (m_text.IsEmpty() || !text.StartsWith(m_text))) toScroll = false;
    m_text = text;
    RefreshUI();

    Scroll(0, toScroll ? GetVirtualSize().y : 0);
}

void QSPLiteView::SetTextFont(const wxFont &font)
{
    if (m_font.GetFaceName().IsSameAs(font.GetFaceName(), false) && m_font.GetPointSize() == font.GetPointSize())
        return;
    m_font = font;
    RefreshUI();
}

void QSPLiteView::SetLinkColor(const wxColour &color)
{
    m_linkColor = color;
    RefreshUI();
}

void QSPLiteView::RefreshUI()
{
    m_container.SetDefaultFont(m_font.GetFaceName(), QSPLiteContainer::GetFontSizeInPixels(*this, m_font, GetScale()));

    const wxString style = wxString::Format(
        "body{margin:%dpx;color:#%s}"
        "a{color:#%s}"
        "a.plain{text-decoration:none}"
        "th{text-align:center}"
        "hr{color:gray}",
        PAGE_MARGIN, QSPTools::GetHexColor(GetForegroundColour()), QSPTools::GetHexColor(m_linkColor));
    const wxString body = QSPTools::HtmlizeWhitespaces(m_toUseHtml ? m_text : QSPTools::ProceedAsPlain(m_text), true);

    m_document = litehtml::document::createFromString(
        body.utf8_string(), &m_container, std::string(litehtml::master_css) + style.utf8_string());
    m_layoutWidth = -1;
    LayoutDocument();
    UpdateAnimationTimer();
    Refresh();
}

void QSPLiteView::LayoutDocument()
{
    const wxSize client = GetClientSize();
    if (!m_document || client.x <= 0) return;
    if (client.x == m_layoutWidth) return;
    m_layoutWidth = client.x;

    const double scale = GetScale();
    m_document->media_changed();
    m_document->render(static_cast<float>(client.x / scale));
    const int width = static_cast<int>(std::ceil(static_cast<float>(m_document->width()) * scale));
    const int height = static_cast<int>(std::ceil(static_cast<float>(m_document->height()) * scale));
    SetVirtualSize(std::max(width, client.x), height);
}

void QSPLiteView::LoadBackImage(const wxString &imagePath)
{
    if (m_imagePath == imagePath) return;
    m_imagePath = imagePath;
    m_backImage = wxNullBitmap;

    if (const wxString fullPath = m_pathProvider ? m_pathProvider->ComposeGamePath(imagePath) : imagePath;
        !imagePath.IsEmpty() && wxFileExists(fullPath))
    {
        wxLogNull noLog;
        if (wxImage image; image.LoadFile(fullPath)) m_backImage = wxBitmap(image);
    }
    UpdateBackImage();
    Refresh();
}

void QSPLiteView::SetBackImageSmoothing(const bool toSmooth)
{
    if (m_toSmoothBackImage == toSmooth) return;
    m_toSmoothBackImage = toSmooth;
    UpdateBackImage();
    Refresh();
}

void QSPLiteView::UpdateBackImage()
{
    m_scaledBackImage = wxNullBitmap;
    if (!m_backImage.IsOk()) return;

    const wxSize size = GetClientSize();
    const int w = std::max(size.x, 1), h = std::max(size.y, 1);
    const int srcW = m_backImage.GetWidth(), srcH = m_backImage.GetHeight();
    if (srcW <= 0 || srcH <= 0) return;

    int destW = srcW * h / srcH;
    int destH = srcH * w / srcW;
    if (destW > w)
        destW = w;
    else
        destH = h;
    m_backImagePos = wxPoint((w - destW) / 2, (h - destH) / 2);
    if (destW > 0 && destH > 0)
        m_scaledBackImage = wxBitmap(m_backImage.ConvertToImage().Scale(
            destW, destH, m_toSmoothBackImage ? wxIMAGE_QUALITY_HIGH : wxIMAGE_QUALITY_NORMAL));
}

wxSize QSPLiteView::GetContentSize() const
{
    if (!m_document) return {};
    const double scale = GetScale();
    return {static_cast<int>(std::ceil(static_cast<float>(m_document->width()) * scale)),
            static_cast<int>(std::ceil(static_cast<float>(m_document->height()) * scale))};
}

void QSPLiteView::ScrollToAnchor(const wxString &anchor)
{
    if (!m_document) return;
    const wxString name = anchor.StartsWith("#") ? anchor.Mid(1) : anchor;
    if (name.IsEmpty() || name.find_first_of("\"\\") != wxString::npos) return;

    const litehtml::element::ptr root = m_document->root();
    if (!root) return;
    const std::string quoted = "\"" + name.utf8_string() + "\"";
    litehtml::element::ptr element = root->select_one("[id=" + quoted + "]");
    if (!element) element = root->select_one("a[name=" + quoted + "]");
    if (!element) return;

    const litehtml::position pos = element->get_placement();
    int unitX, unitY;
    GetScrollPixelsPerUnit(&unitX, &unitY);
    if (unitY > 0) Scroll(-1, static_cast<int>(static_cast<float>(pos.y) * GetScale()) / unitY);
}

void QSPLiteView::OnLinkClicked(const wxString &href)
{
    CallAfter([this, href] {
        wxMouseEvent mouseEvent(wxEVT_LEFT_UP);
        wxHtmlLinkInfo info(href);
        info.SetEvent(&mouseEvent);
        wxHtmlLinkEvent event(GetId(), info);
        event.SetEventObject(this);
        HandleWindowEvent(event);
    });
}

void QSPLiteView::ScrollWindow([[maybe_unused]] int dx, [[maybe_unused]] int dy, [[maybe_unused]] const wxRect *rect)
{
    Refresh();
}

void QSPLiteView::OnCursorChanged(const bool isLink)
{
    SetCursor(isLink ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
}

void QSPLiteView::UpdateAnimationTimer()
{
    if (m_container.HasAnimations())
    {
        if (!m_animationTimer.IsRunning()) m_animationTimer.Start(ANIMATION_TICK_MS);
    }
    else
        m_animationTimer.Stop();
}

void QSPLiteView::OnAnimationTimer([[maybe_unused]] wxTimerEvent &event)
{
    if (m_container.AdvanceAnimations()) Refresh();
}

void QSPLiteView::OnPaint([[maybe_unused]] wxPaintEvent &event)
{
    wxAutoBufferedPaintDC dc(this);
    const std::unique_ptr<wxGraphicsContext> gc(QSPLiteContainer::GetRenderer()->CreateContextFromUnknownDC(dc));
    if (!gc) return;

    const wxSize size = GetClientSize();
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(GetBackgroundColour()));
    gc->DrawRectangle(0, 0, size.x, size.y);
    if (m_scaledBackImage.IsOk())
        gc->DrawBitmap(m_scaledBackImage, m_backImagePos.x, m_backImagePos.y, m_scaledBackImage.GetWidth(),
                       m_scaledBackImage.GetHeight());

    if (!m_document) return;

    const double scale = GetScale();
    const wxPoint origin = CalcUnscrolledPosition(wxPoint(0, 0));
    const wxRect update = GetUpdateRegion().GetBox();
    gc->Scale(scale, scale);
    const litehtml::position clip(static_cast<float>(update.x / scale), static_cast<float>(update.y / scale),
                                  static_cast<float>(update.width / scale + 1),
                                  static_cast<float>(update.height / scale + 1));
    m_document->draw(reinterpret_cast<litehtml::uint_ptr>(gc.get()), static_cast<float>(-origin.x / scale),
                     static_cast<float>(-origin.y / scale), &clip);
}

void QSPLiteView::OnSize(wxSizeEvent &event)
{
    event.Skip();
    LayoutDocument();
    UpdateBackImage();
    Refresh();
}

void QSPLiteView::OnMouseMove(wxMouseEvent &event)
{
    event.Skip();
    if (!m_document) return;
    const wxPoint pos = ToDocument(event.GetPosition());
    const double scale = GetScale();
    if (m_document->on_mouse_over(pos.x, pos.y, static_cast<float>(event.GetX() / scale),
                                  static_cast<float>(event.GetY() / scale), [](const litehtml::position &) {}))
        Refresh();
}

void QSPLiteView::OnMouseDown(wxMouseEvent &event)
{
    event.Skip();
    SetFocus();
    if (!m_document) return;
    const wxPoint pos = ToDocument(event.GetPosition());
    const double scale = GetScale();
    if (m_document->on_lbutton_down(pos.x, pos.y, static_cast<float>(event.GetX() / scale),
                                    static_cast<float>(event.GetY() / scale), [](const litehtml::position &) {}))
        Refresh();
}

void QSPLiteView::OnMouseUp(wxMouseEvent &event)
{
    event.Skip();
    if (!m_document) return;
    const wxPoint pos = ToDocument(event.GetPosition());
    const double scale = GetScale();
    if (m_document->on_lbutton_up(pos.x, pos.y, static_cast<float>(event.GetX() / scale),
                                  static_cast<float>(event.GetY() / scale), [](const litehtml::position &) {}))
        Refresh();
}

void QSPLiteView::OnMouseLeave(wxMouseEvent &event)
{
    event.Skip();
    if (m_document && m_document->on_mouse_leave([](const litehtml::position &) {})) Refresh();
}

void QSPLiteView::OnMouseWheel(wxMouseEvent &event)
{
    event.Skip();
    if (wxFindWindowAtPoint(wxGetMousePosition()) != this) event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
}

void QSPLiteView::OnKeyUp(wxKeyEvent &event)
{
    event.Skip();
    wxKeyEvent keyEvent{event};
    keyEvent.ResumePropagation(wxEVENT_PROPAGATE_MAX);
    TryAfter(keyEvent);
}

void QSPLiteView::OnDPIChanged(wxDPIChangedEvent &event)
{
    event.Skip();
    SetScrollRate(FromDIP(16), FromDIP(16));
    RefreshUI();
    UpdateBackImage();
}
