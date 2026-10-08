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

#include "callbacks_gui.h"
#include "comtools.h"
#include <vector>
#include <algorithm>

QSPFrame *QSPCallbacks::m_frame;
bool QSPCallbacks::m_isHtml;
QSPSounds QSPCallbacks::m_sounds;
float QSPCallbacks::m_volumeCoeff;
QSPVersionInfoValues QSPCallbacks::m_versionInfo;

namespace
{
    template <typename Signature>
    void RegisterCallBack(const int type, Signature *func)
    {
        QSPSetCallBack(type, reinterpret_cast<QSP_CALLBACK>(reinterpret_cast<void (*)()>(func)));
    }
}

void QSPCallbacks::Init(QSPFrame *frame)
{
    m_frame = frame;
    m_volumeCoeff = 1.0;

    if (sound_init_engine() < 0)
        wxLogError("Can't initialize sound engine");
    else
    {
        const wxString soundFontPath(QSPTools::GetResourcePath(QSP_SOUNDPLUGINS, QSP_MIDISOUNDFONT));
#ifdef __WINDOWS__
        const int soundFontInitResult = soundfont_init_w(soundFontPath.wc_str());
#else
        const int soundFontInitResult = soundfont_init(soundFontPath.fn_str());
#endif
        if (soundFontInitResult < 0)
            wxLogError("Can't load soundfont to play MIDI files");
    }

    RegisterCallBack<void(int)>(QSP_CALL_SETTIMER, &SetTimer);
    RegisterCallBack<void(QSP_BOOL)>(QSP_CALL_REFRESHINT, &RefreshInt);
    RegisterCallBack<void(const QSP_CHAR *)>(QSP_CALL_SETINPUTSTRTEXT, &SetInputStrText);
    RegisterCallBack<QSP_BOOL(const QSP_CHAR *)>(QSP_CALL_ISPLAYINGFILE, &IsPlay);
    RegisterCallBack<void(const QSP_CHAR *, int)>(QSP_CALL_PLAYFILE, &PlayFile);
    RegisterCallBack<void(const QSP_CHAR *)>(QSP_CALL_CLOSEFILE, &CloseFile);
    RegisterCallBack<void(const QSP_CHAR *)>(QSP_CALL_SHOWMSGSTR, &Msg);
    RegisterCallBack<void(int)>(QSP_CALL_SLEEP, &Sleep);
    RegisterCallBack<int()>(QSP_CALL_GETMSCOUNT, &GetMSCount);
    RegisterCallBack<int(QSPListItem *, int)>(QSP_CALL_SHOWMENU, &ShowMenu);
    RegisterCallBack<void(const QSP_CHAR *, QSP_CHAR *, int)>(QSP_CALL_INPUTBOX, &Input);
    RegisterCallBack<void(const QSP_CHAR *)>(QSP_CALL_SHOWIMAGE, &ShowImage);
    RegisterCallBack<void(int, QSP_BOOL)>(QSP_CALL_SHOWWINDOW, &ShowPane);
    RegisterCallBack<void(const QSP_CHAR *, QSP_BOOL)>(QSP_CALL_OPENGAME, &OpenGame);
    RegisterCallBack<void(const QSP_CHAR *)>(QSP_CALL_OPENGAMESTATUS, &OpenGameStatus);
    RegisterCallBack<void(const QSP_CHAR *)>(QSP_CALL_SAVEGAMESTATUS, &SaveGameStatus);

    /* Prepare version values */
    m_versionInfo["player"] = "Classic";
    m_versionInfo["platform"] = QSPTools::GetPlatform();
}

void QSPCallbacks::DeInit()
{
    CloseFile(nullptr);
    sound_free_engine();
}

void QSPCallbacks::SetTimer(const int msecs)
{
    if (m_frame->ToQuit()) return;

    if (msecs)
        m_frame->GetTimer()->Start(msecs);
    else
        m_frame->GetTimer()->Stop();
}

