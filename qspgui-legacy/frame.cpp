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

#include "frame.h"
#include "comtools.h"
#include "callbacks_gui.h"
#include <wx/wupdlock.h>
#include "icons/logo.xpm"
#include "icons/logo_big.xpm"
#include "icons/open.xpm"
#include "icons/new.xpm"
#include "icons/exit.xpm"
#include "icons/statusopen.xpm"
#include "icons/statussave.xpm"
#include "icons/windowmode.xpm"
#include "icons/about.xpm"

wxIMPLEMENT_CLASS(QSPFrame, wxFrame);

QSPFrame::QSPFrame(const wxString &configPath, QSPTranslationHelper *transHelper) :
    wxFrame(nullptr, wxID_ANY, wxEmptyString),
    m_configDefPath(configPath),
    m_configPath(configPath),
    m_transHelper(transHelper)
{
    wxRegisterId(ID_DUMMY);

    Bind(wxEVT_WEBREQUEST_STATE, &QSPFrame::OnVersionRequestState, this);
    Bind(wxEVT_CLOSE_WINDOW, &QSPFrame::OnClose, this);
    Bind(wxEVT_TIMER, &QSPFrame::OnTimer, this, ID_TIMER);
    Bind(wxEVT_MENU, &QSPFrame::OnQuit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &QSPFrame::OnOpenGame, this, ID_OPENGAME);
    Bind(wxEVT_MENU, &QSPFrame::OnNewGame, this, ID_NEWGAME);
    Bind(wxEVT_MENU, &QSPFrame::OnOpenGameStat, this, ID_OPENGAMESTAT);
    Bind(wxEVT_MENU, &QSPFrame::OnSaveGameStat, this, ID_SAVEGAMESTAT);
    Bind(wxEVT_MENU, &QSPFrame::OnSelectFont, this, ID_SELECTFONT);
    Bind(wxEVT_MENU, &QSPFrame::OnUseFontSize, this, ID_USEFONTSIZE);
    Bind(wxEVT_MENU, &QSPFrame::OnSelectFontColor, this, ID_SELECTFONTCOLOR);
    Bind(wxEVT_MENU, &QSPFrame::OnSelectBackColor, this, ID_SELECTBACKCOLOR);
    Bind(wxEVT_MENU, &QSPFrame::OnSelectLinkColor, this, ID_SELECTLINKCOLOR);
    Bind(wxEVT_MENU, &QSPFrame::OnCheckUpdatesOnStartup, this, ID_CHECKUPDATESONSTARTUP);
    Bind(wxEVT_MENU, &QSPFrame::OnSelectLang, this, ID_SELECTLANG);
    Bind(wxEVT_MENU, &QSPFrame::OnToggleWinMode, this, ID_TOGGLEWINMODE);
    Bind(wxEVT_MENU, &QSPFrame::OnToggleObjs, this, ID_TOGGLEOBJS);
    Bind(wxEVT_MENU, &QSPFrame::OnToggleActs, this, ID_TOGGLEACTS);
    Bind(wxEVT_MENU, &QSPFrame::OnToggleDesc, this, ID_TOGGLEDESC);
    Bind(wxEVT_MENU, &QSPFrame::OnToggleInput, this, ID_TOGGLEINPUT);
    Bind(wxEVT_MENU, &QSPFrame::OnToggleCaptions, this, ID_TOGGLECAPTIONS);
    Bind(wxEVT_MENU, &QSPFrame::OnVolume, this, ID_VOLUME0);
    Bind(wxEVT_MENU, &QSPFrame::OnVolume, this, ID_VOLUME20);
    Bind(wxEVT_MENU, &QSPFrame::OnVolume, this, ID_VOLUME40);
    Bind(wxEVT_MENU, &QSPFrame::OnVolume, this, ID_VOLUME60);
    Bind(wxEVT_MENU, &QSPFrame::OnVolume, this, ID_VOLUME80);
    Bind(wxEVT_MENU, &QSPFrame::OnVolume, this, ID_VOLUME100);
    Bind(wxEVT_MENU, &QSPFrame::OnCheckUpdates, this, ID_CHECKUPDATES);
    Bind(wxEVT_MENU, &QSPFrame::OnAbout, this, wxID_ABOUT);

    Bind(wxEVT_HTML_LINK_CLICKED, &QSPFrame::OnLinkClicked, this, ID_MAINDESC);
    Bind(wxEVT_HTML_LINK_CLICKED, &QSPFrame::OnLinkClicked, this, ID_VARSDESC);
    Bind(wxEVT_LISTBOX, &QSPFrame::OnObjectChange, this, ID_OBJECTS);
    Bind(wxEVT_LISTBOX, &QSPFrame::OnActionChange, this, ID_ACTIONS);
    Bind(wxEVT_LISTBOX_DCLICK, &QSPFrame::OnActionDblClick, this, ID_ACTIONS);
    Bind(wxEVT_TEXT, &QSPFrame::OnInputTextChange, this, ID_INPUT);
    Bind(wxEVT_TEXT_ENTER, &QSPFrame::OnInputTextEnter, this, ID_INPUT);
    Bind(wxEVT_KEY_UP, &QSPFrame::OnKey, this);
    Bind(wxEVT_MOUSEWHEEL, &QSPFrame::OnWheel, this);
    Bind(wxEVT_LEFT_DOWN, &QSPFrame::OnMouseClick, this);
    Bind(wxEVT_AUI_PANE_CLOSE, &QSPFrame::OnPaneClose, this);
    Bind(wxEVT_DROP_FILES, &QSPFrame::OnDropFiles, this);

    SetIcon(wxICON(logo));
    wxWindow::DragAcceptFiles(true);
    m_timer = new wxTimer{this, ID_TIMER};
    m_menu = new wxMenu;

    auto* menuBar = new wxMenuBar;
    m_fileMenu = new wxMenu;

    auto* fileOpenItem = new wxMenuItem{m_fileMenu, ID_OPENGAME, "-"};
    fileOpenItem->SetBitmap(wxBitmap{open_xpm});
    m_fileMenu->Append(fileOpenItem);

    auto *fileNewItem = new wxMenuItem{m_fileMenu, ID_NEWGAME, "-"};
    fileNewItem->SetBitmap(wxBitmap(new_xpm));
    m_fileMenu->Append(fileNewItem);
    m_fileMenu->AppendSeparator();

    auto *fileExitItem = new wxMenuItem{m_fileMenu, wxID_EXIT};
    fileExitItem->SetBitmap(wxBitmap(exit_xpm));
    m_fileMenu->Append(fileExitItem);

    m_gameMenu = new wxMenu;
    auto *gameOpenItem = new wxMenuItem{m_gameMenu, ID_OPENGAMESTAT, "-"};
    gameOpenItem->SetBitmap(wxBitmap(statusopen_xpm));
    m_gameMenu->Append(gameOpenItem);
    m_gameMenu->Append(ID_SAVEGAMESTAT, "-");

    auto *gameSaveItem = new wxMenuItem{m_gameMenu, ID_QUICKSAVE, "-"};
    gameSaveItem->SetBitmap(wxBitmap(statussave_xpm));
    m_gameMenu->Append(gameSaveItem);

    auto *wndsMenu = new wxMenu;
    wndsMenu->Append(ID_TOGGLEOBJS, "-");
    wndsMenu->Append(ID_TOGGLEACTS, "-");
    wndsMenu->Append(ID_TOGGLEDESC, "-");
    wndsMenu->Append(ID_TOGGLEINPUT, "-");
    wndsMenu->AppendSeparator();
    wndsMenu->Append(ID_TOGGLECAPTIONS, "-");
    wndsMenu->Append(ID_TOGGLEHOTKEYS, "-");

    auto *fontMenu = new wxMenu;
    fontMenu->Append(ID_SELECTFONT, "-");
    fontMenu->AppendCheckItem(ID_USEFONTSIZE, "-");

    auto *colorsMenu = new wxMenu;
    colorsMenu->Append(ID_SELECTFONTCOLOR, "-");
    colorsMenu->Append(ID_SELECTBACKCOLOR, "-");
    colorsMenu->Append(ID_SELECTLINKCOLOR, "-");

    auto *volumeMenu = new wxMenu;
    volumeMenu->AppendRadioItem(ID_VOLUME0, "-");
    volumeMenu->AppendRadioItem(ID_VOLUME20, "-");
    volumeMenu->AppendRadioItem(ID_VOLUME40, "-");
    volumeMenu->AppendRadioItem(ID_VOLUME60, "-");
    volumeMenu->AppendRadioItem(ID_VOLUME80, "-");
    volumeMenu->AppendRadioItem(ID_VOLUME100, "-");

    m_settingsMenu = new wxMenu;
    m_settingsMenu->Append(ID_SHOWHIDE, "-", wndsMenu);
    m_settingsMenu->Append(ID_FONT, "-", fontMenu);
    m_settingsMenu->Append(ID_COLORS, "-", colorsMenu);
    m_settingsMenu->Append(ID_VOLUME, "-", volumeMenu);
    m_settingsMenu->AppendCheckItem(ID_CHECKUPDATESONSTARTUP, "-");
    m_settingsMenu->AppendSeparator();

    auto *settingsWinModeItem = new wxMenuItem{m_settingsMenu, ID_TOGGLEWINMODE, "-"};
    settingsWinModeItem->SetBitmap(wxBitmap(windowmode_xpm));
    m_settingsMenu->Append(settingsWinModeItem);
    m_settingsMenu->AppendSeparator();
    m_settingsMenu->Append(ID_SELECTLANG, "-");

    auto *helpMenu = new wxMenu;
    helpMenu->Append(ID_CHECKUPDATES, "-");
    helpMenu->AppendSeparator();
    auto *helpAboutItem = new wxMenuItem{helpMenu, wxID_ABOUT, "-"};
    helpAboutItem->SetBitmap(wxBitmap(about_xpm));
    helpMenu->Append(helpAboutItem);

    menuBar->Append(m_fileMenu, "-");
    menuBar->Append(m_gameMenu, "-");
    menuBar->Append(m_settingsMenu, "-");
    menuBar->Append(helpMenu, "-");
    wxFrameBase::SetMenuBar(menuBar);

    m_manager = new wxAuiManager(this);
    m_manager->SetDockSizeConstraint(0.5, 0.5);
    m_imgView = new QSPImgCanvas(this, ID_VIEWPIC);
    m_manager->AddPane(
        m_imgView,
        wxAuiPaneInfo().Name("imgview")
        .MinSize(FromDIP(wxSize{50, 50}))
        .BestSize(FromDIP(wxSize{150, 150}))
        .Top().MaximizeButton().Hide()
    );
    m_desc = new QSPTextBox(this, ID_MAINDESC);
    m_manager->AddPane(m_desc, wxAuiPaneInfo().Name("desc").CenterPane());
    m_objects = new QSPListBox(this, ID_OBJECTS);
    m_manager->AddPane(
        m_objects,
        wxAuiPaneInfo().Name("objs")
        .MinSize(FromDIP(wxSize{50, 50}))
        .BestSize(FromDIP(wxSize{100, 100}))
        .Right().MaximizeButton()
    );
    m_actions = new QSPListBox(this, ID_ACTIONS, ListBoxType::Extended);
    m_manager->AddPane(
        m_actions,
        wxAuiPaneInfo().Name("acts")
        .MinSize(FromDIP(wxSize{50, 50}))
        .BestSize(FromDIP(wxSize{100, 100}))
        .Bottom().MaximizeButton()
    );
    m_vars = new QSPTextBox(this, ID_VARSDESC);
    m_manager->AddPane(
        m_vars,
        wxAuiPaneInfo().Name("vars")
        .MinSize(FromDIP(wxSize{50, 50}))
        .BestSize(FromDIP(wxSize{100, 100}))
        .Bottom().MaximizeButton()
    );
    m_input = new QSPInputBox(this, ID_INPUT);
    m_manager->AddPane(
        m_input,
        wxAuiPaneInfo().Name("input")
        .MinSize(FromDIP(wxSize{50, 20}))
        .BestSize(FromDIP(wxSize{100, 20}))
        .Bottom().Layer(1)
    );

    m_desc->SetPathProvider(this);
    m_objects->SetPathProvider(this);
    m_actions->SetPathProvider(this);
    m_vars->SetPathProvider(this);

    m_savedGamePath.Clear();
    m_worldPath.Clear();
    m_toQuit = false;
    m_keyPressedWhileDisabled = false;
    m_isGameOpened = false;

    wxWindowBase::SetMinClientSize(FromDIP(wxSize{450, 300}));
    SetOverallVolume(100);
}

