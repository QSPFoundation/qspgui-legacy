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

#include "lite_container.h"
#include "../comtools.h"
#include <wx/filesys.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include <wx/fontenum.h>
#include <wx/tokenzr.h>
#include <algorithm>
#include <cmath>

namespace
{
    constexpr int MAX_IMAGE_DIMENSION = 8192;
    constexpr wxFileOffset MAX_CSS_SIZE = 4 << 20;
    constexpr size_t MAX_CACHED_WIDTHS = 20000;
    constexpr int MAX_TILES = 10000;

    wxColour ToColour(const litehtml::web_color &color)
    {
        return {color.red, color.green, color.blue, color.alpha};
    }

    uint32_t ColorKey(const litehtml::web_color &color)
    {
        return static_cast<uint32_t>(color.red) << 24 | static_cast<uint32_t>(color.green) << 16 |
               static_cast<uint32_t>(color.blue) << 8 | color.alpha;
    }

    wxGraphicsContext *ToContext(const litehtml::uint_ptr hdc)
    {
        return reinterpret_cast<wxGraphicsContext *>(hdc);
    }

    double Px(const litehtml::pixel_t value)
    {
        return static_cast<double>(static_cast<float>(value));
    }

    bool HasRadius(const litehtml::border_radiuses &r)
    {
        return Px(r.top_left_x) > 0 || Px(r.top_left_y) > 0 || Px(r.top_right_x) > 0 || Px(r.top_right_y) > 0 ||
               Px(r.bottom_right_x) > 0 || Px(r.bottom_right_y) > 0 || Px(r.bottom_left_x) > 0 ||
               Px(r.bottom_left_y) > 0;
    }

    void AddRoundedRect(wxGraphicsPath &path, const litehtml::position &pos, const litehtml::border_radiuses &r)
    {
        const double x = Px(pos.x), y = Px(pos.y), w = Px(pos.width), h = Px(pos.height);
        const double k = 0.5522847498; // Bezier approximation of a quarter of a circle
        const double tlx = Px(r.top_left_x), tly = Px(r.top_left_y);
        const double trx = Px(r.top_right_x), try_ = Px(r.top_right_y);
        const double brx = Px(r.bottom_right_x), bry = Px(r.bottom_right_y);
        const double blx = Px(r.bottom_left_x), bly = Px(r.bottom_left_y);

        path.MoveToPoint(x + tlx, y);
        path.AddLineToPoint(x + w - trx, y);
        if (trx > 0 || try_ > 0)
            path.AddCurveToPoint(x + w - trx + trx * k, y, x + w, y + try_ - try_ * k, x + w, y + try_);
        path.AddLineToPoint(x + w, y + h - bry);
        if (brx > 0 || bry > 0)
            path.AddCurveToPoint(x + w, y + h - bry + bry * k, x + w - brx + brx * k, y + h, x + w - brx, y + h);
        path.AddLineToPoint(x + blx, y + h);
        if (blx > 0 || bly > 0)
            path.AddCurveToPoint(x + blx - blx * k, y + h, x, y + h - bly + bly * k, x, y + h - bly);
        path.AddLineToPoint(x, y + tly);
        if (tlx > 0 || tly > 0)
            path.AddCurveToPoint(x, y + tly - tly * k, x + tlx - tlx * k, y, x + tlx, y);
        path.CloseSubpath();
    }

    wxGraphicsGradientStops MakeStops(const litehtml::background_layer::gradient_base &gradient)
    {
        wxGraphicsGradientStops stops;
        if (gradient.color_points.empty()) return stops;
        stops.SetStartColour(ToColour(gradient.color_points.front().color));
        stops.SetEndColour(ToColour(gradient.color_points.back().color));
        for (const auto &point : gradient.color_points)
            stops.Add(ToColour(point.color), std::clamp(point.offset, 0.0f, 1.0f));
        return stops;
    }