void QSPCallbacks::RefreshInt(const QSP_BOOL isRedraw)
{
    int numVal;
    QSP_CHAR *strVal;
    if (m_frame->ToQuit()) return;

    const bool toScroll = !(qspGetVar(QSP_FMT("DISABLESCROLL"), &numVal) && numVal);
    const bool canSave = !(qspGetVar(QSP_FMT("NOSAVE"), &numVal) && numVal);

    m_isHtml = qspGetVar(QSP_FMT("USEHTML"), &numVal) && numVal;

    m_frame->GetDesc()->SetIsHtml(m_isHtml);
    if (QSPIsMainDescChanged())
    {
        const auto *mainDesc = const_cast<QSP_CHAR *>(QSPGetMainDesc());
        // we don't scroll main description if it's completely updated
        m_frame->GetDesc()->SetText(qspToWxString(mainDesc), toScroll);
    }

    m_frame->GetVars()->SetIsHtml(m_isHtml);
    if (QSPIsVarsDescChanged())
    {
        const auto *varsDesc = const_cast<QSP_CHAR *>(QSPGetVarsDesc());
        // we always try to scroll additional description
        m_frame->GetVars()->SetText(qspToWxString(varsDesc), toScroll);
    }

    m_frame->GetActions()->SetIsHtml(m_isHtml);
    m_frame->GetActions()->SetToShowNums(m_frame->ToShowHotkeys());
    if (QSPIsActionsChanged())
    {
        std::vector<QSPListItem> items(MAX_LIST_ITEMS);

        const int actCount = QSPGetActions(items.data(), MAX_LIST_ITEMS);

        m_frame->GetActions()->BeginItems();
        for (int i = 0; i < std::min(actCount, MAX_LIST_ITEMS); ++i) {
            m_frame->GetActions()->AddItem(qspToWxString(items[i].Image), qspToWxString(items[i].Name));
        }
        m_frame->GetActions()->EndItems();
    }
    m_frame->GetActions()->SetSelection(QSPGetSelActionIndex());

    m_frame->GetObjects()->SetIsHtml(m_isHtml);
    if (QSPIsObjectsChanged())
    {
        std::vector<QSPListItem> items(MAX_LIST_ITEMS);

        const int objCount = QSPGetObjects(items.data(), MAX_LIST_ITEMS);

        m_frame->GetObjects()->BeginItems();
        for (int i = 0; i < std::min(objCount, MAX_LIST_ITEMS); ++i) {
            m_frame->GetObjects()->AddItem(qspToWxString(items[i].Image), qspToWxString(items[i].Name));
        }
        m_frame->GetObjects()->EndItems();
    }
    m_frame->GetObjects()->SetSelection(QSPGetSelObjectIndex());

    if (qspGetStr(QSP_FMT("BACKIMAGE"), &strVal) && !qspIsEmpty(strVal))
        m_frame->GetDesc()->LoadBackImage(qspToWxString(strVal));
    else
        m_frame->GetDesc()->LoadBackImage(wxEmptyString);

    m_frame->ApplyParams();
    if (isRedraw)
    {
        m_frame->EnableControls(false, true);
        m_frame->Update();
        wxYieldIfNeeded();
        if (m_frame->ToQuit()) return;
        m_frame->EnableControls(true, true);
    }
    m_frame->GetGameMenu()->Enable(ID_SAVEGAMESTAT, canSave);
    m_frame->GetGameMenu()->Enable(ID_QUICKSAVE, canSave);
}

void QSPCallbacks::SetInputStrText(const QSP_CHAR *text)
{
    if (m_frame->ToQuit()) return;
    m_frame->GetInput()->SetText(qspToWxString(text));
}

QSP_BOOL QSPCallbacks::IsPlay(const QSP_CHAR *file)
{
    const wxString fileName(qspToWxString(file));

    if (
        const auto elem = m_sounds.find(fileName.Upper());
        elem != m_sounds.end() && elem->second.IsPlaying()
    )
        return QSP_TRUE;

    return QSP_FALSE;
}

void QSPCallbacks::CloseFile(const QSP_CHAR *file)
{
    if (file)
    {
        const wxString fileName = qspToWxString(file);
        if (
            const auto elem = m_sounds.find(fileName.Upper());
            elem != m_sounds.end()
        )
        {
            elem->second.Close();
            m_sounds.erase(elem);
        }
    }
    else
    {
        for (auto& [name, sound] : m_sounds) {
            sound.Close();
        }
        m_sounds.clear();
    }
}