QSPFrame::~QSPFrame()
{
    m_manager->UnInit();
    delete m_manager;
    delete m_menu;
    delete m_timer;
}

void QSPFrame::SaveSettings()
{
    int x, y, w, h;
    bool isMaximized;
    if (IsFullScreen()) ShowFullScreen(false);
    if (IsIconized()) Iconize(false);
    if ((isMaximized = IsMaximized())) Maximize(false);
    wxFileConfig cfg(wxEmptyString, wxEmptyString, m_configPath);
    cfg.Write("Colors/BackColor", m_backColor.Blue() << 16 | m_backColor.Green() << 8 | m_backColor.Red());
    cfg.Write("Colors/FontColor", m_fontColor.Blue() << 16 | m_fontColor.Green() << 8 | m_fontColor.Red());
    cfg.Write("Colors/LinkColor", m_linkColor.Blue() << 16 | m_linkColor.Green() << 8 | m_linkColor.Red());
    cfg.Write("Font/FontSize", m_fontSize);
    cfg.Write("Font/FontName", m_fontName);
    cfg.Write("Font/UseFontSize", m_toUseFontSize);
    cfg.Write("General/Volume", m_volume);
    cfg.Write("General/ShowHotkeys", m_toShowHotkeys);
    cfg.Write("General/Panels", m_manager->SavePerspective());
    cfg.Write("General/CheckUpdates", m_toCheckUpdates);
    m_transHelper->Save(cfg, "General/Language");
    GetPosition(&x, &y);
    GetClientSize(&w, &h);
    cfg.Write("Pos/Left", x);
    cfg.Write("Pos/Top", y);
    cfg.Write("Pos/Width", w);
    cfg.Write("Pos/Height", h);
    cfg.Write("Pos/Maximize", isMaximized);
}