    wxString FromUtf8(const char *text)
    {
        return wxString::FromUTF8(text ? text : "");
    }
}

QSPLiteContainer::QSPLiteContainer(QSPLiteHost &host) : m_host(host)
{
    SetDefaultFont(wxNORMAL_FONT->GetFaceName(), 16.0);
}

QSPLiteContainer::~QSPLiteContainer() = default;

wxGraphicsRenderer *QSPLiteContainer::GetRenderer()
{
#if wxUSE_GRAPHICS_DIRECT2D
    static wxGraphicsRenderer *const renderer = [] {
        wxString value;
        if (wxGetEnv("QSP_LITEHTML_GRAPHICS", &value) && value.IsSameAs("direct2d", false))
        {
            if (wxGraphicsRenderer *direct2D = wxGraphicsRenderer::GetDirect2DRenderer()) return direct2D;
        }
        return wxGraphicsRenderer::GetDefaultRenderer();
    }();
    return renderer;
#else
    return wxGraphicsRenderer::GetDefaultRenderer();
#endif
}

void QSPLiteContainer::SetDefaultFont(const wxString &faceName, const double sizeInPixels)
{
    m_defaultFaceName = faceName;
    m_defaultFaceNameUtf8 = faceName.utf8_string();
    m_defaultFontSize = std::max(sizeInPixels, 1.0);
}

void QSPLiteContainer::ClearImages()
{
    m_images.clear();
}

bool QSPLiteContainer::AdvanceAnimations()
{
    const wxLongLong now = wxGetLocalTimeMillis();
    bool isChanged = false;
    for (auto &[src, image] : m_images)
    {
        if (!image.animation) continue;
        int steps = 0;
        while (image.nextFrameTime <= now && steps++ < 100)
        {
            image.animation->Advance();
            image.nextFrameTime += image.animation->GetDelay();
            isChanged = true;
        }
        if (image.nextFrameTime <= now) image.nextFrameTime = now + image.animation->GetDelay();
        if (steps > 0) image.bitmap = wxBitmap(image.animation->GetFrame());
    }
    return isChanged;
}

bool QSPLiteContainer::HasAnimations() const
{
    return std::any_of(m_images.begin(), m_images.end(), [](const auto &item) { return item.second.animation != nullptr; });
}

wxGraphicsContext *QSPLiteContainer::GetMeasuringContext()
{
    if (!m_measuringContext) m_measuringContext.reset(GetRenderer()->CreateMeasuringContext());
    return m_measuringContext.get();
}

wxString QSPLiteContainer::ResolveFile(const std::string &src) const
{
    if (!m_pathProvider || src.empty()) return wxEmptyString;
    const wxString url = QSPTools::GetGameResourceURL(*m_pathProvider, wxString::FromUTF8(src));
    if (url.IsEmpty()) return wxEmptyString;
    return wxFileSystem::URLToFileName(url).GetFullPath();
}

QSPLiteContainer::Image *QSPLiteContainer::GetImage(const std::string &src)
{
    if (const wxString gamePath = m_pathProvider ? m_pathProvider->GetGamePath() : wxString();
        gamePath != m_imagesGamePath)
    {
        m_images.clear();
        m_imagesGamePath = gamePath;
    }

    if (const auto found = m_images.find(src); found != m_images.end())
        return found->second.image.IsOk() || found->second.animation ? &found->second : nullptr;

    Image &image = m_images[src];
    const wxString path = ResolveFile(src);
    if (path.IsEmpty() || !wxFileExists(path)) return nullptr;

    wxFFileInputStream file(path);
    if (!file.IsOk()) return nullptr;
    wxMemoryOutputStream memory;
    file.Read(memory);
    wxMemoryInputStream data(memory);

    auto animation = std::make_unique<QSPAnimation>();
    if (animation->Load(data))
    {
        image.animation = std::move(animation);
        image.nextFrameTime = wxGetLocalTimeMillis() + image.animation->GetDelay();
        image.bitmap = wxBitmap(image.animation->GetFrame());
        return &image;
    }

    data.SeekI(0);
    wxImage loaded;
    {
        wxLogNull noLog; // a broken image of the game isn't worth a message box
        if (!loaded.LoadFile(data)) return nullptr;
    }
    if (loaded.GetWidth() > MAX_IMAGE_DIMENSION || loaded.GetHeight() > MAX_IMAGE_DIMENSION) return nullptr;
    image.image = loaded;
    image.bitmap = wxBitmap(loaded);
    return &image;
}

