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
#include <wx/graphics.h>
#include <litehtml.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../animation.h"
#include "../pathprovider.h"

class QSPLiteHost
{
public:
    virtual ~QSPLiteHost() = default;

    virtual void OnLinkClicked(const wxString &href) = 0;
    virtual void OnCursorChanged(bool isLink) = 0;
    [[nodiscard]] virtual litehtml::position GetViewport() const = 0;
};

class QSPLiteContainer : public litehtml::document_container
{
public:
    explicit QSPLiteContainer(QSPLiteHost &host);
    ~QSPLiteContainer() override;

    [[nodiscard]] static wxGraphicsRenderer *GetRenderer();

    void SetPathProvider(PathProvider *provider) { m_pathProvider = provider; }
    void SetDefaultFont(const wxString &faceName, double sizeInPixels);
    void ClearImages();
    bool AdvanceAnimations();
    [[nodiscard]] bool HasAnimations() const;

    litehtml::uint_ptr create_font(const litehtml::font_description &descr, const litehtml::document *doc,
                                   litehtml::font_metrics *fm) override;
    void delete_font(litehtml::uint_ptr hFont) override;
    litehtml::pixel_t text_width(const char *text, litehtml::uint_ptr hFont) override;
    void draw_text(litehtml::uint_ptr hdc, const char *text, litehtml::uint_ptr hFont, litehtml::web_color color,
                   const litehtml::position &pos) override;
    litehtml::pixel_t pt_to_px(float pt) const override;
    litehtml::pixel_t get_default_font_size() const override;
    const char *get_default_font_name() const override;
    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker &marker) override;
    void load_image(const char *src, const char *baseurl, bool redraw_on_ready) override;
    void get_image_size(const char *src, const char *baseurl, litehtml::size &sz) override;
    void draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer &layer, const std::string &url,
                    const std::string &base_url) override;
    void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                         const litehtml::web_color &color) override;
    void draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                              const litehtml::background_layer::linear_gradient &gradient) override;
    void draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                              const litehtml::background_layer::radial_gradient &gradient) override;
    void draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                             const litehtml::background_layer::conic_gradient &gradient) override;
    void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders &borders, const litehtml::position &draw_pos,
                      bool root) override;
    void set_caption(const char *caption) override;
    void set_base_url(const char *base_url) override;
    void link(const std::shared_ptr<litehtml::document> &doc, const litehtml::element::ptr &el) override;
    void on_anchor_click(const char *url, const litehtml::element::ptr &el) override;
    void on_mouse_event(const litehtml::element::ptr &el, litehtml::mouse_event event) override;
    void set_cursor(const char *cursor) override;
    void transform_text(std::string &text, litehtml::text_transform tt) override;
    void import_css(std::string &text, const std::string &url, std::string &baseurl) override;
    void set_clip(const litehtml::position &pos, const litehtml::border_radiuses &bdr_radius) override;
    void del_clip() override;
    void get_viewport(litehtml::position &viewport) const override;
    litehtml::element::ptr create_element(const char *tag_name, const litehtml::string_map &attributes,
                                          const std::shared_ptr<litehtml::document> &doc) override;
    void get_media_features(litehtml::media_features &media) const override;
    void get_language(std::string &language, std::string &culture) const override;

private:
    struct Font
    {
        wxString faceName;
        double size{0.0};
        int flags{wxFONTFLAG_DEFAULT};
        int decoration{litehtml::text_decoration_line_none};
        litehtml::web_color decorationColor;
        litehtml::font_metrics metrics;
        wxGraphicsFont measuringFont;
        std::unordered_map<uint32_t, wxGraphicsFont> drawingFonts; // by color
        std::unordered_map<std::string, double> widths;
    };

    struct Image
    {
        wxImage image;
        std::unique_ptr<QSPAnimation> animation;
        wxLongLong nextFrameTime;
        wxBitmap bitmap; // the current frame
    };

    [[nodiscard]] wxGraphicsContext *GetMeasuringContext();
    [[nodiscard]] wxString ResolveFile(const std::string &src) const;
    Image *GetImage(const std::string &src);
    [[nodiscard]] wxString FindFaceName(const std::string &families) const;
    void ApplyClip(wxGraphicsContext *gc, const litehtml::background_layer *layer) const;
    void FillLayer(wxGraphicsContext *gc, const litehtml::background_layer &layer, const wxGraphicsBrush &brush);

    QSPLiteHost &m_host;
    PathProvider *m_pathProvider{nullptr};
    wxString m_defaultFaceName;
    std::string m_defaultFaceNameUtf8;
    double m_defaultFontSize{16.0};
    std::unique_ptr<wxGraphicsContext> m_measuringContext;
    std::unordered_map<std::string, Image> m_images;
    wxString m_imagesGamePath; // the images are relative to it
    std::vector<std::pair<litehtml::position, litehtml::border_radiuses>> m_clips;
};