void QSPFrame::LoadSettings()
{
    bool toMaximize;
    int x, y, w, h, temp;
    Hide();
    const wxFileConfig cfg(wxEmptyString, wxEmptyString, m_configPath);
    cfg.Read("Colors/BackColor", &temp, 0xE0E0E0);
    m_backColor = wxColour(temp);
    cfg.Read("Colors/FontColor", &temp, 0x000000);
    m_fontColor = wxColour(temp);
    cfg.Read("Colors/LinkColor", &temp, 0xFF0000);
    m_linkColor = wxColour(temp);
    temp = wxNORMAL_FONT->GetPointSize();
    if (temp < 12) temp = 12;
    cfg.Read(wxT("Font/FontSize"), &m_fontSize, temp);
    cfg.Read(wxT("Font/FontName"), &m_fontName, wxNORMAL_FONT->GetFaceName());
    cfg.Read(wxT("Font/UseFontSize"), &m_toUseFontSize, false);
    cfg.Read(wxT("General/ShowHotkeys"), &m_toShowHotkeys, false);
    cfg.Read(wxT("General/Volume"), &m_volume, 100);
    cfg.Read(wxT("Pos/Left"), &x, 10);
    cfg.Read(wxT("Pos/Top"), &y, 10);
    cfg.Read(wxT("Pos/Width"), &w, 850);
    cfg.Read(wxT("Pos/Height"), &h, 650);
    cfg.Read(wxT("Pos/Maximize"), &toMaximize, false);
    wxString panels(wxT("layout2|") \
        wxT("name=imgview;state=1080035327;dir=1;layer=0;row=0;pos=0;prop=100000;bestw=832;besth=150;minw=50;minh=50;maxw=-1;maxh=-1;floatx=175;floaty=148;floatw=518;floath=372|") \
        wxT("name=desc;state=768;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=613;besth=341;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|") \
        wxT("name=objs;state=6293500;dir=2;layer=0;row=0;pos=0;prop=100000;bestw=213;besth=324;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|") \
        wxT("name=acts;state=6293500;dir=3;layer=0;row=0;pos=0;prop=117349;bestw=475;besth=185;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|") \
        wxT("name=vars;state=6293500;dir=3;layer=0;row=0;pos=1;prop=82651;bestw=351;besth=185;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|") \
        wxT("name=input;state=2099196;dir=3;layer=1;row=0;pos=0;prop=100000;bestw=832;besth=22;minw=50;minh=20;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|") \
        wxT("dock_size(5,0,0)=22|dock_size(2,0,0)=215|dock_size(3,0,0)=204|dock_size(3,1,0)=41|"));
    cfg.Read(wxT("General/Panels"), &panels);
    cfg.Read(wxT("General/CheckUpdates"), &m_toCheckUpdates, true);
    m_transHelper->Load(cfg, wxT("General/Language"));

    SetOverallVolume(m_volume);
    ApplyBackColor(m_backColor);
    ApplyFontColor(m_fontColor);
    ApplyLinkColor(m_linkColor);
    ApplyFontSize(m_fontSize);
    if (!ApplyFontName(m_fontName))
    {
        m_fontName = wxNORMAL_FONT->GetFaceName();
        ApplyFontName(m_fontName);
    }
    RefreshUI();
    m_settingsMenu->Check(ID_USEFONTSIZE, m_toUseFontSize);
    m_settingsMenu->Check(ID_CHECKUPDATESONSTARTUP, m_toCheckUpdates);
    m_manager->LoadPerspective(panels);
    m_manager->RestoreMaximizedPane();

    const wxSize winSize(ClientToWindowSize(wxSize(w, h)));
    w = winSize.GetWidth();
    h = winSize.GetHeight();
    const wxRect dispRect(wxGetClientDisplayRect());
    if (w > dispRect.GetWidth()) w = dispRect.GetWidth();
    if (h > dispRect.GetHeight()) h = dispRect.GetHeight();
    if (x < dispRect.GetLeft()) x = dispRect.GetLeft();
    if (y < dispRect.GetTop()) y = dispRect.GetTop();
    if (x + w - 1 > dispRect.GetRight()) x = dispRect.GetRight() - w + 1;
    if (y + h - 1 > dispRect.GetBottom()) y = dispRect.GetBottom() - h + 1;

    SetSize(x, y, w, h);
    ShowPane(ID_VIEWPIC, false);
    ShowPane(ID_ACTIONS, true);
    ShowPane(ID_OBJECTS, true);
    ShowPane(ID_VARSDESC, true);
    ShowPane(ID_INPUT, true);
    ReCreateGUI();
    if (toMaximize) Maximize();
    Show();

    m_manager->Update();
}

void QSPFrame::EnableControls(bool status, bool isExtended)
{
    if (isExtended) m_fileMenu->Enable(ID_OPENGAME, status);
    m_fileMenu->Enable(ID_NEWGAME, status);
    m_gameMenu->Enable(ID_OPENGAMESTAT, status);
    m_gameMenu->Enable(ID_SAVEGAMESTAT, status);
    m_gameMenu->Enable(ID_QUICKSAVE, status);
    m_settingsMenu->Enable(ID_TOGGLEOBJS, status);
    m_settingsMenu->Enable(ID_TOGGLEACTS, status);
    m_settingsMenu->Enable(ID_TOGGLEDESC, status);
    m_settingsMenu->Enable(ID_TOGGLEINPUT, status);
    m_objects->Enable(status);
    m_actions->Enable(status);
    m_input->SetEditable(status);
    m_toProcessEvents = status;
    m_keyPressedWhileDisabled = false;
}