wxString QSPLiteContainer::FindFaceName(const std::string &families) const
{
    wxStringTokenizer tokens(wxString::FromUTF8(families), ",");
    while (tokens.HasMoreTokens())
    {
        wxString name = tokens.GetNextToken().Trim().Trim(false);
        if (name.length() >= 2 && (name[0] == '"' || name[0] == '\'') && name.Last() == name[0])
            name = name.Mid(1, name.length() - 2);
        if (name.IsEmpty()) continue;

        wxFontFamily family = wxFONTFAMILY_UNKNOWN;
        if (name.IsSameAs("serif", false))
            family = wxFONTFAMILY_ROMAN;
        else if (name.IsSameAs("sans-serif", false) || name.IsSameAs("system-ui", false))
            family = wxFONTFAMILY_SWISS;
        else if (name.IsSameAs("monospace", false))
            family = wxFONTFAMILY_TELETYPE;
        else if (name.IsSameAs("cursive", false))
            family = wxFONTFAMILY_SCRIPT;
        else if (name.IsSameAs("fantasy", false))
            family = wxFONTFAMILY_DECORATIVE;

        if (family != wxFONTFAMILY_UNKNOWN)
            return wxFont(wxFontInfo().Family(family)).GetFaceName();
        if (wxFontEnumerator::IsValidFacename(name)) return name;
    }
    return m_defaultFaceName;
}

litehtml::uint_ptr QSPLiteContainer::create_font(const litehtml::font_description &descr,
                                                 [[maybe_unused]] const litehtml::document *doc,
                                                 litehtml::font_metrics *fm)
{
    auto font = std::make_unique<Font>();
    font->faceName = FindFaceName(descr.family);
    font->size = std::max(Px(descr.size), 1.0);
    if (descr.weight >= 600) font->flags |= wxFONTFLAG_BOLD;
    if (descr.style != litehtml::font_style_normal) font->flags |= wxFONTFLAG_ITALIC;
    font->decoration = descr.decoration_line & (litehtml::text_decoration_line_underline |
                                                litehtml::text_decoration_line_overline |
                                                litehtml::text_decoration_line_line_through);
    font->decorationColor = descr.decoration_color;
    font->measuringFont = GetRenderer()->CreateFont(font->size, font->faceName, font->flags, *wxBLACK);

    wxGraphicsContext *gc = GetMeasuringContext();
    gc->SetFont(font->measuringFont);
    wxDouble width, height, descent, leading;
    gc->GetTextExtent("x", &width, &height, &descent, &leading);
    wxDouble zeroWidth;
    gc->GetTextExtent("0", &zeroWidth, nullptr);

    litehtml::font_metrics &metrics = font->metrics;
    metrics.font_size = static_cast<float>(font->size);
    metrics.height = static_cast<float>(std::ceil(height));
    metrics.descent = static_cast<float>(std::ceil(descent));
    metrics.ascent = static_cast<float>(std::ceil(height - descent));
    metrics.x_height = static_cast<float>((height - descent) * 0.53);
    metrics.ch_width = static_cast<float>(zeroWidth);
    metrics.draw_spaces = font->decoration != litehtml::text_decoration_line_none;
    metrics.sub_shift = static_cast<float>(font->size / 5);
    metrics.super_shift = static_cast<float>(font->size / 3);
    if (fm) *fm = metrics;

    return reinterpret_cast<litehtml::uint_ptr>(font.release());
}

