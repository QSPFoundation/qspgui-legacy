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

#ifndef CALLBACKS_GUI_H
    #define CALLBACKS_GUI_H

    #include <map>
    #include <qsp_default.h>
    #include "frame.h"
    #include "msgdlg.h"
    #include "inputdlg.h"
    #include "sound/sound_engine.h"

    typedef struct QSPSound_
    {
        ma_sound_file Sound;
        int Volume;

        QSPSound_()
        {
            Sound = 0;
            Volume = 0;
        }

        bool Play(const wxString& file, int volume, float volumeCoeff)
        {
            #ifdef _UNICODE
                Sound = sound_play_file_w(file.c_str());
            #else
                Sound = sound_play_file(file.c_str());
            #endif
            if (Sound)
            {
                SetVolume(volume, volumeCoeff);
                return true;
            }
            return false;
        }

        void SetVolume(int volume, float volumeCoeff)
        {
            if (Sound)
            {
                Volume = volume;
                sound_set_volume(Sound, volumeCoeff * volume / 100.0f);
            }
        }

        bool IsPlaying() const
        {
            if (Sound)
                return (bool)sound_is_playing(Sound);
            return false;
        }

        void Close()
        {
            if (Sound)
            {
                sound_close_file(Sound);
                Sound = 0;
            }
        }
    } QSPSound;

    typedef std::map<wxString, QSPSound> QSPSounds;
    typedef std::map<wxString, wxString> QSPVersionInfoValues;

    static bool qspIsEmpty(QSP_CHAR *s)
    {
        return s;
    }

    static wxString qspToWxString(QSP_CHAR *s)
    {
        return wxString(s);
    }

    /* Helpers */
    #define QSP_STATIC_LEN(x) (sizeof(x) / sizeof(QSP_CHAR) - 1)

    class QSPCallbacks
    {
    public:
        // Methods
        static void Init(QSPFrame *frame);
        static void DeInit();
        static void SetOverallVolume(float coeff);

        // Callbacks
        static int RefreshInt(QSP_BOOL isForced, QSP_BOOL isNewDesc);
        static int SetTimer(int msecs);
        static int SetInputStrText(QSP_CHAR *text);
        static int IsPlay(QSP_CHAR *file);
        static int CloseFile(QSP_CHAR *file);
        static int PlayFile(QSP_CHAR *file, int volume);
        static int ShowPane(int type, QSP_BOOL toShow);
        static int Sleep(int msecs);
        static int GetMSCount();
        static int Msg(QSP_CHAR *str);
        static int ShowMenu(QSPListItem *items, int count);
        static int Input(QSP_CHAR *text, QSP_CHAR *buffer, int maxLen);
        static int ShowImage(QSP_CHAR *file);
        static int OpenGame(QSP_CHAR *file, QSP_BOOL isAddLocs);
        static int OpenGameStatus(QSP_CHAR *file);
        static int SaveGameStatus(QSP_CHAR *file);
        static int Version(QSP_CHAR *param, QSP_CHAR *buffer, int maxLen);
    private:
        // Internal methods
        static bool SetVolume(QSP_CHAR *file, int volume);
        static void UpdateSounds();

        // Fields
        static QSPFrame *m_frame;
        static bool m_isHtml;
        static QSPSounds m_sounds;
        static float m_volumeCoeff;
        static QSPVersionInfoValues m_versionInfo;

        static const int MAX_LIST_ITEMS = 1000;
    };

#endif