void QSPFrame::ShowPane(const wxWindowID id, const bool toShow)
{
    const wxAuiPaneInfoArray& allPanes = m_manager->GetAllPanes();

    wxWindowUpdateLocker noUpdates{this};

    const wxAuiPaneInfo* maximizedPane = nullptr;
    wxAuiPaneInfo* pane = nullptr;

    for (auto& currentPane : allPanes)
    {
        if (currentPane.window && currentPane.window->GetId() == id)
            pane = &currentPane;

        if (currentPane.IsMaximized())
            maximizedPane = &currentPane;
    }

    if (pane)
    {
        if (maximizedPane)
        {
            if (maximizedPane == pane)
            {
                if (!toShow)
                {
                    m_manager->RestorePane(*pane);
                    pane->Hide();
                    m_manager->Update();
                }
            }
            else if (pane->HasFlag(wxAuiPaneInfo::savedHiddenState) == toShow)
            {
                pane->SetFlag(wxAuiPaneInfo::savedHiddenState, !toShow);
            }
        }
        else if (pane->IsShown() != toShow)
        {
            pane->Show(toShow);
            m_manager->Update();
        }
    }
}

void QSPFrame::ApplyParams()
{
    int numVal;
    QSP_CHAR *strVal;
    int setFontSize;
    bool toRefreshUI = false;

    if (
        const wxColour setBackColor = qspGetVar(QSP_FMT("BCOLOR"), &numVal) && numVal ? wxColour(numVal) : m_backColor;
        setBackColor != m_desc->GetBackgroundColour()
    )
    {
        if (ApplyBackColor(setBackColor)) toRefreshUI = true;
    }

    if (
        const wxColour setFontColor = qspGetVar(QSP_FMT("FCOLOR"), &numVal) && numVal ? wxColour(numVal): m_fontColor;
        setFontColor != m_desc->GetForegroundColour())
    {
        if (ApplyFontColor(setFontColor)) toRefreshUI = true;
    }

    if (
        const wxColour setLinkColor = qspGetVar(QSP_FMT("LCOLOR"), &numVal) && numVal ? wxColour(numVal): m_linkColor;
        setLinkColor != m_desc->GetLinkColor()
    )
    {
        if (ApplyLinkColor(setLinkColor)) toRefreshUI = true;
    }

    if (m_toUseFontSize)
    {
        setFontSize = m_fontSize;
    }
    else
    {
        setFontSize =  qspGetVar(QSP_FMT("FSIZE"), &numVal) && numVal ? numVal : m_fontSize;
    }

    if (setFontSize != m_desc->GetTextFont().GetPointSize())
    {
        if (ApplyFontSize(setFontSize)) toRefreshUI = true;
    }


    if (
        const wxString setFontName = qspGetStr(QSP_FMT("FNAME"), &strVal) && !qspIsEmpty(strVal) ? qspToWxString(strVal): m_fontName;
        !setFontName.IsSameAs(m_desc->GetTextFont().GetFaceName(), false)
    )
    {
        if (ApplyFontName(setFontName))
            toRefreshUI = true;
        else if (!m_fontName.IsSameAs(m_desc->GetTextFont().GetFaceName(), false))
        {
            if (ApplyFontName(m_fontName)) toRefreshUI = true;
        }
    }

    if (toRefreshUI) RefreshUI();
}

void QSPFrame::DeleteMenu()
{
    delete m_menu;
    m_menu = new wxMenu;
    m_menuItemId = ID_BEGOFDYNMENU;
}

void QSPFrame::AddMenuItem(const wxString &name, const wxString &imgPath)
{
    Connect(m_menuItemId, wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(QSPFrame::OnMenu));

    if (name == "-")
    {
        m_menu->AppendSeparator();
    }
    else
    {
        auto *item = new wxMenuItem(m_menu, m_menuItemId, name);
        if (
            const wxString imageFullPath(ComposeGamePath(imgPath));
            wxFileExists(imageFullPath)
        )
        {
            if (
                const wxBitmap itemBmp(imageFullPath, wxBITMAP_TYPE_ANY);
                itemBmp.IsOk()
            ) item->SetBitmap(itemBmp);
        }
        m_menu->Append(item);
    }

    ++m_menuItemId;
}

int QSPFrame::ShowMenu()
{
    m_menuIndex = -1;
    PopupMenu(m_menu);
    return m_menuIndex;
}