void QSPLiteContainer::delete_font(const litehtml::uint_ptr hFont)
{
    delete reinterpret_cast<Font *>(hFont);
}

litehtml::pixel_t QSPLiteContainer::text_width(const char *text, const litehtml::uint_ptr hFont)
{
    auto *font = reinterpret_cast<Font *>(hFont);
    if (!font || !text || !*text) return 0;

     if (const auto found = font->widths.find(text); found != font->widths.end())
        return static_cast<float>(found->second);

    wxGraphicsContext *gc = GetMeasuringContext();
    gc->SetFont(font->measuringFont);
    wxDouble width;
    gc->GetTextExtent(wxString::FromUTF8(text), &width, nullptr);

    if (font->widths.size() >= MAX_CACHED_WIDTHS) font->widths.clear();
    font->widths.emplace(text, width);
    return static_cast<float>(width);
}

void QSPLiteContainer::draw_text(const litehtml::uint_ptr hdc, const char *text, const litehtml::uint_ptr hFont,
                                 const litehtml::web_color color, const litehtml::position &pos)
{
    wxGraphicsContext *gc = ToContext(hdc);
    auto *font = reinterpret_cast<Font *>(hFont);
    if (!gc || !font || !text) return;

    wxGraphicsFont &drawingFont = font->drawingFonts[ColorKey(color)];
    if (drawingFont.IsNull())
        drawingFont = GetRenderer()->CreateFont(font->size, font->faceName, font->flags, ToColour(color));

    gc->PushState();
    ApplyClip(gc, nullptr);
    gc->SetFont(drawingFont);
    const wxString string = wxString::FromUTF8(text);
    gc->DrawText(string, Px(pos.x), Px(pos.y));

    if (font->decoration != litehtml::text_decoration_line_none)
    {
        wxDouble width;
        gc->GetTextExtent(string, &width, nullptr);
        const double thickness = std::max(1.0, font->size / 16.0);
        const double baseline = Px(pos.y) + Px(font->metrics.ascent);
        const litehtml::web_color lineColor = font->decorationColor.is_current_color ? color : font->decorationColor;
        gc->SetBrush(wxBrush(ToColour(lineColor)));
        gc->SetPen(*wxTRANSPARENT_PEN);
        if (font->decoration & litehtml::text_decoration_line_underline)
            gc->DrawRectangle(Px(pos.x), baseline + thickness, width, thickness);
        if (font->decoration & litehtml::text_decoration_line_line_through)
            gc->DrawRectangle(Px(pos.x), baseline - Px(font->metrics.x_height) / 2, width, thickness);
        if (font->decoration & litehtml::text_decoration_line_overline)
            gc->DrawRectangle(Px(pos.x), Px(pos.y), width, thickness);
    }
    gc->PopState();
}

litehtml::pixel_t QSPLiteContainer::pt_to_px(const float pt) const
{
    return pt * 96.0f / 72.0f;
}

litehtml::pixel_t QSPLiteContainer::get_default_font_size() const
{
    return static_cast<float>(m_defaultFontSize);
}

const char *QSPLiteContainer::get_default_font_name() const
{
    return m_defaultFaceNameUtf8.c_str();
}

