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
#include <wx/fileconf.h>
#include <wx/cmdline.h>
#include <wx/regex.h>
#include <wx/webrequest.h>
#include <wx/fontenum.h>
#include <wx/fontdlg.h>
#include <wx/colordlg.h>
#include <wx/aboutdlg.h>
#include <wx/aui/aui.h>
#include <qsp_default.h>
#include "transhelper.h"
#include "inputbox.h"
#include "textbox.h"
#include "listbox.h"
#include "imgcanvas.h"
#include "initevent.h"
#include "pathprovider.h"
#include "updateappdialog.h"

#include "qspgui_legacy_config.h"

inline constexpr const char* QSP_VER = QSPGUI_LEGACY_VER_STR;
#define QSP_LOGO wxT("Quest Soft Player Legacy ") QSPGUI_LEGACY_VER_STR

enum CustomIDs : int
{
    ID_BEGOFDYNMENU = wxID_HIGHEST + 1,
    ID_ENDOFDYNMENU = ID_BEGOFDYNMENU + 500,
    ID_OPENGAME,
    ID_NEWGAME,
    ID_OPENGAMESTAT,
    ID_SAVEGAMESTAT,
    ID_QUICKSAVE,
    ID_VOLUME,
    ID_VOLUME0,
    ID_VOLUME20,
    ID_VOLUME40,
    ID_VOLUME60,
    ID_VOLUME80,
    ID_VOLUME100,
    ID_FONT,
    ID_SELECTFONT,
    ID_USEFONTSIZE,
    ID_COLORS,
    ID_SELECTFONTCOLOR,
    ID_SELECTBACKCOLOR,
    ID_SELECTLINKCOLOR,
    ID_CHECKUPDATESONSTARTUP,
    ID_CHECKUPDATES,
    ID_SELECTLANG,
    ID_TOGGLEWINMODE,
    ID_TOGGLEOBJS,
    ID_TOGGLEACTS,
    ID_TOGGLEDESC,
    ID_TOGGLEINPUT,
    ID_TOGGLECAPTIONS,
    ID_TOGGLEHOTKEYS,
    ID_SHOWHIDE,
    ID_MAINDESC,
    ID_VARSDESC,
    ID_OBJECTS,
    ID_ACTIONS,
    ID_VIEWPIC,
    ID_INPUT,
    ID_TIMER,
    ID_DUMMY
};

enum AppUpdateType
{
    UPDATE_SHOW_ONLY_NEW,
    UPDATE_SHOW_ALL_RESULTS
};

class QSPFrame : public wxFrame, public PathProvider
{
public:
    QSPFrame(const wxString &configPath, QSPTranslationHelper *transHelper);
    ~QSPFrame() override;

    void SaveSettings();
    void LoadSettings();
    void EnableControls(bool status, bool isExtended = false);
    void ShowPane(wxWindowID id, bool toShow);
    void ApplyParams();
    void DeleteMenu();
    void AddMenuItem(const wxString &name, const wxString &imgPath);
    int ShowMenu();
    void UpdateGamePath(const wxString &fullPath);

    [[nodiscard]] wxString ComposeGamePath(const wxString &relativePath) const override;
    [[nodiscard]] wxString GetGamePath() const override { return m_worldPath; }
    [[nodiscard]] bool IsValidFullPath(const wxString &path) const override;

    [[nodiscard]] wxTimer* GetTimer() const { return m_timer; }
    [[nodiscard]] QSPTextBox* GetDesc() const { return m_desc; }
    [[nodiscard]] QSPTextBox* GetVars() const { return m_vars; }
    [[nodiscard]] QSPInputBox* GetInput() const { return m_input; }
    [[nodiscard]] QSPListBox* GetActions() const { return m_actions; }
    [[nodiscard]] QSPListBox* GetObjects() const { return m_objects; }
    [[nodiscard]] QSPImgCanvas* GetImgView() const { return m_imgView; }
    [[nodiscard]] wxMenu* GetGameMenu() const { return m_gameMenu; }

    [[nodiscard]] bool ToShowHotkeys() const { return m_toShowHotkeys; }
    [[nodiscard]] bool ToCheckUpdates() const { return m_toCheckUpdates; }
    [[nodiscard]] bool ToQuit() const { return m_toQuit; }
    [[nodiscard]] bool IsKeyPressedWhileDisabled() const { return m_keyPressedWhileDisabled; }

