// Stub do VGUI para o build Android (Xash3D FWGS).
// Substitui vgui_TeamFortressViewport.h, vgui_int.h, vgui_scorepanel.h,
// vgui_SpectatorPanel.h e vgui_schememanager.h quando NO_VGUI esta definido.
// Mantem as chamadas existentes compilando, mas sem desenhar nenhum painel VGUI.
#ifndef AZ_ANDROID_VGUI_STUB_H
#define AZ_ANDROID_VGUI_STUB_H

#ifndef MAX_SERVERNAME_LENGTH
#define MAX_SERVERNAME_LENGTH 32
#endif

#include "tf_defs.h" // MENU_SPECHELP etc.

// Cores de time (antes em vgui_TeamFortressViewport.cpp)
static int iNumberOfTeamColors = 5;
static int iTeamColors[5][3] =
{
	{ 255, 170, 0 },	// HL orange (default)
	{ 125, 165, 210 },	// Blue
	{ 200, 90, 70 },	// Red
	{ 225, 205, 45 },	// Yellow
	{ 145, 215, 140 },	// Green
};

namespace vgui
{
	class Panel
	{
	public:
		virtual ~Panel() {}
		bool isVisible() { return false; }
		void setParent(Panel*) {}
		void removeChild(Panel*) {}
	};
}

class CCommandMenu;
class CMenuPanel;

class CSchemeManager
{
public:
	CSchemeManager() {}
};

class ScorePanel : public vgui::Panel
{
public:
	bool isVisible() { return false; }
};

class SpectatorBanner
{
public:
	void LoadImage(const char*) {}
};

class SpectatorPanel : public vgui::Panel
{
public:
	SpectatorPanel() : m_menuVisible(false), m_TopBanner(&m_bannerStub) {}
	void ShowMenu(bool) {}
	void Update() {}
	void EnableInsetView(bool) {}
	bool isVisible() { return false; }

	bool m_menuVisible;
	SpectatorBanner* m_TopBanner;
private:
	SpectatorBanner m_bannerStub;
};

class TeamFortressViewport : public vgui::Panel
{
public:
	TeamFortressViewport()
		: m_SpectatorOptionsMenu(0), m_SpectatorCameraMenu(0),
		  m_pScoreBoard(&m_scoreStub), m_pSpectatorPanel(&m_specStub)
	{
		m_szServerName[0] = '\0';
	}

	void Initialize(void) {}
	void UpdateCursorState(void) {}
	void UpdateSpectatorPanel(void) {}
	int  KeyInput(int /*down*/, int /*keynum*/, const char* /*pszCurrentBinding*/) { return 1; } // 1 = deixa o engine tratar a tecla
	void GetAllPlayersInfo(void) {}
	void DeathMsg(int /*killer*/, int /*victim*/) {}

	void ShowCommandMenu(int) {}
	void HideCommandMenu(void) {}
	void SetCurrentCommandMenu(CCommandMenu*) {}
	void SetCurrentMenu(CMenuPanel*) {}

	void ShowScoreBoard(void) {}
	void HideScoreBoard(void) {}
	bool IsScoreBoardVisible(void) { return false; }

	bool AllowedToPrintText(void) { return true; }

	void ShowVGUIMenu(int) {}
	void HideVGUIMenu(void) {}
	void HideTopMenu(void) {}
	void ToggleServerBrowser(void) {}

	int   GetNumberOfTeams() { return 0; }
	char* GetTeamName(int) { return m_szServerName; } // string vazia
	int   GetAllowSpectators() { return 0; }
	bool  SlotInput(int) { return false; }

	CSchemeManager* GetSchemeManager(void) { return &m_SchemeManager; }
	ScorePanel* GetScoreBoard(void) { return m_pScoreBoard; }

	int MsgFunc_TeamNames(const char*, int, void*) { return 1; }
	int MsgFunc_VGUIMenu(const char*, int, void*) { return 1; }
	int MsgFunc_MOTD(const char*, int, void*) { return 1; }
	int MsgFunc_ServerName(const char*, int, void*) { return 1; }
	int MsgFunc_ScoreInfo(const char*, int, void*) { return 1; }
	int MsgFunc_TeamScore(const char*, int, void*) { return 1; }
	int MsgFunc_TeamInfo(const char*, int, void*) { return 1; }
	int MsgFunc_Spectator(const char*, int, void*) { return 1; }
	int MsgFunc_AllowSpec(const char*, int, void*) { return 1; }

	int m_SpectatorOptionsMenu;
	int m_SpectatorCameraMenu;
	ScorePanel* m_pScoreBoard;
	SpectatorPanel* m_pSpectatorPanel;
	char m_szServerName[MAX_SERVERNAME_LENGTH];

private:
	CSchemeManager m_SchemeManager;
	ScorePanel m_scoreStub;
	SpectatorPanel m_specStub;
};

extern TeamFortressViewport* gViewPort;

extern "C"
{
	inline void VGui_Startup() { if (!gViewPort) gViewPort = new TeamFortressViewport(); }
	inline void VGui_Shutdown() { /* gViewPort e estatico no Android: nada a liberar */ }
	inline void VGui_ViewportPaintBackground(int /*extents*/[4]) {}
}

#endif