void QSPLiteContainer::draw_list_marker(const litehtml::uint_ptr hdc, const litehtml::list_marker &marker)
{
    wxGraphicsContext *gc = ToContext(hdc);
    if (!gc) return;

    const double x = Px(marker.pos.x), y = Px(marker.pos.y);
    const double w = Px(marker.pos.width), h = Px(marker.pos.height);
    gc->PushState();
    ApplyClip(gc, nullptr);
    if (!marker.image.empty())
    {
        if (const Image *image = GetImage(marker.image))
            gc->DrawBitmap(image->bitmap, x, y, image->bitmap.GetWidth(), image->bitmap.GetHeight());
    }
    else
    {
        const wxColour color = ToColour(marker.color);
        switch (marker.marker_type)
        {
        case litehtml::list_style_type_circle:
            gc->SetPen(wxPen(color, 1));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->DrawEllipse(x, y, w, h);
            break;
        case litehtml::list_style_type_disc:
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(color));
            gc->DrawEllipse(x, y, w, h);
            break;
        case litehtml::list_style_type_square:
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(color));
            gc->DrawRectangle(x, y, w, h);
            break;
        default:
            break; // litehtml draws the numbers itself
        }
    }
    gc->PopState();
}

void QSPLiteContainer::load_image(const char *src, [[maybe_unused]] const char *baseurl,
                                  [[maybe_unused]] bool redraw_on_ready)
{
    if (src) GetImage(src);
}

void QSPLiteContainer::get_image_size(const char *src, [[maybe_unused]] const char *baseurl, litehtml::size &sz)
{
    sz.width = 0;
    sz.height = 0;
    if (!src) return;
    if (const Image *image = GetImage(src))
    {
        sz.width = image->bitmap.GetWidth();
        sz.height = image->bitmap.GetHeight();
    }
}

void QSPLiteContainer::ApplyClip(wxGraphicsContext *gc, const litehtml::background_layer *layer) const
{
    for (const auto &[box, radius] : m_clips)
        gc->Clip(Px(box.x), Px(box.y), Px(box.width), Px(box.height));
    if (layer)
    {
        const litehtml::position &box = layer->clip_box;
        gc->Clip(Px(box.x), Px(box.y), Px(box.width), Px(box.height));
    }
}

void QSPLiteContainer::FillLayer(wxGraphicsContext *gc, const litehtml::background_layer &layer,
                                 const wxGraphicsBrush &brush)
{
    gc->PushState();
    ApplyClip(gc, &layer);
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(brush);
    wxGraphicsPath path = gc->CreatePath();
    if (HasRadius(layer.border_radius))
        AddRoundedRect(path, layer.border_box, layer.border_radius);
    else
        path.AddRectangle(Px(layer.border_box.x), Px(layer.border_box.y), Px(layer.border_box.width),
                          Px(layer.border_box.height));
    gc->FillPath(path);
    gc->PopState();
}

void QSPLiteContainer::draw_image(const litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                                  const std::string &url, [[maybe_unused]] const std::string &base_url)
{
    wxGraphicsContext *gc = ToContext(hdc);
    if (!gc || url.empty()) return;
    const Image *image = GetImage(url);
    if (!image) return;

    const double tileW = Px(layer.origin_box.width), tileH = Px(layer.origin_box.height);
    if (tileW < 1 || tileH < 1) return;

    gc->PushState();
    ApplyClip(gc, &layer);
    gc->SetInterpolationQuality(wxINTERPOLATION_GOOD);

    const litehtml::position &clip = layer.clip_box;
    double startX = Px(layer.origin_box.x), startY = Px(layer.origin_box.y);
    double endX = startX + tileW, endY = startY + tileH;
    const bool isRepeatX = layer.repeat == litehtml::background_repeat_repeat ||
                           layer.repeat == litehtml::background_repeat_repeat_x;
    const bool isRepeatY = layer.repeat == litehtml::background_repeat_repeat ||
                           layer.repeat == litehtml::background_repeat_repeat_y;
    if (isRepeatX)
    {
        startX -= std::ceil((startX - Px(clip.x)) / tileW) * tileW;
        endX = Px(clip.x) + Px(clip.width);
    }
    if (isRepeatY)
    {
        startY -= std::ceil((startY - Px(clip.y)) / tileH) * tileH;
        endY = Px(clip.y) + Px(clip.height);
    }

    int tiles = 0;
    for (double y = startY; y < endY && tiles < MAX_TILES; y += tileH)
        for (double x = startX; x < endX && tiles < MAX_TILES; x += tileW, ++tiles)
            gc->DrawBitmap(image->bitmap, x, y, tileW, tileH);
    gc->PopState();
}

