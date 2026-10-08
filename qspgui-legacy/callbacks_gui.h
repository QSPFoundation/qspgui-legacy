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

#include <map>
#include <vector>
#include <memory>
#include <cstring>
#include <qsp_default.h>
#include "frame.h"
#include "msgdlg.h"
#include "inputdlg.h"
#include "sound/sound_engine.h"

struct QSPSound
{
    ma_sound_file Sound{nullptr};
    int Volume{0};

    bool Play(const wxString& file, const int volume, const float volumeCoeff)
    {
#ifdef __WINDOWS__
        Sound = sound_play_file_w(file.wc_str());
#else
        Sound = sound_play_file(file.fn_str());
#endif
        if (Sound)
        {
            SetVolume(volume, volumeCoeff);
            return true;
        }
        return false;
    }

    void SetVolume(const int volume, const float volumeCoeff)
    {
        if (Sound)
        {
            Volume = volume;
            sound_set_volume(Sound, volumeCoeff * volume / 100.0f);
        }
    }

    [[nodiscard]] bool IsPlaying() const
    {
        return Sound && static_cast<bool>(sound_is_playing(Sound));
    }

    void Close()
    {
        if (Sound)
        {
            sound_close_file(Sound);
            Sound = nullptr;
        }
    }
};

using QSPSounds = std::map<wxString, QSPSound>;
using QSPVersionInfoValues = std::map<wxString, wxString>;

inline bool qspGetVar(const char16_t* name, int* val)
{
    const auto* qspName = reinterpret_cast<QSP_CHAR*>(const_cast<char16_t*>(name));
    return QSPGetNumVarValue(qspName, 0, val) != 0;
}

inline bool qspGetStr(const char16_t* name, QSP_CHAR** val)
{
    const auto* qspName = reinterpret_cast<QSP_CHAR*>(const_cast<char16_t*>(name));
    return QSPGetStrVarValue(qspName, 0, val) != 0;
}

inline bool qspIsEmpty(const QSP_CHAR *s)
{
    return s != nullptr && s[0] == 0;
}

inline wxString qspToWxString(const QSP_CHAR *s)
{
    if (!s) return {};

    return wxString {
        reinterpret_cast<const char*>(s),
        wxMBConvUTF16()
    };
}

inline std::unique_ptr<QSP_CHAR[]> wxStringToQsp(const wxString &wx_str)
{
    if (wx_str.IsEmpty()) {
        auto empty_str = std::make_unique<QSP_CHAR[]>(1);
        empty_str[0] = 0;
        return empty_str;
    }

    wxCharBuffer buffer = wx_str.mb_str(wxMBConvUTF16());
    if (!buffer) return nullptr;

    const size_t bytes_count = buffer.length();
    const size_t chars_count = bytes_count / sizeof(QSP_CHAR);

    auto qsp_str = std::make_unique<QSP_CHAR[]>(chars_count + 1);
    std::memcpy(qsp_str.get(), buffer.data(), bytes_count);
    qsp_str[chars_count] = 0;

    return qsp_str;
}

#define QSP_STATIC_LEN(x) (sizeof(x) / sizeof(QSP_CHAR) - 1)

class QSPCallbacks
{
public:
    static void Init(QSPFrame *frame);
    static void DeInit();
    static void SetOverallVolume(float coeff);

    static void RefreshInt(QSP_BOOL isRedraw);
    static void SetTimer(int msecs);
    static void SetInputStrText(const QSP_CHAR *text);
    static QSP_BOOL IsPlay(const QSP_CHAR *file);
    static void CloseFile(const QSP_CHAR *file);
    static void PlayFile(const QSP_CHAR *file, int volume);
    static void ShowPane(int type, QSP_BOOL toShow);
    static void Sleep(int msecs);
    static int GetMSCount();
    static void Msg(const QSP_CHAR *str);
    static int ShowMenu(QSPListItem *items, int count);
    static void Input(const QSP_CHAR *text, QSP_CHAR *buffer, int maxLen);
    static void ShowImage(const QSP_CHAR *file);
    static void OpenGame(const QSP_CHAR *file, QSP_BOOL isAddLocs);
    static void OpenGameStatus(const QSP_CHAR *file);
    static void SaveGameStatus(const QSP_CHAR *file);

private:
    static bool SetVolume(const QSP_CHAR *file, int volume);
    static void UpdateSounds();

    static QSPFrame *m_frame;
    static bool m_isHtml;
    static QSPSounds m_sounds;
    static float m_volumeCoeff;
    static QSPVersionInfoValues m_versionInfo;

    static constexpr int MAX_LIST_ITEMS = 1000;
};
