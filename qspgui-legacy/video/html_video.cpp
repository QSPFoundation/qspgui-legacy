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

#include "html_video.h"
#include "video_player.h"
#include <wx/app.h>
#include <wx/filefn.h>
#include <wx/filesys.h>
#include <wx/log.h>
#include <wx/timer.h>
#include <wx/dc.h>
#include <wx/html/htmlcell.h>
#include <wx/html/htmlwin.h>
#include <wx/html/m_templ.h>
#include <algorithm>
#include <map>
#include <tuple>

namespace
{
    constexpr int UPDATE_INTERVAL = 10; // ms
    constexpr int MAX_CELL_SIZE = 8192;

    // The page is rebuilt from scratch on every text update. Players of the destroyed cells
    // wait here until the next idle time, so that the same video on the new page keeps playing.
    class PlayerPool
    {
    public:
        struct Key
        {
            const wxWindow *window;
            wxString path;
            bool toPlay;
            bool toLoop;

            bool operator<(const Key &other) const
            {
                return std::tie(window, path, toPlay, toLoop) < std::tie(other.window, other.path, other.toPlay, other.toLoop);
            }
        };

        std::unique_ptr<VideoPlayer> Acquire(const Key &key)
        {
            const auto it = m_released.find(key);
            if (it == m_released.end()) return nullptr;
            std::unique_ptr<VideoPlayer> player = std::move(it->second);
            m_released.erase(it);
            return player;
        }

        void Release(const Key &key, std::unique_ptr<VideoPlayer> player)
        {
            m_released.emplace(key, std::move(player));
            if (!m_isPurgeScheduled && wxTheApp)
            {
                m_isPurgeScheduled = true;
                wxTheApp->CallAfter([this] { Purge(); });
            }
        }

        void Purge()
        {
            m_isPurgeScheduled = false;
            m_released.clear();
        }

        // Pages are rebuilt often, so don't try to open the same broken file over and over
        [[nodiscard]] bool HasFailed(const wxString &path) const
        {
            const auto it = m_failed.find(path);
            return it != m_failed.end() && it->second == wxFileModificationTime(path);
        }

        void SetFailed(const wxString &path)
        {
            m_failed[path] = wxFileModificationTime(path);
        }

    private:
        std::multimap<Key, std::unique_ptr<VideoPlayer>> m_released;
        std::map<wxString, time_t> m_failed;
        bool m_isPurgeScheduled{false};
    };

    PlayerPool pool;

    class QSPVideoCell;

    class QSPVideoTimer : public wxTimer
    {
    public:
        explicit QSPVideoTimer(QSPVideoCell *cell) : m_cell(cell) {}
        void Notify() override;

    private:
        QSPVideoCell *m_cell;
    };

    class QSPVideoCell : public wxHtmlCell
    {
    public:
        QSPVideoCell(wxHtmlWindowInterface *windowIface, PlayerPool::Key key, std::unique_ptr<VideoPlayer> player,
            int width, bool isWidthPercent, int height, double scale, int align) :
            m_windowIface(windowIface), m_key(std::move(key)), m_player(std::move(player)), m_timer(this),
            m_width(width), m_isWidthPercent(isWidthPercent), m_height(height), m_scale(scale), m_align(align)
        {
            SetCanLiveOnPagebreak(false);
            if (m_player)
            {
                // A reused player already has the current picture
                if (!m_player->GetBitmap().IsOk() || !m_player->IsFinished())
                    m_timer.Start(UPDATE_INTERVAL);
            }
        }

        ~QSPVideoCell() override
        {
            m_timer.Stop();
            if (m_player) pool.Release(m_key, std::move(m_player));
        }

        void Layout(int w) override
        {
            wxSize natural = m_player ? m_player->GetVideoSize() : wxSize();
            if (natural.x <= 0 || natural.y <= 0) natural = wxSize();
            const auto scaled = [this](const int value) { return (int)(m_scale * value); };

            int width, height;
            if (m_isWidthPercent)
            {
                width = w * m_width / 100;
                if (m_height >= 0)
                    height = scaled(m_height);
                else
                    height = natural.x > 0 ? (int)((long long)natural.y * width / natural.x) : 0;
            }
            else if (m_width >= 0 && m_height >= 0)
            {
                width = scaled(m_width);
                height = scaled(m_height);
            }
            else if (m_width >= 0)
            {
                width = scaled(m_width);
                height = natural.x > 0 ? (int)((long long)natural.y * width / natural.x) : 0;
            }
            else if (m_height >= 0)
            {
                height = scaled(m_height);
                width = natural.y > 0 ? (int)((long long)natural.x * height / natural.y) : 0;
            }
            else
            {
                width = scaled(natural.x);
                height = scaled(natural.y);
            }
            m_Width = std::clamp(width, 0, MAX_CELL_SIZE);
            m_Height = std::clamp(height, 0, MAX_CELL_SIZE);

            switch (m_align)
            {
            case wxHTML_ALIGN_TOP: m_Descent = m_Height; break;
            case wxHTML_ALIGN_CENTER: m_Descent = m_Height / 2; break;
            default: m_Descent = 0; break;
            }

            if (m_player) m_player->SetTargetSize(wxSize(m_Width, m_Height));
            wxHtmlCell::Layout(w);
        }