void QSPLiteContainer::draw_solid_fill(const litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                                       const litehtml::web_color &color)
{
    wxGraphicsContext *gc = ToContext(hdc);
    if (!gc || color.alpha == 0) return;
    FillLayer(gc, layer, gc->CreateBrush(wxBrush(ToColour(color))));
}

void QSPLiteContainer::draw_linear_gradient(const litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                                            const litehtml::background_layer::linear_gradient &gradient)
{
    wxGraphicsContext *gc = ToContext(hdc);
    if (!gc || gradient.color_points.empty()) return;
    // The points are in document coordinates already
    FillLayer(gc, layer, gc->CreateLinearGradientBrush(gradient.start.x, gradient.start.y, gradient.end.x,
                                                       gradient.end.y, MakeStops(gradient)));
}

void QSPLiteContainer::draw_radial_gradient(const litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                                            const litehtml::background_layer::radial_gradient &gradient)
{
    wxGraphicsContext *gc = ToContext(hdc);
    if (!gc || gradient.color_points.empty()) return;
    const double cx = gradient.position.x, cy = gradient.position.y;
    const double rx = gradient.radius.x, ry = gradient.radius.y;
    if (rx <= 0 || ry <= 0) return;
    if (std::abs(rx - ry) < 0.5)
    {
        FillLayer(gc, layer, gc->CreateRadialGradientBrush(cx, cy, cx, cy, rx, MakeStops(gradient)));
        return;
    }

    // An ellipse is a circle stretched vertically: the box is drawn in the stretched coordinates
    gc->PushState();
    ApplyClip(gc, &layer);
    gc->Translate(cx, cy);
    gc->Scale(1.0, ry / rx);
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(gc->CreateRadialGradientBrush(0, 0, 0, 0, rx, MakeStops(gradient)));
    const litehtml::position &box = layer.border_box;
    gc->DrawRectangle(Px(box.x) - cx, (Px(box.y) - cy) * rx / ry, Px(box.width), Px(box.height) * rx / ry);
    gc->PopState();
}

void QSPLiteContainer::draw_conic_gradient(const litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                                           const litehtml::background_layer::conic_gradient &gradient)
{
    wxGraphicsContext *gc = ToContext(hdc);
    if (!gc || gradient.color_points.empty()) return;
    // Not supported by wxGraphicsContext: the first color is drawn instead
    FillLayer(gc, layer, gc->CreateBrush(wxBrush(ToColour(gradient.color_points.front().color))));
}