    void CheckLatestVersion(int type);
    void ProcessVersionResult(const wxString &versionInfo, int type);

protected:
    void ShowError();
    void UpdateTitle();
    void ReCreateGUI();
    void RefreshUI();
    void ApplyFont(const wxFont& font);
    bool ApplyFontSize(int size);
    bool ApplyFontName(const wxString& name);
    bool ApplyFontColor(const wxColour& color);
    bool ApplyBackColor(const wxColour& color);
    bool ApplyLinkColor(const wxColour& color);
    void CallPaneFunc(wxWindowID id, QSP_BOOL toShow) const;
    void TogglePane(wxWindowID id);
    void SetOverallVolume(int percents);
    void OpenGameFile(const wxString& fullPath);
    void OpenGameState(const wxString& fullPath);
    void SaveGameState(const wxString& fullPath);

    void OnVersionRequestState(const wxWebRequestEvent &event);
    void OnInit(const wxInitEvent &event);
    void OnClose(wxCloseEvent &event);
    void OnTimer(wxTimerEvent &event);
    void OnMenu(wxCommandEvent &event);
    void OnQuit(wxCommandEvent &event);
    void OnOpenGame(wxCommandEvent &event);
    void OnNewGame(wxCommandEvent &event);
    void OnOpenGameStat(wxCommandEvent &event);
    void OnSaveGameStat(wxCommandEvent &event);
    void OnQuickSave(wxCommandEvent &event);
    void OnSelectFont(wxCommandEvent &event);
    void OnUseFontSize(wxCommandEvent &event);
    void OnSelectFontColor(wxCommandEvent &event);
    void OnSelectBackColor(wxCommandEvent &event);
    void OnSelectLinkColor(wxCommandEvent &event);
    void OnCheckUpdatesOnStartup(wxCommandEvent &event);
    void OnSelectLang(wxCommandEvent &event);
    void OnToggleWinMode(wxCommandEvent &event);
    void OnToggleObjs(wxCommandEvent &event);
    void OnToggleActs(wxCommandEvent &event);
    void OnToggleDesc(wxCommandEvent &event);
    void OnToggleInput(wxCommandEvent &event);
    void OnToggleCaptions(wxCommandEvent &event);
    void OnToggleHotkeys(wxCommandEvent &event);
    void OnVolume(const wxCommandEvent &event);
    void OnCheckUpdates(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
    void OnLinkClicked(wxHtmlLinkEvent &event);
    void OnObjectChange(const wxCommandEvent &event);
    void OnActionChange(const wxCommandEvent &event);
    void OnActionDblClick(wxCommandEvent &event);
    void OnInputTextChange(const wxCommandEvent &event);
    void OnInputTextEnter(wxCommandEvent &event);
    void OnKey(wxKeyEvent &event);
    void OnMouseClick(wxMouseEvent &event);
    void OnWheel(const wxMouseEvent &event);
    void OnPaneClose(wxAuiManagerEvent &event);
    void OnDropFiles(const wxDropFilesEvent &event);

private:
    bool m_isGameOpened{false};
    wxString m_worldPath{};
    wxString m_savedGamePath{};
    wxString m_configPath{};
    wxString m_configDefPath{};
    QSPTranslationHelper* m_transHelper{nullptr};
    wxTimer* m_timer{nullptr};
    QSPTextBox* m_desc{nullptr};
    QSPTextBox* m_vars{nullptr};
    QSPInputBox* m_input{nullptr};
    QSPListBox* m_objects{nullptr};
    QSPListBox* m_actions{nullptr};
    QSPImgCanvas* m_imgView{nullptr};
    wxMenu* m_gameMenu{nullptr};
    int m_menuItemId{ID_BEGOFDYNMENU};
    wxMenu* m_menu{nullptr};
    wxMenu* m_fileMenu{nullptr};
    wxMenu* m_settingsMenu{nullptr};
    wxAuiManager* m_manager{nullptr};

    wxColour m_backColor{*wxWHITE};
    wxColour m_linkColor{*wxBLUE};
    wxColour m_fontColor{*wxBLACK};
    int m_fontSize{12};
    wxString m_fontName{};

    bool m_toUseFontSize{false};
    bool m_toProcessEvents{false};
    bool m_toQuit{false};
    bool m_keyPressedWhileDisabled{false};
    bool m_toShowHotkeys{false};
    bool m_toCheckUpdates{true};
    int m_volume{100};
    int m_menuIndex{-1};

    DECLARE_CLASS(QSPFrame)
};