void QSPCallbacks::PlayFile(const QSP_CHAR *file, const int volume)
{
    QSPSound snd;
    if (SetVolume(file, volume)) return;
    CloseFile(file);
    const wxString fileName = qspToWxString(file);
    if (
        const wxString filePath = m_frame->ComposeGamePath(fileName);
        !snd.Play(filePath, volume, m_volumeCoeff)
    )
        return;
    UpdateSounds();
    m_sounds.insert(QSPSounds::value_type(fileName.Upper(), snd));
}

void QSPCallbacks::ShowPane(const int type, const QSP_BOOL toShow)
{
    if (m_frame->ToQuit()) return;

    wxWindowID paneId;
    switch (type)
    {
        case QSP_WIN_ACTS: paneId = ID_ACTIONS; break;
        case QSP_WIN_OBJS: paneId = ID_OBJECTS; break;
        case QSP_WIN_VARS: paneId = ID_VARSDESC; break;
        case QSP_WIN_INPUT: paneId = ID_INPUT; break;
        default: return;
    }

    m_frame->ShowPane(paneId, toShow != QSP_FALSE);
}

void QSPCallbacks::Sleep(const int msecs)
{
    if (m_frame->ToQuit()) return;

    const bool canSaveGame = m_frame->GetGameMenu()->IsEnabled(ID_SAVEGAMESTAT);
    const bool canQuicksave = m_frame->GetGameMenu()->IsEnabled(ID_QUICKSAVE);

    bool toBreak = false;

    m_frame->EnableControls(false, true);

    for (auto i = 0; i < msecs / 50; ++i)
    {
        wxThread::Sleep(50);
        m_frame->Update();
        wxYieldIfNeeded();
        if (m_frame->ToQuit() ||
            m_frame->IsKeyPressedWhileDisabled())
        {
            toBreak = true;
            break;
        }
    }

    if (!toBreak)
    {
        wxThread::Sleep(msecs % 50);
        m_frame->Update();
        wxYieldIfNeeded();
    }

    m_frame->EnableControls(true, true);
    m_frame->GetGameMenu()->Enable(ID_SAVEGAMESTAT, canSaveGame);
    m_frame->GetGameMenu()->Enable(ID_QUICKSAVE, canQuicksave);
}

int QSPCallbacks::GetMSCount()
{
    static wxStopWatch stopWatch;
    const int ret = stopWatch.Time();
    stopWatch.Start();
    return ret;
}

void QSPCallbacks::Msg(const QSP_CHAR *str)
{
    if (m_frame->ToQuit()) return;

    QSPMsgDlg dialog(m_frame,
                     wxID_ANY,
                     m_frame->GetDesc()->GetBackgroundColour(),
                     m_frame->GetDesc()->GetForegroundColour(),
                     m_frame->GetDesc()->GetTextFont(),
                     _("Info"),
                     qspToWxString(str),
                     m_isHtml,
                     m_frame
    );
    m_frame->EnableControls(false);
    dialog.ShowModal();
    m_frame->EnableControls(true);
}

int QSPCallbacks::ShowMenu(QSPListItem *items, int count)
{
    if (m_frame->ToQuit()) return -1;

    m_frame->EnableControls(false);
    m_frame->DeleteMenu();

    for (int i = 0; i < count; ++i)
        m_frame->AddMenuItem(qspToWxString(items[i].Name), qspToWxString(items[i].Image));

    const int index = m_frame->ShowMenu();
    m_frame->EnableControls(true);

    return index;
}

void QSPCallbacks::Input(const QSP_CHAR *text, QSP_CHAR *buffer, const int maxLen)
{
    if (m_frame->ToQuit()) return;

    QSPInputDlg dialog(m_frame,
                       wxID_ANY,
                       m_frame->GetDesc()->GetBackgroundColour(),
                       m_frame->GetDesc()->GetForegroundColour(),
                       m_frame->GetDesc()->GetTextFont(),
                       _("Input data"),
                       qspToWxString(text),
                       m_isHtml,
                       m_frame
    );
    m_frame->EnableControls(false);
    dialog.ShowModal();
    m_frame->EnableControls(true);

    const wxString wx_str = dialog.GetText();
    if (wx_str.IsEmpty())
    {
        buffer[0] = 0;
        return;
    }

    wxCharBuffer tempBuffer = wx_str.mb_str(wxMBConvUTF16());
    if (!tempBuffer)
    {
        buffer[0] = 0;
        return;
    }

    const size_t chars_count = tempBuffer.length() / sizeof(QSP_CHAR);
    const size_t copy_chars = chars_count < static_cast<size_t>(maxLen) ? chars_count : static_cast<size_t>(maxLen);

    std::memcpy(buffer, tempBuffer.data(), copy_chars * sizeof(QSP_CHAR));

    buffer[copy_chars] = 0;
}