void QSPLiteContainer::draw_borders(const litehtml::uint_ptr hdc, const litehtml::borders &borders,
                                    const litehtml::position &draw_pos, [[maybe_unused]] const bool root)
{
    wxGraphicsContext *gc = ToContext(hdc);
    if (!gc) return;

    const auto isVisible = [](const litehtml::border &border) {
        return Px(border.width) > 0 && border.style != litehtml::border_style_none &&
               border.style != litehtml::border_style_hidden && border.color.alpha > 0;
    };
    const auto penStyle = [](const litehtml::border &border) {
        if (border.style == litehtml::border_style_dotted) return wxPENSTYLE_DOT;
        if (border.style == litehtml::border_style_dashed) return wxPENSTYLE_LONG_DASH;
        return wxPENSTYLE_SOLID;
    };

    const double x = Px(draw_pos.x), y = Px(draw_pos.y), w = Px(draw_pos.width), h = Px(draw_pos.height);

    gc->PushState();
    ApplyClip(gc, nullptr);

    // Rounded corners: one line along the middle of the border, when all sides look the same
    const litehtml::border &top = borders.top;
    const bool isUniform = isVisible(top) && Px(borders.left.width) == Px(top.width) &&
                           Px(borders.right.width) == Px(top.width) && Px(borders.bottom.width) == Px(top.width) &&
                           borders.left.color == top.color && borders.right.color == top.color &&
                           borders.bottom.color == top.color && borders.left.style == top.style &&
                           borders.right.style == top.style && borders.bottom.style == top.style;
    if (isUniform && HasRadius(borders.radius))
    {
        const float half = static_cast<float>(Px(top.width) / 2);
        // The line goes along the middle of the border, its corners are smaller by half of the width
        litehtml::border_radiuses radius = borders.radius;
        for (litehtml::pixel_t *value : {&radius.top_left_x, &radius.top_left_y, &radius.top_right_x,
                                         &radius.top_right_y, &radius.bottom_right_x, &radius.bottom_right_y,
                                         &radius.bottom_left_x, &radius.bottom_left_y})
            *value = std::max(static_cast<float>(*value) - half, 0.0f);
        wxGraphicsPath path = gc->CreatePath();
        AddRoundedRect(path,
                       litehtml::position(static_cast<float>(x) + half, static_cast<float>(y) + half,
                                          static_cast<float>(w) - 2 * half, static_cast<float>(h) - 2 * half),
                       radius);
        gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(ToColour(top.color), Px(top.width), penStyle(top))));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->StrokePath(path);
        gc->PopState();
        return;
    }

    const double lw = isVisible(borders.left) ? Px(borders.left.width) : 0;
    const double tw = isVisible(borders.top) ? Px(borders.top.width) : 0;
    const double rw = isVisible(borders.right) ? Px(borders.right.width) : 0;
    const double bw = isVisible(borders.bottom) ? Px(borders.bottom.width) : 0;

    // Every side is a trapezoid, so the corners meet diagonally as in browsers
    const auto drawSide = [&](const litehtml::border &border, const wxPoint2DDouble (&points)[4],
                              const wxPoint2DDouble &from, const wxPoint2DDouble &to, const double width,
                              const bool isTopLeft) {
        if (width <= 0) return;
        wxColour color = ToColour(border.color);
        // The 3D styles darken one pair of sides and lighten the other, as browsers do
        const bool isSunken = border.style == litehtml::border_style_inset || border.style == litehtml::border_style_groove;
        const bool isRaised = border.style == litehtml::border_style_outset || border.style == litehtml::border_style_ridge;
        if (isSunken || isRaised) color = color.ChangeLightness(isTopLeft == isSunken ? 60 : 140);
        if (border.style == litehtml::border_style_dotted || border.style == litehtml::border_style_dashed)
        {
            gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(color, width, penStyle(border))));
            gc->StrokeLine(from.m_x, from.m_y, to.m_x, to.m_y);
            return;
        }
        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(points[0]);
        for (int i = 1; i < 4; ++i) path.AddLineToPoint(points[i]);
        path.CloseSubpath();
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(color));
        gc->FillPath(path);
    };

    drawSide(borders.top, {{x, y}, {x + w, y}, {x + w - rw, y + tw}, {x + lw, y + tw}}, {x, y + tw / 2},
             {x + w, y + tw / 2}, tw, true);
    drawSide(borders.bottom, {{x, y + h}, {x + w, y + h}, {x + w - rw, y + h - bw}, {x + lw, y + h - bw}},
             {x, y + h - bw / 2}, {x + w, y + h - bw / 2}, bw, false);
    drawSide(borders.left, {{x, y}, {x + lw, y + tw}, {x + lw, y + h - bw}, {x, y + h}}, {x + lw / 2, y},
             {x + lw / 2, y + h}, lw, true);
    drawSide(borders.right, {{x + w, y}, {x + w - rw, y + tw}, {x + w - rw, y + h - bw}, {x + w, y + h}},
             {x + w - rw / 2, y}, {x + w - rw / 2, y + h}, rw, false);
    gc->PopState();
}

