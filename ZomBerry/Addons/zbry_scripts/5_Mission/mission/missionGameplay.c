modded class MissionGameplay {
	ref ZomberryClient m_ZomberryClient;
	ref ZomberryMenu m_ZomberryMenu;

	void MissionGameplay() {
		m_ZomberryClient = new ref ZomberryClient;

		ZomberryClient.Log( "ZomBerry", "Starting Client side v" + g_zbryVer );
	}

	void ~MissionGameplay() {

		if (m_ZomberryClient) delete m_ZomberryClient;
		if (m_ZomberryMenu) delete m_ZomberryMenu;
	}

	private ref ZomberryMenu GetZomberryMenu() {
		if ( !m_ZomberryMenu ) {
			m_ZomberryMenu = new ref ZomberryMenu;
			m_ZomberryMenu.Init();
		}

		return m_ZomberryMenu;
	}

	override void OnMissionStart() {
		super.OnMissionStart();

		m_ZomberryClient.OnClientReady();
	}

	override void OnKeyPress(int key) {
		super.OnKeyPress(key);

		if (GetZomberryMenu().GetLayoutRoot().IsVisible()) {
			GetZomberryMenu().OnKeyPress(key);
		} else if (m_ZomberryClient.IsAdmin()) {
			//ZomberryClient.GetKeyBindsMgr().OnKeyPress(key);
		}
	}

	void DisplayMessage(string msg) {
		GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "", msg, ""));
	}

	override void OnUpdate(float timeslice) {
		super.OnUpdate(timeslice);

		if (GetUApi().GetInputByName("UAZBerryOpenAdminMenu").LocalPress()) {
			string r_zbryVer = m_ZomberryClient.GetRemoteVersion();
			UIScriptedMenu menu = GetUIManager().GetMenu();

			if (!menu && !GetZomberryMenu().GetLayoutRoot().IsVisible() && m_ZomberryClient.IsAdmin()) {
				GetUIManager().ShowScriptedMenu( GetZomberryMenu(), NULL );
				PlayerControlDisable(INPUT_EXCLUDE_ALL);
			} else if (GetZomberryMenu().GetLayoutRoot().IsVisible() && GetZomberryMenu().GetCloseClearance()) {
				GetUIManager().HideScriptedMenu( GetZomberryMenu() );
				PlayerControlEnable(false);
			}
		}

		if (GetUApi().GetInputByName("UAUIBack").LocalPress() && GetZomberryMenu().GetLayoutRoot().IsVisible() && GetZomberryMenu().GetCloseClearance()) {
			GetUIManager().HideScriptedMenu( GetZomberryMenu() );
			PlayerControlEnable(false);
		}
	}
};
