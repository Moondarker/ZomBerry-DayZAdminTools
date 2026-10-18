//Menu shell: title, tab switching, shared header button and RPC routing. All page logic lives in 5_Mission/ui components
class ZomberryMenu extends UIScriptedMenu {
	protected TextWidget m_TxtTitle;
	protected ButtonWidget m_ActionButton;

	protected ref ZBerryUIPlayersPanel m_PlayersPanel;
	protected ref ZBerryUIFunctionsTab m_FunctionsTab;
	protected ref ZBerryUISpawnTab m_SpawnTab;
	protected ref ZBerryUIMapTab m_MapTab;
	protected ref ZBerryUIPermsTab m_PermsTab;

	protected ref array<ZBerryUITab> m_Tabs = new array<ZBerryUITab>;
	protected ZBerryUITab m_ActiveTab;

	override Widget Init() {
		if (layoutRoot) return layoutRoot;

		layoutRoot = GetGame().GetWorkspace().CreateWidgets( "ZomBerry/AT/scripts/5_Mission/layouts/AdminDialog.layout" );
		if (!layoutRoot) {
			ZomberryBase.Log( "ZomBerryUI", "ERROR: Can't load AdminDialog.layout" );
			return NULL;
		}

		m_TxtTitle = TextWidget.Cast( layoutRoot.FindAnyWidget("TitleText") );
		m_ActionButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("BindButton") );

		Widget contentWrapper = layoutRoot.FindAnyWidget("ContentWrapper");
		Widget sectionWrapper = layoutRoot.FindAnyWidget("SectionWrapper");

		m_PlayersPanel = new ZBerryUIPlayersPanel();
		m_PlayersPanel.Init(contentWrapper);

		m_FunctionsTab = new ZBerryUIFunctionsTab();
		m_FunctionsTab.SetPlayersPanel(m_PlayersPanel);
		AddTab(m_FunctionsTab, sectionWrapper, "ZomBerry/AT/scripts/5_Mission/layouts/FunctionsTab.layout", "FunctionsButton");

		m_SpawnTab = new ZBerryUISpawnTab();
		AddTab(m_SpawnTab, sectionWrapper, "ZomBerry/AT/scripts/5_Mission/layouts/SpawnTab.layout", "SpawnButton");

		m_MapTab = new ZBerryUIMapTab();
		AddTab(m_MapTab, sectionWrapper, "ZomBerry/AT/scripts/5_Mission/layouts/MapTab.layout", "MapButton");

		m_PermsTab = new ZBerryUIPermsTab();
		AddTab(m_PermsTab, sectionWrapper, "ZomBerry/AT/scripts/5_Mission/layouts/PermsTab.layout", "PermsButton");
		m_PermsTab.SetPlayersPanel(m_PlayersPanel);

		SelectTab(m_FunctionsTab);
		ApplyPermissions();