void QSPCallbacks::ShowImage(const QSP_CHAR *file)
{
    if (m_frame->ToQuit()) return;

    if (!file)
    {
        m_frame->ShowPane(ID_VIEWPIC, false);
    }
    else
    {
        const wxString imgFullPath(m_frame->ComposeGamePath(qspToWxString(file)));
        m_frame->ShowPane(ID_VIEWPIC, m_frame->GetImgView()->OpenFile(imgFullPath));
    }
}

void QSPCallbacks::OpenGame(const QSP_CHAR *file, const QSP_BOOL isAddLocs)
{
    if (m_frame->ToQuit()) return;

    const wxString fullPath = m_frame->ComposeGamePath(qspToWxString(file));
    if (const auto filePath = wxStringToQsp(fullPath))
    {
        if (QSPLoadGameWorldFromFile(filePath.get(), isAddLocs) && !isAddLocs)
            m_frame->UpdateGamePath(fullPath);
    }
}

void QSPCallbacks::OpenGameStatus(const QSP_CHAR *file)
{
    if (m_frame->ToQuit()) return;

    wxString fullPath;
    if (file)
    {
        fullPath = m_frame->ComposeGamePath(qspToWxString(file));
    }
    else
    {
        wxFileDialog dialog(
            m_frame,
            _("Select saved game file"),
            wxEmptyString,
            wxEmptyString,
            _("Saved game files (*.sav)|*.sav"),
            wxFD_OPEN
        );
        m_frame->EnableControls(false);
        const int res = dialog.ShowModal();
        m_frame->EnableControls(true);
        if (res != wxID_OK) return;
        fullPath = dialog.GetPath();
    }

    if (wxFileExists(fullPath))
    {
        if (const auto file_path = wxStringToQsp(fullPath))
        {
            QSPOpenSavedGameFromFile(file_path.get(), QSP_FALSE);
        }
    }
}

void QSPCallbacks::SaveGameStatus(const QSP_CHAR *file)
{
    if (m_frame->ToQuit()) return;

    wxString fullPath;
    if (file)
    {
        fullPath = m_frame->ComposeGamePath(qspToWxString(file));
    } else
    {
        wxFileDialog dialog(
            m_frame,
            _("Select file to save"),
            wxEmptyString,
            wxEmptyString,
            _("Saved game files (*.sav)|*.sav"),
            wxFD_SAVE
        );
        m_frame->EnableControls(false);
        const int res = dialog.ShowModal();
        m_frame->EnableControls(true);
        if (res != wxID_OK) return;
        fullPath = dialog.GetPath();
    }

    if (const auto file_path = wxStringToQsp(fullPath))
    {
        QSPSaveGameAsFile(file_path.get(), QSP_FALSE);
    }
}

bool QSPCallbacks::SetVolume(const QSP_CHAR *file, const int volume)
{
    if (!IsPlay(file)) return false;
    const wxString fileName(qspToWxString(file));
    const auto elem = m_sounds.find(fileName.Upper());

    if (elem != m_sounds.end())
    {
        QSPSound *snd = &elem->second;
        snd->SetVolume(volume, m_volumeCoeff);
        return true;
    }

    return false;
}

void QSPCallbacks::SetOverallVolume(const float coeff)
{
    m_volumeCoeff = std::clamp(coeff, 0.0f, 1.0f);

    for (auto& [name, sound] : m_sounds)
    {
        if (sound.IsPlaying()) {
            sound.SetVolume(sound.Volume, m_volumeCoeff);
        }
    }
}

void QSPCallbacks::UpdateSounds()
{
    std::erase_if(m_sounds, [](auto& pair) {
        if (auto& sound = pair.second;
            !sound.IsPlaying()
        ) {
            sound.Close();
            return true;
        }
        return false;
    });
}