void QSPLiteContainer::set_caption([[maybe_unused]] const char *caption)
{
}

void QSPLiteContainer::set_base_url([[maybe_unused]] const char *base_url)
{
    // Every resource is resolved from the game folder
}

void QSPLiteContainer::link([[maybe_unused]] const std::shared_ptr<litehtml::document> &doc,
                            const litehtml::element::ptr &el)
{
    // <link rel="stylesheet"> of the game
    const char *rel = el->get_attr("rel");
    const char *href = el->get_attr("href");
    if (!rel || !href || !wxString::FromUTF8(rel).IsSameAs("stylesheet", false)) return;

    std::string text, baseurl;
    import_css(text, href, baseurl);
    if (!text.empty()) doc->add_stylesheet(text.c_str(), baseurl.c_str(), el->get_attr("media"));
}

void QSPLiteContainer::on_anchor_click(const char *url, [[maybe_unused]] const litehtml::element::ptr &el)
{
    m_host.OnLinkClicked(FromUtf8(url));
}

void QSPLiteContainer::on_mouse_event([[maybe_unused]] const litehtml::element::ptr &el,
                                      [[maybe_unused]] litehtml::mouse_event event)
{
}

void QSPLiteContainer::set_cursor(const char *cursor)
{
    m_host.OnCursorChanged(cursor && strcmp(cursor, "pointer") == 0);
}

void QSPLiteContainer::transform_text(std::string &text, const litehtml::text_transform tt)
{
    wxString string = wxString::FromUTF8(text);
    switch (tt)
    {
    case litehtml::text_transform_uppercase:
        string.MakeUpper();
        break;
    case litehtml::text_transform_lowercase:
        string.MakeLower();
        break;
    case litehtml::text_transform_capitalize:
        if (!string.IsEmpty()) string[0] = wxToupper(string[0]);
        break;
    default:
        return;
    }
    text = string.utf8_string();
}

void QSPLiteContainer::import_css(std::string &text, const std::string &url, std::string &baseurl)
{
    text.clear();
    baseurl.clear();
    // Style sheets come from the game folder only, like images
    const wxString path = ResolveFile(url);
    if (path.IsEmpty() || !wxFileExists(path)) return;

    wxFFileInputStream file(path);
    if (!file.IsOk()) return;
    const wxFileOffset size = std::min(file.GetLength(), MAX_CSS_SIZE);
    if (size <= 0) return;
    text.resize(static_cast<size_t>(size));
    file.Read(text.data(), text.size());
    text.resize(file.LastRead());
}

void QSPLiteContainer::set_clip(const litehtml::position &pos, const litehtml::border_radiuses &bdr_radius)
{
    m_clips.emplace_back(pos, bdr_radius);
}

void QSPLiteContainer::del_clip()
{
    if (!m_clips.empty()) m_clips.pop_back();
}

void QSPLiteContainer::get_viewport(litehtml::position &viewport) const
{
    viewport = m_host.GetViewport();
}

litehtml::element::ptr QSPLiteContainer::create_element([[maybe_unused]] const char *tag_name,
                                                        [[maybe_unused]] const litehtml::string_map &attributes,
                                                        [[maybe_unused]] const std::shared_ptr<litehtml::document> &doc)
{
    return nullptr;
}

void QSPLiteContainer::get_media_features(litehtml::media_features &media) const
{
    const litehtml::position viewport = m_host.GetViewport();
    media.type = litehtml::media_type_screen;
    media.width = viewport.width;
    media.height = viewport.height;
    media.device_width = viewport.width;
    media.device_height = viewport.height;
    media.color = 8;
    media.monochrome = 0;
    media.color_index = 256;
    media.resolution = 96;
}

void QSPLiteContainer::get_language(std::string &language, std::string &culture) const
{
    language = "en";
    culture.clear();
}
