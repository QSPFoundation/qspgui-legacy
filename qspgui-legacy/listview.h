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
#include <wx/wupdlock.h>
#include "comtools.h"
#include "pathprovider.h"

enum class ListBoxType
{
    Normal,
    Extended // runs the item under the cursor on click, as for actions
};

class QSPListView
{
public:
    virtual ~QSPListView() = default;

    [[nodiscard]] virtual wxWindow *GetWindow() = 0;
    [[nodiscard]] virtual const wxWindow *GetWindow() const = 0;

    virtual void SetPathProvider(PathProvider *provider) = 0;
    virtual void SetIsHtml(bool isHtml) = 0;
    [[nodiscard]] virtual bool IsHtml() const = 0;
    virtual void SetToShowNums(bool toShow) = 0;
    [[nodiscard]] virtual bool ToShowNums() const = 0;
    virtual void BeginItems() = 0;
    virtual void AddItem(const wxString &image, const wxString &desc) = 0;
    virtual void EndItems() = 0;
    [[nodiscard]] virtual const wxArrayString &GetImages() const = 0;
    [[nodiscard]] virtual const wxArrayString &GetDescs() const = 0;
    virtual void SelectItem(int index) = 0;
    [[nodiscard]] virtual int GetSelectedItem() const = 0;
    virtual void SetTextFont(const wxFont &font) = 0;
    [[nodiscard]] virtual wxFont GetTextFont() const = 0;
    virtual void SetLinkColor(const wxColour &color) = 0;
    [[nodiscard]] virtual const wxColour &GetLinkColor() const = 0;
    virtual void RefreshUI() = 0;
};

template <class Base>
class QSPListBoxBase : public Base, public QSPListView
{
public:
    QSPListBoxBase(wxWindow *parent, const wxWindowID id, const ListBoxType type)
        : Base(parent, id, wxDefaultPosition, wxDefaultSize, wxNO_BORDER), m_type(type)
    {
        this->Bind(wxEVT_MOTION, &QSPListBoxBase::OnMouseMove, this);
        this->Bind(wxEVT_LEFT_DOWN, &QSPListBoxBase::OnMouseClick, this);
        this->Bind(wxEVT_CHAR, &QSPListBoxBase::OnChar, this);
        this->Bind(wxEVT_KEY_UP, &QSPListBoxBase::OnKeyUp, this);
        this->Bind(wxEVT_MOUSEWHEEL, &QSPListBoxBase::OnMouseWheel, this);
        this->SetSelectionBackground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
    }

    [[nodiscard]] wxWindow *GetWindow() override { return this; }
    [[nodiscard]] const wxWindow *GetWindow() const override { return this; }

    void SetPathProvider(PathProvider *provider) override { m_pathProvider = provider; }

    void SetIsHtml(const bool isHtml) override
    {
        if (m_toUseHtml == isHtml) return;
        m_toUseHtml = isHtml;
        RefreshUI();
    }
    [[nodiscard]] bool IsHtml() const override { return m_toUseHtml; }

    void SetToShowNums(const bool toShow) override
    {
        if (m_toShowNums == toShow) return;
        m_toShowNums = toShow;
        RefreshUI();
    }
    [[nodiscard]] bool ToShowNums() const override { return m_toShowNums; }

    void BeginItems() override
    {
        m_newImages.Clear();
        m_newDescs.Clear();
    }

    void AddItem(const wxString &image, const wxString &desc) override
    {
        m_newImages.Add(image);
        m_newDescs.Add(desc);
    }

    void EndItems() override
    {
        if (m_images == m_newImages && m_descs == m_newDescs) return;

        m_images = m_newImages;
        m_descs = m_newDescs;

        wxWindowUpdateLocker noUpdates{this};
        OnContentsChanged();
        const size_t count = m_descs.GetCount();
        this->SetItemCount(count);
        this->RefreshAll();
        if (count > 0) this->ScrollToRow(0);
    }

