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

#include "htmlview.h"
#include "textbox.h"
#include "listbox.h"
#if QSP_LITEHTML
#include "lite/lite_view.h"
#include "lite/lite_listbox.h"
#endif

wxString GetAvailableRenderer(const wxString &name)
{
#if QSP_LITEHTML
    if (name == RENDERER_LITEHTML) return RENDERER_LITEHTML;
#endif
    return RENDERER_WXHTML;
}

QSPHtmlView *CreateHtmlView(wxWindow *parent, const wxWindowID id, const wxString &renderer)
{
#if QSP_LITEHTML
    if (GetAvailableRenderer(renderer) == RENDERER_LITEHTML) return new QSPLiteView(parent, id);
#endif
    return new QSPTextBox(parent, id);
}

QSPListView *CreateListView(wxWindow *parent, const wxWindowID id, const ListBoxType type, const wxString &renderer)
{
#if QSP_LITEHTML
    if (GetAvailableRenderer(renderer) == RENDERER_LITEHTML) return new QSPLiteListBox(parent, id, type);
#endif
    return new QSPListBox(parent, id, type);
}