        void Draw(wxDC &dc, int x, int y, int, int, wxHtmlRenderingInfo &) override
        {
            if (!m_player || m_Width <= 0 || m_Height <= 0) return;
            const wxBitmap &bitmap = m_player->GetBitmap();
            if (!bitmap.IsOk()) return;
            if (bitmap.GetSize() == wxSize(m_Width, m_Height))
            {
                dc.DrawBitmap(bitmap, x + m_PosX, y + m_PosY, true);
            }
            else
            {
                // The layout has just changed, the next frames will have the right size
                const wxImage image = bitmap.ConvertToImage().Scale(m_Width, m_Height, wxIMAGE_QUALITY_BILINEAR);
                dc.DrawBitmap(wxBitmap(image), x + m_PosX, y + m_PosY, true);
            }
        }

        void OnTimer()
        {
            if (m_player->Update() && m_windowIface)
            {
                wxWindow *window = m_windowIface->GetHTMLWindow();
                const wxRect rect(m_windowIface->HTMLCoordsToWindow(this, GetAbsPos()), wxSize(m_Width, m_Height));
                if (window && window->GetClientRect().Intersects(rect))
                    window->RefreshRect(rect);
            }
            if (m_player->IsFinished() || (!m_key.toPlay && m_player->GetBitmap().IsOk()))
                m_timer.Stop();
        }

    private:
        wxHtmlWindowInterface *m_windowIface;
        PlayerPool::Key m_key;
        std::unique_ptr<VideoPlayer> m_player;
        QSPVideoTimer m_timer;
        int m_width;
        bool m_isWidthPercent;
        int m_height;
        double m_scale;
        int m_align;
    };

    void QSPVideoTimer::Notify()
    {
        m_cell->OnTimer();
    }

    wxString ResolveVideoPath(const wxHtmlWindowInterface *windowIface, const wxString &src)
    {
        // Same rules as for images: the window keeps the game inside its folder
        wxString path = src;
        wxString redirect;
        switch (windowIface->OnHTMLOpeningURL(wxHTML_URL_OTHER, src, &redirect))
        {
        case wxHTML_OPEN: break;
        case wxHTML_REDIRECT: path = redirect; break;
        default: return wxEmptyString;
        }
        if (path.StartsWith("file:")) path = wxFileSystem::URLToFileName(path).GetFullPath();
        return wxFileExists(path) ? path : wxString();
    }
}

TAG_HANDLER_BEGIN(VIDEO, "VIDEO")
    TAG_HANDLER_CONSTR(VIDEO) { }

    TAG_HANDLER_PROC(tag)
    {
        // Like a browser that supports <video>, never show the fallback content inside the tag
        wxHtmlWindowInterface *windowIface = m_WParser->GetWindowInterface();
        wxString src;
        if (!windowIface || !windowIface->GetHTMLWindow() || !tag.GetParamAsString("SRC", &src) || src.IsEmpty())
            return true;

        int width = wxDefaultCoord, height = wxDefaultCoord;
        bool isWidthPercent = false;
        if (tag.GetParamAsIntOrPercent("WIDTH", &width, isWidthPercent))
        {
            if (isWidthPercent) width = std::clamp(width, 0, 100);
            else if (width < 0) width = wxDefaultCoord;
        }
        if (tag.GetParamAsInt("HEIGHT", &height) && height < 0)
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

        const wxString path = ResolveVideoPath(windowIface, src);
        PlayerPool::Key key{windowIface->GetHTMLWindow(), path, tag.HasParam("AUTOPLAY"), tag.HasParam("LOOP")};
        std::unique_ptr<VideoPlayer> player;
        if (!path.IsEmpty())
        {
            player = pool.Acquire(key);
            if (!player && !pool.HasFailed(path))
            {
                wxString error;
                player = VideoPlayer::Create(path, key.toPlay, key.toLoop, error);
                if (!player)
                {
                    pool.SetFailed(path);
                    wxLogDebug("Can't play \"%s\": %s", path, error);
                }
            }
            if (player) player->SetMuted(tag.HasParam("MUTED"));
        }

        auto *cell = new QSPVideoCell(windowIface, std::move(key), std::move(player),
            width, isWidthPercent, height, m_WParser->GetPixelScale(), align);
        m_WParser->ApplyStateToCell(cell);
        m_WParser->StopCollapsingSpaces();
        m_WParser->GetContainer()->InsertCell(cell);
        return true;
    }
TAG_HANDLER_END(VIDEO)

TAGS_MODULE_BEGIN(QSPVideo)
    TAGS_MODULE_ADD(VIDEO)
TAGS_MODULE_END(QSPVideo)

void QSPVideo::SetOverallVolume(const float volume)
{
    VideoPlayer::SetOverallVolume(volume);
}

void QSPVideo::ReleasePlayers()
{
    pool.Purge();
}
