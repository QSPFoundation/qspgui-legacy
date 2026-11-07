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
#include <wx/vlbox.h>
#include <vector>
#include "lite_container.h"
#include "../listview.h"

class QSPLiteListBox : public QSPListBoxBase<wxVListBox>, private QSPLiteHost
{
public:
    QSPLiteListBox(wxWindow *parent, wxWindowID id, ListBoxType type);
    ~QSPLiteListBox() override;

    void SetPathProvider(PathProvider *provider) override;

protected:
    void OnDrawItem(wxDC &dc, const wxRect &rect, size_t n) const override;
    wxCoord OnMeasureItem(size_t n) const override;
    void OnContentsChanged() override;

private:
    struct Item
    {
        litehtml::document::ptr normal;
        litehtml::document::ptr selected;
    };

    // QSPLiteHost
    void OnLinkClicked([[maybe_unused]] const wxString &href) override {}
    void OnCursorChanged([[maybe_unused]] bool isLink) override {}
    [[nodiscard]] litehtml::position GetViewport() const override;

    [[nodiscard]] double GetScale() const { return QSPLiteContainer::GetScale(*this); }
    [[nodiscard]] float GetLayoutWidth() const;
    litehtml::document *GetDocument(size_t n, bool isSelected) const;

    void OnSize(wxSizeEvent &event);
    void OnDPIChanged(wxDPIChangedEvent &event);
    void OnAnimationTimer(wxTimerEvent &event);

    mutable QSPLiteContainer m_container;
    mutable std::vector<Item> m_items;
    int m_layoutWidth{-1};
    wxTimer m_animationTimer;
};
