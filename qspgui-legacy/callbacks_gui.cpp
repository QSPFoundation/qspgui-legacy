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

QSPFrame *QSPCallbacks::m_frame;
bool QSPCallbacks::m_isHtml;
QSPSounds QSPCallbacks::m_sounds;
float QSPCallbacks::m_volumeCoeff;
QSPVersionInfoValues QSPCallbacks::m_versionInfo;

void QSPCallbacks::Init(QSPFrame *frame)
{
    m_frame = frame;
    m_volumeCoeff = 1.0;

    if (sound_init_engine() < 0)
        wxLogError("Can't initialize sound engine");
    else
    {
        const wxString soundFontPath(QSPTools::GetResourcePath(QSP_SOUNDPLUGINS, QSP_MIDISOUNDFONT));
        if (
            const int soundFontInitResult = soundfont_init_w(soundFontPath.c_str());
            soundFontInitResult < 0
        )
            wxLogError("Can't load soundfont to play MIDI files");
    }

    QSPSetCallBack(QSP_CALL_SETTIMER, reinterpret_cast<QSP_CALLBACK>(&SetTimer));
    QSPSetCallBack(QSP_CALL_REFRESHINT, reinterpret_cast<QSP_CALLBACK>(&RefreshInt));
    QSPSetCallBack(QSP_CALL_SETINPUTSTRTEXT, reinterpret_cast<QSP_CALLBACK>(&SetInputStrText));
    QSPSetCallBack(QSP_CALL_ISPLAYINGFILE, reinterpret_cast<QSP_CALLBACK>(&IsPlay));
    QSPSetCallBack(QSP_CALL_PLAYFILE, reinterpret_cast<QSP_CALLBACK>(&PlayFile));
    QSPSetCallBack(QSP_CALL_CLOSEFILE, reinterpret_cast<QSP_CALLBACK>(&CloseFile));
    QSPSetCallBack(QSP_CALL_SHOWMSGSTR, reinterpret_cast<QSP_CALLBACK>(&Msg));
    QSPSetCallBack(QSP_CALL_SLEEP, reinterpret_cast<QSP_CALLBACK>(&Sleep));
    QSPSetCallBack(QSP_CALL_GETMSCOUNT, reinterpret_cast<QSP_CALLBACK>(&GetMSCount));
    QSPSetCallBack(QSP_CALL_SHOWMENU, reinterpret_cast<QSP_CALLBACK>(&ShowMenu));
    QSPSetCallBack(QSP_CALL_INPUTBOX, reinterpret_cast<QSP_CALLBACK>(&Input));
    QSPSetCallBack(QSP_CALL_SHOWIMAGE, reinterpret_cast<QSP_CALLBACK>(&ShowImage));
    QSPSetCallBack(QSP_CALL_SHOWWINDOW, reinterpret_cast<QSP_CALLBACK>(&ShowPane));
    QSPSetCallBack(QSP_CALL_OPENGAME, reinterpret_cast<QSP_CALLBACK>(&OpenGame));
    QSPSetCallBack(QSP_CALL_OPENGAMESTATUS, reinterpret_cast<QSP_CALLBACK>(&OpenGameStatus));
    QSPSetCallBack(QSP_CALL_SAVEGAMESTATUS, reinterpret_cast<QSP_CALLBACK>(&SaveGameStatus));

    /* Prepare version values */
    m_versionInfo["player"] = "Classic";
    m_versionInfo["platform"] = QSPTools::GetPlatform();
}

void QSPCallbacks::DeInit()
{
    CloseFile(nullptr);
    sound_free_engine();
}

int QSPCallbacks::SetTimer(const int msecs)
{
    if (m_frame->ToQuit()) return 0;

    if (msecs)
        m_frame->GetTimer()->Start(msecs);
    else
        m_frame->GetTimer()->Stop();

    return 0;
}

int QSPCallbacks::RefreshInt(const QSP_BOOL isRedraw)
{
    int numVal;
    QSP_CHAR *strVal;
    if (m_frame->ToQuit()) return 0;

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

        const int actionsCount = QSPGetActions(items.data(), MAX_LIST_ITEMS);

        m_frame->GetActions()->BeginItems();
        for (int i = 0; i < actionsCount; ++i) {
            m_frame->GetActions()->AddItem(qspToWxString(items[i].Image), qspToWxString(items[i].Name));
        }
        m_frame->GetActions()->EndItems();
    }
    m_frame->GetActions()->SetSelection(QSPGetSelActionIndex());

    m_frame->GetObjects()->SetIsHtml(m_isHtml);
    if (QSPIsObjectsChanged())
    {
        std::vector<QSPListItem> items(MAX_LIST_ITEMS);

        const int objectsCount = QSPGetObjects(items.data(), MAX_LIST_ITEMS);

        m_frame->GetObjects()->BeginItems();
        for (int i = 0; i < objectsCount; ++i) {
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
        if (m_frame->ToQuit()) return 0;
        m_frame->EnableControls(true, true);
    }
    m_frame->GetGameMenu()->Enable(ID_SAVEGAMESTAT, canSave);
    m_frame->GetGameMenu()->Enable(ID_QUICKSAVE, canSave);

    return 0;
}