    [[nodiscard]] const wxArrayString &GetImages() const override { return m_images; }
    [[nodiscard]] const wxArrayString &GetDescs() const override { return m_descs; }

    void SelectItem(const int index) override { this->SetSelection(index); }
    [[nodiscard]] int GetSelectedItem() const override { return this->GetSelection(); }

    void SetTextFont(const wxFont &font) override
    {
        if (m_font.GetFaceName().IsSameAs(font.GetFaceName(), false) && m_font.GetPointSize() == font.GetPointSize())
            return;
        m_font = font;
        OnFontChanged();
    }
    [[nodiscard]] wxFont GetTextFont() const override { return m_font; }

    void SetLinkColor(const wxColour &color) override
    {
        m_linkColor = color;
        RefreshUI();
    }
    [[nodiscard]] const wxColour &GetLinkColor() const override { return m_linkColor; }

    void RefreshUI() override
    {
        wxWindowUpdateLocker noUpdates{this};
        OnContentsChanged();
        this->SetItemCount(this->GetItemCount());
        this->RefreshAll();
    }

protected:
    virtual void OnContentsChanged() {}
    virtual void OnFontChanged() { RefreshUI(); }

    [[nodiscard]] wxString GetItemCells(const size_t n, const bool toKeepRawText) const
    {
        const wxString text = QSPTools::HtmlizeWhitespaces(
            m_toUseHtml ? m_descs[n] : QSPTools::ProceedAsPlain(m_descs[n]), toKeepRawText);
        wxString cells;
        if (m_toShowNums && n < 9) cells << wxString::Format("<TD>[%d]</TD>", static_cast<int>(n + 1));
        if (!m_images[n].IsEmpty()) cells << "<TD><IMG SRC=\"" << m_images[n] << "\"></TD>";
        cells << "<TD WIDTH=100%>" << text << "</TD>";
        return cells;
    }

    PathProvider *m_pathProvider{nullptr};
    wxFont m_font{*wxNORMAL_FONT};
    wxColour m_linkColor{0, 0, 0xFF};

private:
    void OnMouseMove(wxMouseEvent &event)
    {
        event.Skip();
        if (m_type != ListBoxType::Extended) return;
        if (const int item = this->VirtualHitTest(event.GetY()); item != wxNOT_FOUND)
            this->DoHandleItemClick(item, 0);
    }

    void OnMouseClick(wxMouseEvent &event)
    {
        event.Skip();
        if (m_type != ListBoxType::Extended) return;

        const int item = this->VirtualHitTest(event.GetY());
        if (item == wxNOT_FOUND) return;
        if (this->GetSelection() != item) this->DoHandleItemClick(item, 0);
        if (this->GetSelection() == item) SendExecuteEvent(item);
    }

    void OnChar(wxKeyEvent &event)
    {
        event.Skip();
        if (m_type == ListBoxType::Extended && event.GetKeyCode() == WXK_RETURN && this->GetSelection() != wxNOT_FOUND)
        {
            SendExecuteEvent(this->GetSelection());
            this->SetFocus();
        }
    }

    void SendExecuteEvent(const int item)
    {
        wxCommandEvent clickEvent(wxEVT_LISTBOX_DCLICK, this->GetId());
        clickEvent.SetEventObject(this);
        clickEvent.SetInt(item);
        this->ProcessEvent(clickEvent);
    }

    void OnKeyUp(wxKeyEvent &event)
    {
        event.Skip();
        event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
    }

    void OnMouseWheel(wxMouseEvent &event)
    {
        event.Skip();
        if (wxFindWindowAtPoint(wxGetMousePosition()) != this) event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
    }

    ListBoxType m_type{ListBoxType::Normal};
    bool m_toUseHtml{false};
    bool m_toShowNums{false};
    wxArrayString m_images;
    wxArrayString m_descs;
    wxArrayString m_newImages;
    wxArrayString m_newDescs;
};