void QSPFrame::UpdateGamePath(const wxString &fullPath)
{
    const wxFileName fileName(fullPath, wxPATH_DOS);
    m_worldPath = fileName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

wxString QSPFrame::ComposeGamePath(const wxString &relativePath) const
{
    if (relativePath.IsEmpty())
        return wxEmptyString;

    wxFileName fullPath(m_worldPath + relativePath, wxPATH_DOS);
    fullPath.MakeAbsolute();
    if (
        wxString normalizedPath(fullPath.GetFullPath());
        normalizedPath.StartsWith(m_worldPath)
    )
        return normalizedPath;

    return wxEmptyString;
}

bool QSPFrame::IsValidFullPath(const wxString &path) const
{
    if (path.IsEmpty())
        return true;

    wxFileName fullPath(path);
    fullPath.MakeAbsolute();
    if (fullPath.GetFullPath().StartsWith(m_worldPath))
        return true;

    return false;
}

void QSPFrame::ShowError()
{
    if (m_toQuit) return;
    QSPErrorInfo errorInfo = QSPGetLastErrorData();
    if (!errorInfo.ErrorNum) return; // error is undefined
    wxString locName(qspToWxString(errorInfo.LocName));
    wxString errorDesc(qspToWxString(errorInfo.ErrorDesc));
    wxString line(qspToWxString(errorInfo.IntLine));
    if (line.IsEmpty())
        line = _("Unknown");

    wxString wxMessage;
    if (!locName.IsEmpty())
        wxMessage = wxString::Format(
            _("Location: %s\nArea: %s\nLine %d: %s\nCode: %d\nDesc: %s"),
            locName.wx_str(),
            (errorInfo.ActIndex < 0 ? _("on visit").wx_str() : _("on action").wx_str()),
            errorInfo.TopLineNum,
            line.wx_str(),
            errorInfo.ErrorNum,
            wxGetTranslation(errorDesc).wx_str()
        );
    else
        wxMessage = wxString::Format(
            _("Line %d: %s\nCode: %d\nDesc: %s"),
            errorInfo.IntLineNum,
            line.wx_str(),
            errorInfo.ErrorNum,
            wxGetTranslation(errorDesc).wx_str()
        );
    QSPMsgDlg dialog(this,
                     wxID_ANY,
                     m_desc->GetBackgroundColour(),
                     m_desc->GetForegroundColour(),
                     m_desc->GetTextFont(),
                     _("Error"),
                     wxMessage,
                     false,
                     this
    );
    const bool oldToProcessEvents = m_toProcessEvents;
    m_toProcessEvents = false;
    dialog.ShowModal();
    m_toProcessEvents = oldToProcessEvents;
    if (m_isGameOpened) QSPCallbacks::RefreshInt(QSP_FALSE, QSP_FALSE);
}

void QSPFrame::UpdateTitle()
{
    wxString title(QSP_LOGO);
    #ifdef _DEBUG
        title = wxString::Format(wxT("%s (DEBUG)"), title.wx_str());
    #endif
    if (m_configPath != m_configDefPath)
        title = wxString::Format(wxT("%s [+]"), title.wx_str());
    SetTitle(title);
}

void QSPFrame::ReCreateGUI()
{
    wxMenuBar *menuBar = GetMenuBar();
    UpdateTitle();

    menuBar->SetMenuLabel(0, _("&Quest"));
    menuBar->SetMenuLabel(1, _("&Game"));
    menuBar->SetMenuLabel(2, _("&Settings"));
    menuBar->SetMenuLabel(3, _("&Help"));

    menuBar->SetLabel(ID_OPENGAME, _("&Open game...\tAlt-O"));
    menuBar->SetLabel(ID_NEWGAME, _("&Restart game\tAlt-N"));
    menuBar->SetLabel(wxID_EXIT, _("&Quit\tAlt-X"));
    menuBar->SetLabel(ID_OPENGAMESTAT, _("&Open saved game...\tCtrl-O"));
    menuBar->SetLabel(ID_SAVEGAMESTAT, _("&Save game..."));
    menuBar->SetLabel(ID_QUICKSAVE, _("&Quicksave\tCtrl-S"));
    menuBar->SetLabel(ID_TOGGLEOBJS, _("&Objects\tCtrl-1"));
    menuBar->SetLabel(ID_TOGGLEACTS, _("&Actions\tCtrl-2"));
    menuBar->SetLabel(ID_TOGGLEDESC, _("A&dditional desc\tCtrl-3"));
    menuBar->SetLabel(ID_TOGGLEINPUT, _("&Input area\tCtrl-4"));
    menuBar->SetLabel(ID_TOGGLECAPTIONS, _("&Captions\tCtrl-5"));
    menuBar->SetLabel(ID_TOGGLEHOTKEYS, _("&Hotkeys for actions\tCtrl-6"));
    menuBar->SetLabel(ID_SHOWHIDE, _("&Show / Hide"));
    menuBar->SetLabel(ID_FONT, _("&Font"));
    menuBar->SetLabel(ID_SELECTFONT, _("Select &font...\tAlt-F"));
    menuBar->SetLabel(ID_USEFONTSIZE, _("&Always use selected font size"));
    menuBar->SetLabel(ID_COLORS, _("&Colors"));
    menuBar->SetLabel(ID_SELECTFONTCOLOR, _("Select font &color...\tAlt-C"));
    menuBar->SetLabel(ID_SELECTBACKCOLOR, _("Select &background color...\tAlt-B"));
    menuBar->SetLabel(ID_SELECTLINKCOLOR, _("Select l&inks color...\tAlt-I"));
    menuBar->SetLabel(ID_VOLUME, _("Sound &volume"));
    menuBar->SetLabel(ID_VOLUME0, _("No sound\tAlt-1"));
    menuBar->SetLabel(ID_VOLUME20, _("20%\tAlt-2"));
    menuBar->SetLabel(ID_VOLUME40, _("40%\tAlt-3"));
    menuBar->SetLabel(ID_VOLUME60, _("60%\tAlt-4"));
    menuBar->SetLabel(ID_VOLUME80, _("80%\tAlt-5"));
    menuBar->SetLabel(ID_VOLUME100, _("Initial volume\tAlt-6"));
    menuBar->SetLabel(ID_CHECKUPDATESONSTARTUP, _("Check for updates on startup"));
    menuBar->SetLabel(ID_TOGGLEWINMODE, _("Window / Fullscreen &mode\tAlt-Enter"));
    menuBar->SetLabel(ID_SELECTLANG, _("Select &language...\tAlt-L"));
    menuBar->SetLabel(ID_CHECKUPDATES, _("Check for latest version"));
    menuBar->SetLabel(wxID_ABOUT, _("&About...\tCtrl-H"));

    m_manager->GetPane(wxT("imgview")).Caption(_("Preview"));
    m_manager->GetPane(wxT("objs")).Caption(_("Objects"));
    m_manager->GetPane(wxT("acts")).Caption(_("Actions"));
    m_manager->GetPane(wxT("vars")).Caption(_("Additional desc"));
    m_manager->GetPane(wxT("input")).Caption(_("Input area"));

    m_manager->Update();
}

void QSPFrame::RefreshUI()
{
    m_desc->RefreshUI();
    m_objects->RefreshUI();
    m_actions->RefreshUI();
    m_vars->RefreshUI();
    m_input->Refresh();
    m_imgView->RefreshUI();
}

void QSPFrame::ApplyFont(const wxFont &font)
{
    m_desc->SetTextFont(font);
    m_objects->SetTextFont(font);
    m_actions->SetTextFont(font);
    m_vars->SetTextFont(font);
    m_input->SetFont(font);
}

bool QSPFrame::ApplyFontSize(int size)
{
    wxFont font(m_desc->GetTextFont());
    font.SetPointSize(size);
    ApplyFont(font);
    return true;
}

bool QSPFrame::ApplyFontName(const wxString& name)
{
    if (wxFontEnumerator::IsValidFacename(name))
    {
        wxFont font(m_desc->GetTextFont());
        font.SetFaceName(name);
        ApplyFont(font);
        return true;
    }
    return false;
}

bool QSPFrame::ApplyFontColor(const wxColour& color)
{
    m_desc->SetForegroundColour(color);
    m_objects->SetForegroundColour(color);
    m_actions->SetForegroundColour(color);
    m_vars->SetForegroundColour(color);
    m_input->SetForegroundColour(color);
    return true;
}

bool QSPFrame::ApplyBackColor(const wxColour& color)
{
    m_desc->SetBackgroundColour(color);
    m_objects->SetBackgroundColour(color);
    m_actions->SetBackgroundColour(color);
    m_vars->SetBackgroundColour(color);
    m_input->SetBackgroundColour(color);
    m_imgView->SetBackgroundColour(color);
    return true;
}

bool QSPFrame::ApplyLinkColor(const wxColour& color)
{
    m_desc->SetLinkColor(color);
    m_objects->SetLinkColor(color);
    m_actions->SetLinkColor(color);
    m_vars->SetLinkColor(color);
    return true;
}

void QSPFrame::CallPaneFunc(wxWindowID id, QSP_BOOL toShow) const
{
    switch (id)
    {
    case ID_VARSDESC:
        QSPShowWindow(QSP_WIN_VARS, toShow);
        break;
    case ID_ACTIONS:
        QSPShowWindow(QSP_WIN_ACTS, toShow);
        break;
    case ID_OBJECTS:
        QSPShowWindow(QSP_WIN_OBJS, toShow);
        break;
    case ID_INPUT:
        QSPShowWindow(QSP_WIN_INPUT, toShow);
        break;
    case ID_VIEWPIC:
        // QSPShowWindow(QSP_WIN_VIEW, toShow);
        break;
    }
}

void QSPFrame::SetOverallVolume(int percents)
{
    int id = wxNOT_FOUND;
    switch (percents)
    {
    case 0: id = ID_VOLUME0; break;
    case 20: id = ID_VOLUME20; break;
    case 40: id = ID_VOLUME40; break;
    case 60: id = ID_VOLUME60; break;
    case 80: id = ID_VOLUME80; break;
    case 100: id = ID_VOLUME100; break;
    }
    if (id >= 0) m_settingsMenu->Check(id, true);
    QSPCallbacks::SetOverallVolume((float)percents / 100);
    m_volume = percents;
}

void QSPFrame::TogglePane(const wxWindowID id)
{
    const bool toShow = !m_manager->GetPane(FindWindow(id)).IsShown();
    CallPaneFunc(id, toShow);
    ShowPane(id, toShow);
}

void QSPFrame::OpenGameFile(const wxString& fullPath)
{
    if (wxFileExists(fullPath))
    {
        if (
            const auto file_path = wxStringToQsp(fullPath);
            file_path != nullptr
        )
        {
            if (!QSPLoadGameWorldFromFile(file_path.get(), QSP_FALSE))
            {
                ShowError();
            }
            else
            {
                UpdateGamePath(fullPath);
                m_isGameOpened = true;

                const wxString configString(m_worldPath + QSP_CONFIG);
                if (
                    const wxString newPath(wxFileExists(configString) ? configString : m_configDefPath);
                    newPath != m_configPath
                )
                {
                    SaveSettings();
                    m_configPath = newPath;
                    LoadSettings();
                }

                wxCommandEvent dummy;
                OnNewGame(dummy);

                if (m_toQuit) return;
                UpdateTitle();
                EnableControls(true);
                m_savedGamePath.Clear();
            }
        }
    }
}

void QSPFrame::OpenGameState(const wxString& fullPath)
{
    if (wxFileExists(fullPath))
    {
        if (
            const auto file_path = wxStringToQsp(fullPath);
            file_path != nullptr
        )
        {
            if (!QSPOpenSavedGameFromFile(file_path.get(), QSP_TRUE)) ShowError();
        }
    }
}

void QSPFrame::SaveGameState(const wxString &fullPath)
{
    if (
        const auto file_path = wxStringToQsp(fullPath);
        file_path != nullptr
    )
    {
        QSPSaveGameAsFile(file_path.get(), QSP_TRUE);
    }
    m_savedGamePath = fullPath;
}

void QSPFrame::CheckLatestVersion(int type)
{
    wxWebRequest verRequest = wxWebSession::GetDefault().CreateRequest(this, QSP_LATESTVERAPI, type);

    verRequest.Start();
}

void QSPFrame::ProcessVersionResult(const wxString& versionInfo, int type)
{
    bool isSuccess = false;

    if (!versionInfo.IsEmpty())
    {
        if (
            const wxRegEx versionRegEx(R"lit("name"\s*:\s*"((?:[^"\\]|\\.)*)")lit");
            versionRegEx.Matches(versionInfo)
        )
        {
            isSuccess = true;
            if (
                const wxString latestVersion = versionRegEx.GetMatch(versionInfo, 1);
                latestVersion > QSP_VER
            )
            {
                wxString releaseNotes;
                if (
                    const wxRegEx releaseNotesRegEx(R"lit("body"\s*:\s*"((?:[^"\\]|\\.)*)")lit");
                    releaseNotesRegEx.Matches(versionInfo)
                )
                {
                    releaseNotes = releaseNotesRegEx.GetMatch(versionInfo, 1);
                    releaseNotes.Replace("\\r\\n", "\n");
                    releaseNotes.Replace("\\n", "\n");
                }
                wxString releaseUrl(QSP_LATESTVERPAGE);
                if (
                    const wxRegEx releaseUrlRegEx(R"lit("html_url"\s*:\s*"((?:[^"\\]|\\.)*)")lit");
                    releaseUrlRegEx.Matches(versionInfo)
                )
                    releaseUrl = releaseUrlRegEx.GetMatch(versionInfo, 1);

                UpdateAppDialog dialog(
                    this,
                    _("Update available"),
                    latestVersion,
                    releaseNotes,
                    releaseUrl
                );
                dialog.CenterOnParent();
                if (dialog.ShowModal() == wxID_OK)
                    QSPTools::LaunchDefaultBrowser(releaseUrl);
            }
            else if (type == UPDATE_SHOW_ALL_RESULTS)
            {
                wxMessageDialog dlgMsg(
                    this,
                    _("Your app is already up to date."),
                    _("Info"),
                    wxOK | wxCENTRE | wxICON_INFORMATION
                );
                dlgMsg.ShowModal();
            }
        }
    }

    if (!isSuccess && type == UPDATE_SHOW_ALL_RESULTS)
    {
        wxMessageDialog dlgMsg(
            this,
            _("Can't check the latest version!"),
            _("Error"),
            wxOK | wxCENTRE | wxICON_ERROR
        );
        dlgMsg.ShowModal();
    }
}

void QSPFrame::OnInit(const wxInitEvent& event)
{
    OpenGameFile(event.GetInitString());
}

void QSPFrame::OnClose([[maybe_unused]] wxCloseEvent& event)
{
    SaveSettings();
    EnableControls(false, true);
    Destroy();
    m_toQuit = true;
}

void QSPFrame::OnTimer([[maybe_unused]] wxTimerEvent& event)
{
    if (m_toProcessEvents && !QSPExecCounter(QSP_TRUE))
        ShowError();
}

void QSPFrame::OnMenu(wxCommandEvent& event)
{
    m_menuIndex = event.GetId() - ID_BEGOFDYNMENU;
}

void QSPFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close();
}

void QSPFrame::OnVersionRequestState(const wxWebRequestEvent& event)
{
    switch (event.GetState())
    {
        case wxWebRequest::State_Completed:
            ProcessVersionResult(event.GetResponse().AsString(), event.GetId());
            break;
        case wxWebRequest::State_Failed:
        case wxWebRequest::State_Unauthorized:
            ProcessVersionResult(wxEmptyString, event.GetId());
            break;
    }
}

void QSPFrame::OnOpenGame([[maybe_unused]] wxCommandEvent& event)
{
    wxFileDialog dialog(
        this,
        _("Select game file"),
        wxEmptyString,
        wxEmptyString,
        _("QSP games (*.qsp;*.gam)|*.qsp;*.gam"),
        wxFD_OPEN
    );
    if (dialog.ShowModal() == wxID_OK)
        OpenGameFile(dialog.GetPath());
}

void QSPFrame::OnNewGame([[maybe_unused]] wxCommandEvent& event)
{
    if (!QSPRestartGame(QSP_TRUE))
        ShowError();
}

void QSPFrame::OnOpenGameStat([[maybe_unused]] wxCommandEvent& event)
{
    wxFileDialog dialog(
        this,
        _("Select saved game file"),
        wxEmptyString,
        wxEmptyString,
        _("Saved game files (*.sav)|*.sav"),
        wxFD_OPEN
    );
    if (dialog.ShowModal() == wxID_OK)
        OpenGameState(dialog.GetPath());
}

void QSPFrame::OnSaveGameStat([[maybe_unused]] wxCommandEvent& event)
{
    wxFileDialog dialog(
        this,
        _("Select file to save"),
        wxEmptyString,
        "game.sav",
        _("Saved game files (*.sav)|*.sav"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT
    );
    if (dialog.ShowModal() == wxID_OK)
        SaveGameState(dialog.GetPath());
}

void QSPFrame::OnQuickSave(wxCommandEvent& event)
{
    if (m_savedGamePath.IsEmpty())
        OnSaveGameStat(event);
    else
        SaveGameState(m_savedGamePath);
}

void QSPFrame::OnSelectFont(wxCommandEvent& WXUNUSED(event))
{
    wxFontData data;
    wxFont font(m_desc->GetTextFont());
    font.SetPointSize(m_fontSize);
    font.SetFaceName(m_fontName);
    data.EnableEffects(false);
    data.SetAllowSymbols(false);
    data.SetInitialFont(font);
    wxFontDialog dialog(this, data);
    dialog.SetTitle(_("Select font"));
    if (dialog.ShowModal() == wxID_OK)
    {
        font = dialog.GetFontData().GetChosenFont();
        m_fontSize = font.GetPointSize();
        m_fontName = font.GetFaceName();
        if (m_toProcessEvents)
            ApplyParams();
        else
        {
            ApplyFontSize(m_fontSize);
            ApplyFontName(m_fontName);
            RefreshUI();
        }
    }
}

void QSPFrame::OnUseFontSize([[maybe_unused]] wxCommandEvent& event)
{
    m_toUseFontSize = !m_toUseFontSize;
    if (m_toProcessEvents)
        ApplyParams();
    else
    {
        ApplyFontSize(m_fontSize);
        RefreshUI();
    }
}

void QSPFrame::OnSelectFontColor([[maybe_unused]] wxCommandEvent& event)
{
    wxColourData data;
    data.SetColour(m_fontColor);
    wxColourDialog dialog(this, &data);
    dialog.SetTitle(_("Select font color"));
    if (dialog.ShowModal() == wxID_OK)
    {
        m_fontColor = dialog.GetColourData().GetColour();
        if (m_toProcessEvents)
            ApplyParams();
        else
        {
            ApplyFontColor(m_fontColor);
            RefreshUI();
        }
    }
}

void QSPFrame::OnSelectBackColor([[maybe_unused]] wxCommandEvent& event)
{
    wxColourData data;
    data.SetColour(m_backColor);
    wxColourDialog dialog(this, &data);
    dialog.SetTitle(_("Select background color"));
    if (dialog.ShowModal() == wxID_OK)
    {
        m_backColor = dialog.GetColourData().GetColour();
        if (m_toProcessEvents)
            ApplyParams();
        else
        {
            ApplyBackColor(m_backColor);
            RefreshUI();
        }
    }
}

void QSPFrame::OnSelectLinkColor([[maybe_unused]] wxCommandEvent& event)
{
    wxColourData data;
    data.SetColour(m_linkColor);
    wxColourDialog dialog(this, &data);
    dialog.SetTitle(_("Select links color"));
    if (dialog.ShowModal() == wxID_OK)
    {
        m_linkColor = dialog.GetColourData().GetColour();
        if (m_toProcessEvents)
            ApplyParams();
        else
        {
            ApplyLinkColor(m_linkColor);
            RefreshUI();
        }
    }
}

void QSPFrame::OnCheckUpdatesOnStartup([[maybe_unused]] wxCommandEvent& event)
{
    m_toCheckUpdates = !m_toCheckUpdates;
}

void QSPFrame::OnSelectLang([[maybe_unused]] wxCommandEvent& event)
{
    if (m_transHelper->AskUserForLanguage()) ReCreateGUI();
}

void QSPFrame::OnVolume(const wxCommandEvent& event)
{
    int volume = 100;
    switch (event.GetId())
    {
    case ID_VOLUME0: volume = 0; break;
    case ID_VOLUME20: volume = 20; break;
    case ID_VOLUME40: volume = 40; break;
    case ID_VOLUME60: volume = 60; break;
    case ID_VOLUME80: volume = 80; break;
    }
    SetOverallVolume(volume);
}

void QSPFrame::OnToggleWinMode([[maybe_unused]] wxCommandEvent& event)
{
    ShowFullScreen(!IsFullScreen());
}

void QSPFrame::OnToggleObjs([[maybe_unused]] wxCommandEvent& event)
{
    TogglePane(ID_OBJECTS);
}

void QSPFrame::OnToggleActs([[maybe_unused]] wxCommandEvent& event)
{
    TogglePane(ID_ACTIONS);
}

void QSPFrame::OnToggleDesc([[maybe_unused]] wxCommandEvent& event)
{
    TogglePane(ID_VARSDESC);
}

void QSPFrame::OnToggleInput([[maybe_unused]] wxCommandEvent& event)
{
    TogglePane(ID_INPUT);
}

void QSPFrame::OnToggleCaptions([[maybe_unused]] wxCommandEvent& event)
{
    const bool toShow = !m_manager->GetPane(m_objects).HasCaption();
    const wxAuiPaneInfoArray& allPanes = m_manager->GetAllPanes();
    for (int i = static_cast<int>(allPanes.GetCount()) - 1; i >= 0; --i)
        allPanes.Item(i).CaptionVisible(toShow);
    m_manager->GetPane(m_desc).CaptionVisible(false);
    m_manager->Update();
}

void QSPFrame::OnToggleHotkeys([[maybe_unused]] wxCommandEvent& event)
{
    m_toShowHotkeys = !m_toShowHotkeys;
    if (m_toProcessEvents) QSPCallbacks::RefreshInt(QSP_FALSE, QSP_FALSE);
}

void QSPFrame::OnCheckUpdates([[maybe_unused]] wxCommandEvent& event)
{
    CheckLatestVersion(UPDATE_SHOW_ALL_RESULTS);
}

void QSPFrame::OnAbout([[maybe_unused]] wxCommandEvent& event)
{
    wxAboutDialogInfo info;
    info.SetIcon(wxIcon(logo_big_xpm));
    info.SetName(QSP_LOGO);
    info.SetCopyright(wxT("QSP Foundation, 2001-2026"));

    const auto *version = const_cast<QSP_CHAR *>(QSPGetVersion());
    const auto *libCompiledDate = const_cast<QSP_CHAR *>(QSPGetCompiledDateTime());
    const wxString guiCompiledDate(wxT(__DATE__) wxT(", ") wxT(__TIME__));

    info.SetDescription(wxString::Format(
        _("Engine version: %s\nEngine compiled: %s\nGUI compiled: %s"),
        qspToWxString(version).wx_str(),
        qspToWxString(libCompiledDate).wx_str(),
        guiCompiledDate.wx_str()
    ));
    info.SetWebSite("https://qsp.org");

    wxAboutBox(info, this);
}

void QSPFrame::OnLinkClicked(wxHtmlLinkEvent& event)
{
    if (
        const wxHtmlLinkInfo info(event.GetLinkInfo());
        info.GetEvent()->LeftUp()
    )
    {
        if (
            const wxString href = info.GetHref();
            href.StartsWith("#")
        )
        {
            if (event.GetId() == m_desc->GetId())
                m_desc->LoadPage(href);
            else
                m_vars->LoadPage(href);
        }
        else if (href.Upper().StartsWith("EXEC:"))
        {
            const wxString string = href.Mid(5);
            if (
                const auto exec_code = wxStringToQsp(string);
                exec_code != nullptr
            )
            {
                if (m_toProcessEvents && !QSPExecString(exec_code.get(), QSP_TRUE)) ShowError();
            }
        }
        else
        {
            QSPTools::LaunchDefaultBrowser(href);
        }
    }
    else
    {
        event.Skip();
    }
}

void QSPFrame::OnObjectChange(const wxCommandEvent& event)
{
    m_objects->Update();
    wxThread::Sleep(10);

    if (!QSPSetSelObjectIndex(event.GetInt(), QSP_TRUE))
        ShowError();
}

void QSPFrame::OnActionChange(const wxCommandEvent& event)
{
    m_actions->Update();
    wxThread::Sleep(10);

    if (!QSPSetSelActionIndex(event.GetInt(), QSP_TRUE))
        ShowError();
}

void QSPFrame::OnActionDblClick([[maybe_unused]] wxCommandEvent& event)
{
    if (!QSPExecuteSelActionCode(QSP_TRUE))
        ShowError();
}

void QSPFrame::OnInputTextChange(const wxCommandEvent& event)
{
    const wxString text(event.GetString());
    m_input->SetText(text, false);
    if (
        const auto input_str = wxStringToQsp(text);
        input_str != nullptr
    )
    {
        QSPSetInputStrText(input_str.get());
    }
}

void QSPFrame::OnInputTextEnter([[maybe_unused]] wxCommandEvent& event)
{
    if (!QSPExecUserInput(QSP_TRUE))
        ShowError();
}

void QSPFrame::OnKey(wxKeyEvent& event)
{
    event.Skip();

    if (IsFullScreen() && event.GetKeyCode() == WXK_ESCAPE)
    {
        ShowFullScreen(false);
        return;
    }

    if (event.GetKeyCode() == WXK_SPACE)
        m_keyPressedWhileDisabled = true;

    if (m_toProcessEvents && !event.HasModifiers() && FindFocus() != m_input)
    {
        int ind = -1;
        const int actsCount = QSPGetActions(nullptr, 0);
        switch (event.GetKeyCode())
        {
        case '1': case WXK_NUMPAD1: case WXK_NUMPAD_END: ind = 0; break;
        case '2': case WXK_NUMPAD2: case WXK_NUMPAD_DOWN: ind = 1; break;
        case '3': case WXK_NUMPAD3: case WXK_NUMPAD_PAGEDOWN: ind = 2; break;
        case '4': case WXK_NUMPAD4: case WXK_NUMPAD_LEFT: ind = 3; break;
        case '5': case WXK_NUMPAD5: case WXK_CLEAR: ind = 4; break;
        case '6': case WXK_NUMPAD6: case WXK_NUMPAD_RIGHT: ind = 5; break;
        case '7': case WXK_NUMPAD7: case WXK_NUMPAD_HOME: ind = 6; break;
        case '8': case WXK_NUMPAD8: case WXK_NUMPAD_UP: ind = 7; break;
        case '9': case WXK_NUMPAD9: case WXK_NUMPAD_PAGEUP: ind = 8; break;
        case WXK_SPACE:
            if (actsCount == 1) ind = 0;
            break;
        }
        if (ind >= 0 && ind < actsCount)
        {
            wxCommandEvent e;
            if (QSPSetSelActionIndex(ind, QSP_TRUE))
                OnActionDblClick(e);
            else
                ShowError();
        }
    }
}

void QSPFrame::OnWheel(const wxMouseEvent& event)
{
    if (wxWindow *win = wxFindWindowAtPoint(wxGetMousePosition()))
        win->ScrollLines(-event.GetWheelRotation() / event.GetWheelDelta() * event.GetLinesPerAction());
}

void QSPFrame::OnMouseClick(wxMouseEvent& event)
{
    event.Skip();
    m_keyPressedWhileDisabled = true;
}

void QSPFrame::OnPaneClose(wxAuiManagerEvent& event)
{
    if (m_toProcessEvents)
        CallPaneFunc(event.GetPane()->window->GetId(), QSP_FALSE);
    else
        event.Veto();
}

void QSPFrame::OnDropFiles(const wxDropFilesEvent& event)
{
    if (event.GetNumberOfFiles() && (!m_isGameOpened || m_toProcessEvents))
    {
        wxFileName path(*event.GetFiles());
        path.MakeAbsolute();
        OpenGameFile(path.GetFullPath());
    }
}