int QSPCallbacks::SetInputStrText(QSP_CHAR *text)
{
    if (m_frame->ToQuit()) return 0;
    m_frame->GetInput()->SetText(qspToWxString(text));
    return 0;
}

int QSPCallbacks::IsPlay(const QSP_CHAR *file)
{
    const wxString fileName(qspToWxString(file));

    if (
        const auto elem = m_sounds.find(fileName.Upper());
        elem != m_sounds.end() && elem->second.IsPlaying()
    )
        return QSP_TRUE;

    return QSP_FALSE;
}

int QSPCallbacks::CloseFile(const QSP_CHAR *file)
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

    return 0;
}

int QSPCallbacks::PlayFile(QSP_CHAR *file, int volume)
{
    QSPSound snd;
    if (SetVolume(file, volume)) return 0;
    CloseFile(file);
    const wxString fileName = qspToWxString(file);
    if (
        const wxString filePath = m_frame->ComposeGamePath(fileName);
        !snd.Play(filePath, volume, m_volumeCoeff)
    )
        return 0;
    UpdateSounds();
    m_sounds.insert(QSPSounds::value_type(fileName.Upper(), snd));
    return 0;
}

int QSPCallbacks::ShowPane(const int type, const QSP_BOOL toShow)
{
    if (m_frame->ToQuit()) return 0;

    if (type & QSP_WIN_VARS)
        m_frame->ShowPane(ID_VARSDESC, toShow != QSP_FALSE);
    if (type & QSP_WIN_ACTS)
        m_frame->ShowPane(ID_ACTIONS, toShow != QSP_FALSE);
    if (type & QSP_WIN_OBJS)
        m_frame->ShowPane(ID_OBJECTS, toShow != QSP_FALSE);
    if (type & QSP_WIN_INPUT)
        m_frame->ShowPane(ID_INPUT, toShow != QSP_FALSE);

    return 0;
}

int QSPCallbacks::Sleep(const int msecs)
{
    if (m_frame->ToQuit()) return 0;

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

    return 0;
}

int QSPCallbacks::GetMSCount()
{
    static wxStopWatch stopWatch;
    const int ret = stopWatch.Time();
    stopWatch.Start();
    return ret;
}

int QSPCallbacks::Msg(QSP_CHAR *str)
{
    if (m_frame->ToQuit()) return 0;

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

    return 0;
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

int QSPCallbacks::Input(QSP_CHAR *text, QSP_CHAR *buffer, int maxLen)
{
    if (m_frame->ToQuit()) return 0;

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
        return 0;
    }

    wxCharBuffer tempBuffer = wx_str.mb_str(wxMBConvUTF16());
    if (!tempBuffer)
    {
        buffer[0] = 0;
        return 0;
    }

    const size_t chars_count = tempBuffer.length() / sizeof(QSP_CHAR);
    const size_t copy_chars = chars_count < static_cast<size_t>(maxLen) ? chars_count : static_cast<size_t>(maxLen);

    std::memcpy(buffer, tempBuffer.data(), copy_chars * sizeof(QSP_CHAR));

    buffer[copy_chars] = 0;

    return 0;
}

int QSPCallbacks::ShowImage(QSP_CHAR *file)
{
    if (m_frame->ToQuit()) return 0;

    if (!file)
    {
        m_frame->ShowPane(ID_VIEWPIC, false);
    }
    else
    {
        const wxString imgFullPath(m_frame->ComposeGamePath(qspToWxString(file)));
        m_frame->ShowPane(ID_VIEWPIC, m_frame->GetImgView()->OpenFile(imgFullPath));
    }

    return 0;
}

int QSPCallbacks::OpenGame(QSP_CHAR *file, QSP_BOOL isAddLocs)
{
    if (m_frame->ToQuit()) return 0;

    if (
        const wxString fullPath = m_frame->ComposeGamePath(qspToWxString(file));
        wxFileExists(fullPath)
    )
    {
        if (const auto file_path = wxStringToQsp(fullPath))
        {
            if (QSPLoadGameWorldFromFile(file_path.get(), isAddLocs) && !isAddLocs) {
                m_frame->UpdateGamePath(fullPath);
            }
        }
    }

    return 0;
}

int QSPCallbacks::OpenGameStatus(QSP_CHAR *file)
{
    if (m_frame->ToQuit()) return 0;

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
        if (res != wxID_OK) return 0;
        fullPath = dialog.GetPath();
    }

    if (wxFileExists(fullPath))
    {
        if (const auto file_path = wxStringToQsp(fullPath))
        {
            QSPOpenSavedGameFromFile(file_path.get(), QSP_FALSE);
        }
    }

    return 0;
}

int QSPCallbacks::SaveGameStatus(QSP_CHAR *file)
{
    if (m_frame->ToQuit()) return 0;

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
        if (res != wxID_OK)
            return 0;
        fullPath = dialog.GetPath();
    }

    if (const auto file_path = wxStringToQsp(fullPath))
    {
        QSPSaveGameAsFile(file_path.get(), QSP_FALSE);
    }

    return 0;
}

bool QSPCallbacks::SetVolume(QSP_CHAR *file, int volume)
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
