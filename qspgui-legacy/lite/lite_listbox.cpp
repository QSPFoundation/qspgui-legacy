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

#include "lite_listbox.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr int ANIMATION_TICK_MS = 20;
}

QSPLiteListBox::QSPLiteListBox(wxWindow *parent, const wxWindowID id, const ListBoxType type)
    : QSPListBoxBase(parent, id, type), m_container(*this)
{
    m_animationTimer.SetOwner(this);
    Bind(wxEVT_SIZE, &QSPLiteListBox::OnSize, this);
    Bind(wxEVT_DPI_CHANGED, &QSPLiteListBox::OnDPIChanged, this);
    Bind(wxEVT_TIMER, &QSPLiteListBox::OnAnimationTimer, this);
    RefreshUI();
}

QSPLiteListBox::~QSPLiteListBox()
{
    m_animationTimer.Stop();
    m_items.clear();
}

void QSPLiteListBox::SetPathProvider(PathProvider *provider)
{
    QSPListBoxBase::SetPathProvider(provider);
    m_container.SetPathProvider(provider);
}

litehtml::position QSPLiteListBox::GetViewport() const
{
    return {0.0f, 0.0f, GetLayoutWidth(), static_cast<float>(GetClientSize().y / GetScale())};
}

float QSPLiteListBox::GetLayoutWidth() const
{
    return static_cast<float>(std::max(GetClientSize().x, 1) / GetScale());
}

void QSPLiteListBox::OnContentsChanged()
{
    m_items.clear();
    m_container.SetDefaultFont(m_font.GetFaceName(), QSPLiteContainer::GetFontSizeInPixels(*this, m_font, GetScale()));
}

litehtml::document *QSPLiteListBox::GetDocument(const size_t n, const bool isSelected) const
{
    if (n >= GetDescs().GetCount()) return nullptr;
    if (m_items.size() != GetDescs().GetCount()) m_items.assign(GetDescs().GetCount(), Item());

    litehtml::document::ptr &document = isSelected ? m_items[n].selected : m_items[n].normal;
    if (!document)
    {
        const wxColour color = isSelected ? wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT) : GetForegroundColour();
        const wxString style = wxString::Format(
            "body{margin:0;color:#%s}"
            "a{color:#%s}"
            "a.plain{text-decoration:none}"
            "th{text-align:center}"
            "hr{color:gray}",
            QSPTools::GetHexColor(color), QSPTools::GetHexColor(isSelected ? color : m_linkColor));
        const wxString html = "<table cellspacing=\"4\" cellpadding=\"0\"><tr>" + GetItemCells(n, true) + "</tr></table>";
        document = litehtml::document::createFromString(
            html.utf8_string(), &m_container, std::string(litehtml::master_css) + style.utf8_string());
        document->render(GetLayoutWidth());
    }
    return document.get();
}

wxCoord QSPLiteListBox::OnMeasureItem(const size_t n) const
{
    const litehtml::document *document = GetDocument(n, false);
    if (!document) return 1;
    return std::max(static_cast<wxCoord>(std::ceil(static_cast<float>(document->height()) * GetScale())), 1);
}

void QSPLiteListBox::OnDrawItem(wxDC &dc, const wxRect &rect, const size_t n) const
{
    litehtml::document *document = GetDocument(n, IsSelected(n));
    if (!document) return;

    const std::unique_ptr<wxGraphicsContext> gc(QSPLiteContainer::GetRenderer()->CreateContextFromUnknownDC(dc));
    if (!gc) return;

    const double scale = GetScale();
    gc->Clip(rect.x, rect.y, rect.width, rect.height);
    gc->Translate(rect.x, rect.y);
    gc->Scale(scale, scale);
    const litehtml::position clip(0.0f, 0.0f, static_cast<float>(rect.width / scale + 1),
                                  static_cast<float>(rect.height / scale + 1));
    document->draw(reinterpret_cast<litehtml::uint_ptr>(gc.get()), 0.0f, 0.0f, &clip);

    if (m_container.HasAnimations() && !m_animationTimer.IsRunning())
        const_cast<wxTimer &>(m_animationTimer).Start(ANIMATION_TICK_MS);
}

void QSPLiteListBox::OnSize(wxSizeEvent &event)
{
    event.Skip();
    if (const int width = GetClientSize().x; width != m_layoutWidth)
    {
        m_layoutWidth = width;
        m_items.clear();
        SetItemCount(GetItemCount());
        RefreshAll();
    }
}

void QSPLiteListBox::OnDPIChanged(wxDPIChangedEvent &event)
{
    event.Skip();
    RefreshUI();
}

void QSPLiteListBox::OnAnimationTimer([[maybe_unused]] wxTimerEvent &event)
{
    if (!m_container.HasAnimations())
        m_animationTimer.Stop();
    else if (m_container.AdvanceAnimations())
        Refresh();
}