		return layoutRoot;
	}

	protected void AddTab(ZBerryUITab tab, Widget parent, string layoutPath, string tabButtonName) {
		tab.InitTab(parent, layoutPath, ButtonWidget.Cast( layoutRoot.FindAnyWidget(tabButtonName) ), m_ActionButton);
		m_Tabs.Insert(tab);
	}

	//Tabs the admin may not use are hidden, the server refuses their requests anyway
	protected void ApplyPermissions() {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		ShowTabButton(m_SpawnTab, auth.HasAny({ZBerryPermissions.SPAWN_GROUND, ZBerryPermissions.SPAWN_INVENTORY}));
		ShowTabButton(m_MapTab, auth.Has(ZBerryPermissions.MAP_VIEW));
		ShowTabButton(m_PermsTab, auth.CanManagePermissions());

		foreach (ZBerryUITab tab : m_Tabs) {
			tab.OnPermissionsChanged();
		}
	}

	protected void ShowTabButton(ZBerryUITab tab, bool show) {
		if (tab.GetTabButton()) tab.GetTabButton().Show(show);
		if (!show && tab == m_ActiveTab) SelectTab(m_FunctionsTab);
	}

	//The server sent new rights: refresh what depends on them
	void OnPermissionsChanged() {
		if (!layoutRoot) return;

		ApplyPermissions();
		if (layoutRoot.IsVisible()) {
			ZBerryUIPlayersPanel.RequestSync();
			m_FunctionsTab.OnMenuShow(); //Asks for the (filtered) function list again
		}
	}

	void OnPermsData(ZBerryPermsSnapshot snapshot) {
		if (m_PermsTab) m_PermsTab.SetSnapshot(snapshot);
	}

	protected void SelectTab(ZBerryUITab tab) {
		foreach (ZBerryUITab otherTab : m_Tabs) {
			if (otherTab != tab) otherTab.Deactivate();
		}

		m_ActiveTab = tab;
		m_ActiveTab.Activate();
	}

	//Input focus is taken and released by UIScriptedMenu itself (LockControls/UnlockControls in super.OnShow/OnHide).
	//Taking extra game focus here froze the input API while the menu was open: the menu key stayed "held"
	//from the frame the menu opened and UAUIBack never fired, so the menu couldn't be closed
	override void OnShow() {
		super.OnShow();

		m_TxtTitle.SetText( "Players in game: ... | ZomBerry Admin Tools " + ZBerryVersion.GetDisplayVersion() + " by Vaker" );
		ApplyPermissions();
		m_PlayersPanel.UpdateListScale();
		ZBerryUIPlayersPanel.RequestSync();

		foreach (ZBerryUITab tab : m_Tabs) {
			tab.OnMenuShow();
		}
	}

	override void OnHide() {
		super.OnHide();

		foreach (ZBerryUITab tab : m_Tabs) {
			tab.OnMenuHide();
		}
	}

	override void Update(float timeslice) {
		if (m_ActiveTab) m_ActiveTab.Update(timeslice);
	}

	override bool OnClick( Widget w, int x, int y, int button ) {
		if ( w == m_ActionButton ) {
			if (m_ActiveTab) m_ActiveTab.OnActionButtonClick();
			return true;
		}

		foreach (ZBerryUITab tab : m_Tabs) {
			if ( w == tab.GetTabButton() ) {
				SelectTab(tab);
				return true;
			}
		}

		return false;
	}

	//Raw key presses forwarded from MissionGameplay while the menu is open (used for keybinding)
	void OnBindKeyPress( int key ) {

		foreach (ZBerryUITab tab : m_Tabs) {
			tab.OnBindKeyPress(key);
		}
	}

	bool GetCloseClearance() {
		foreach (ZBerryUITab tab : m_Tabs) {
			if (tab.BlocksClose()) return false;
		}
		return true;
	}

	void SyncPlayers( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2<ref ZBerryPlayerArray, int> playerListS;

		if ( !(type == ZBerryCallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer()) ) return;
		if ( !ctx.Read( playerListS ) ) {
			ZBerryUIUtils.Message("Player sync data read error - possible version mismatch");
			return;
		}

		ZBerryPlayerArray players = playerListS.param1;
		int sUpTime = Math.Round(playerListS.param2 / 1000);

		m_TxtTitle.SetText( "Players in game: " + players.Count() + " | " + Math.Floor(sUpTime / 3600).ToString() + "h " + ((sUpTime / 60) % 60).ToString() + "min | ZomBerry Admin Tools " + ZBerryVersion.GetDisplayVersion() + " by Vaker");
		m_PlayersPanel.SetPlayers(players);
		m_MapTab.SetPlayers(players);
	}

	void SyncFunctions( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<ref ZBerryCategoryArray> catListS;

		if ( !(type == ZBerryCallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer()) ) return;
		if ( !ctx.Read( catListS ) ) return;

		m_FunctionsTab.SetFunctions(catListS.param1);
	}
}
