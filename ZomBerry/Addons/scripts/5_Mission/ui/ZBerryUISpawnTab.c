//Object spawner: category filters, search, spawn target and Safe/Expert mode. The Loadouts button switches to the
//admin's own and the shared loadouts instead (GetZBerryClientLoadouts), and the gear online players wear now, which
//can be saved, tried on or given. Give and Undo act on the players selected in the players panel
class ZBerryUISpawnTab extends ZBerryUITab {
	static const int COLOR_CATEGORY = 0xFFBCCDE8;
	static const int COLOR_CATEGORY_HIDDEN = 0xFF6767E0;
	static const int COLOR_PARTLY = 0xFFE8A060; //A loadout with items this admin's spawn rules leave out
	static const int COLOR_SAVED = 0xFF47EB00;
	static const int COLOR_PLAYER = 0xFFA0E0A0; //Online players' current gear
	static const float DROPUP_LINGER = 0.25; //Seconds the target drop-up stays open after the mouse left it and the button
	static const float DELETE_CONFIRM = 3; //Seconds a first click on Delete waits for the second

	protected TextListboxWidget m_ObjectsList; //The visible variant of m_ObjectsLists
	protected ref ZBerryScaledList m_ObjectsLists;
	protected EditBoxWidget m_SearchInput;
	protected ButtonWidget m_TargetButton;
	protected Widget m_TargetDropup;
	protected ref array<ButtonWidget> m_TargetButtons = new array<ButtonWidget>; //Drop-up rows, one per m_Targets entry
	protected ref TStringArray m_Targets = new TStringArray;
	protected ref array<ButtonWidget> m_FilterButtons = new array<ButtonWidget>;

	protected ref ZBerryJsonSpawnMenuGroupArray m_SpawnMenuGroups;
	protected ref map<string, bool> m_CategoryHidden = new map<string, bool>;

	protected string m_ObjFilter = "All";
	protected string m_ObjTarget = "Cursor";
	protected ZBerryUIPlayersPanel m_PlayersPanel; //For the selected player's spawn targets
	protected bool m_ExpertMode = false;
	protected bool m_MouseOnSearch = false;
	protected bool m_MouseOnTargets = false; //On the target button or its drop-up
	protected float m_DropupTimer = 0; //Seconds since the mouse left them
	protected bool m_ListDirty = true;

	protected ButtonWidget m_LoadoutsButton;
	protected Widget m_ContentWrapper, m_LoadoutsWrapper;
	protected TextListboxWidget m_LoadoutsList, m_LoadoutItemsList; //Visible variants
	protected ref ZBerryScaledList m_LoadoutsLists, m_LoadoutItemsLists;
	protected EditBoxWidget m_LoadoutName;
	protected ButtonWidget m_LoadoutSave, m_LoadoutShare, m_LoadoutDelete, m_LoadoutApply, m_LoadoutUndo, m_LoadoutTarget;
	protected bool m_ApplyToPlayers; //Apply and Undo act on the players selected on the left, not on the admin
	protected ZBerryUIConfirmDialog m_ConfirmDialog; //The menu's
	protected bool m_LoadoutMode;
	protected bool m_MouseOnName;
	protected bool m_SelectedShared; //The selected loadout: shared or the admin's own, and its name ("" for none)
	protected string m_SelectedName = "";
	protected int m_SelectedPlayer = -1; //Session id when a player's current gear is selected (m_SelectedName is theirs)
	protected string m_RosterKey = ""; //The players listed, to rebuild the list only when they change
	protected ref TIntArray m_PendingTargets = new TIntArray; //Waiting for the confirmation
	protected bool m_PendingUndo;
	protected float m_DeleteTimer = -1; //Running after a first click on Delete

	protected override void OnInit() {
		m_ObjectsLists = new ZBerryScaledList(m_Root, "ObjectsList", this);
		m_ObjectsLists.Pick();
		m_ObjectsList = m_ObjectsLists.GetActive();
		m_SearchInput = EditBoxWidget.Cast( BindWidget("SearchInput") );
		m_TargetButton = ButtonWidget.Cast( BindWidget("TargetButton") );
		m_TargetDropup = BindWidget("TargetDropup"); //Bound too: the gaps between its rows keep it open

		TStringArray targets = {"Cursor", "Ground", "Inventory", "Player's feet", "Player's inv."};
		TStringArray rowNames = {"TargetCursor", "TargetGround", "TargetInventory", "TargetPlayerFeet", "TargetPlayerInv"};
		for (int i = 0; i < rowNames.Count(); ++i) {
			m_Targets.Insert(targets[i]);
			m_TargetButtons.Insert( ButtonWidget.Cast( BindWidget(rowNames[i]) ) );
		}

		m_FilterButtons.Insert( ButtonWidget.Cast( BindWidget("ItemsButton") ) );
		m_FilterButtons.Insert( ButtonWidget.Cast( BindWidget("ObjectsButton") ) );
		m_FilterButtons.Insert( ButtonWidget.Cast( BindWidget("AIButton") ) );

		m_LoadoutsButton = ButtonWidget.Cast( BindWidget("LoadoutsButton") );
		m_ContentWrapper = m_Root.FindAnyWidget("SContentWrapper");
		m_LoadoutsWrapper = m_Root.FindAnyWidget("LoadoutsWrapper");
		m_LoadoutsLists = new ZBerryScaledList(m_Root, "LoadoutsList", this);
		m_LoadoutItemsLists = new ZBerryScaledList(m_Root, "LoadoutItemsList", this);
		m_LoadoutsLists.Pick();
		m_LoadoutItemsLists.Pick();
		m_LoadoutsList = m_LoadoutsLists.GetActive();
		m_LoadoutItemsList = m_LoadoutItemsLists.GetActive();
		m_LoadoutName = EditBoxWidget.Cast( BindWidget("LoadoutName") );
		m_LoadoutSave = ButtonWidget.Cast( BindWidget("LoadoutSave") );
		m_LoadoutShare = ButtonWidget.Cast( BindWidget("LoadoutShare") );
		m_LoadoutDelete = ButtonWidget.Cast( BindWidget("LoadoutDelete") );
		m_LoadoutApply = ButtonWidget.Cast( BindWidget("LoadoutApply") );
		m_LoadoutUndo = ButtonWidget.Cast( BindWidget("LoadoutUndo") );
		m_LoadoutTarget = ButtonWidget.Cast( BindWidget("LoadoutTarget") );
	}

	void SetConfirmDialog(ZBerryUIConfirmDialog dialog) {
		m_ConfirmDialog = dialog;
	}

	void ~ZBerryUISpawnTab() {
		if (g_ZBerryStreamClient) g_ZBerryStreamClient.OnPlayersChanged.Remove(OnRoster);
		if (!g_ZBerryClientLoadouts) return;
		g_ZBerryClientLoadouts.OnChanged.Remove(OnLoadoutsChanged);
		g_ZBerryClientLoadouts.OnDetailsChanged.Remove(RefreshItems);
	}

	protected override void OnActivate() {
		UpdateModeButton();
		if (m_ListDirty) UpdateObjList();
		if (m_LoadoutMode) GetZBerryClientLoadouts().RequestList();
	}

	protected override void OnDeactivate() {
		ShowDropup(false);
		DisarmDelete();
	}

	override void OnMenuHide() {
		ShowDropup(false);
	}

	override void Update(float tDelta) {
		if (m_DeleteTimer >= 0) {
			m_DeleteTimer += tDelta;
			if (m_DeleteTimer >= DELETE_CONFIRM) DisarmDelete();
		}

		if (!m_TargetDropup || !m_TargetDropup.IsVisible() || m_MouseOnTargets) return;

		m_DropupTimer += tDelta;
		if (m_DropupTimer >= DROPUP_LINGER) ShowDropup(false);
	}

	override void OnMenuShow() {
		if (m_ObjectsLists.Pick()) m_ObjectsList = m_ObjectsLists.GetActive(); //Rebuilt below (m_ListDirty)
		m_LoadoutsLists.Pick();
		m_LoadoutItemsLists.Pick();
		m_LoadoutsList = m_LoadoutsLists.GetActive();
		m_LoadoutItemsList = m_LoadoutItemsLists.GetActive();

		//Again on every show: the client side is built anew for each server, the menu isn't
		ZBerryClientLoadouts loadouts = GetZBerryClientLoadouts();
		loadouts.OnChanged.Remove(OnLoadoutsChanged);
		loadouts.OnChanged.Insert(OnLoadoutsChanged);
		loadouts.OnDetailsChanged.Remove(RefreshItems);
		loadouts.OnDetailsChanged.Insert(RefreshItems);
		GetZBerryStreams().OnPlayersChanged.Remove(OnRoster);
		GetZBerryStreams().OnPlayersChanged.Insert(OnRoster);
		UpdateLoadoutMode();
		if (m_LoadoutMode && IsVisible()) loadouts.RequestList();
		m_SpawnMenuGroups = ZomberryBase.GetConfig().GetSpawnMenuData();

		for (int i = 0; i < m_FilterButtons.Count(); ++i) {
			if (i < m_SpawnMenuGroups.Count()) {
				ZBerryUIUtils.SetButtonText(m_FilterButtons[i], m_SpawnMenuGroups[i].CategoryName);
				m_FilterButtons[i].Show(true);
			} else {
				m_FilterButtons[i].Show(false);
			}
		}

		ZBerryUIUtils.SetButtonText(m_TargetButton, m_ObjTarget);

		//Config list is huge, so only rebuild it when the tab is actually visible
		m_ListDirty = true;
		if (IsVisible()) UpdateObjList();
	}

	override bool BlocksClose() {
		return m_MouseOnSearch || m_MouseOnName;
	}

	override void OnActionButtonClick() {
		m_ExpertMode = !m_ExpertMode;
		UpdateModeButton();
		UpdateObjList();
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		ButtonWidget btn = ButtonWidget.Cast(w);
		int filterIdx = -1;
		if (btn) filterIdx = m_FilterButtons.Find(btn);

		if (filterIdx != -1) {
			bool fromLoadouts = m_LoadoutMode;
			m_LoadoutMode = false;
			UpdateLoadoutMode();
			if (!fromLoadouts || m_ObjFilter != m_SpawnMenuGroups[filterIdx].BaseClassNames) ToggleFilter(filterIdx); //Back to the filter shown before: kept on
			return true;
		}

		if (w == m_LoadoutsButton) {
			m_LoadoutMode = !m_LoadoutMode;
			UpdateLoadoutMode();
			if (m_LoadoutMode) GetZBerryClientLoadouts().RequestList();
			return true;
		}
		if (OnLoadoutClick(w)) return true;

		int targetIdx = -1;
		if (btn) targetIdx = m_TargetButtons.Find(btn);
		if (targetIdx != -1) {
			m_ObjTarget = m_Targets[targetIdx];
			ZBerryUIUtils.SetButtonText(m_TargetButton, m_ObjTarget);
			LayoutDropup(); //Stays open to show the new choice; leaving it closes it
			return true;
		}

		if (w == m_TargetButton) { //Clicking still steps through the targets, in the drop-up's order
			m_ObjTarget = NextAllowedTarget(m_ObjTarget);
			ZBerryUIUtils.SetButtonText(m_TargetButton, m_ObjTarget);
			ShowDropup(true);
			return true;
		}
		return false;
	}

	//Hidden widgets don't always get a mouse leave, so closing also forgets the mouse
	protected void ShowDropup(bool show) {
		if (!m_TargetDropup) return;

		if (show) LayoutDropup();
		m_TargetDropup.Show(show);
		m_DropupTimer = 0;
		if (!show) m_MouseOnTargets = false;
	}

	//Only the targets the admin may use, stacked from the top, the drop-up's bottom edge on the button's top edge
	protected void LayoutDropup() {
		float screenWidth = 0;
		float screenHeight = 0;
		m_Root.GetScreenSize(screenWidth, screenHeight);
		if (screenWidth <= 0) return;

		//SetPos/SetSize take physical pixels, the offsets below are layout units: the wrapper is exactly 600 units wide
		float u = screenWidth / 600;
		int rows = 0;
		for (int i = 0; i < m_TargetButtons.Count(); ++i) {
			ButtonWidget row = m_TargetButtons[i];
			if (!row) continue;

			bool allowed = IsTargetAllowed(m_Targets[i]);
			row.Show(allowed);
			if (!allowed) continue;

			row.SetPos(4 * u, (4 + rows * 34) * u);
			ZBerryUIUtils.SetButtonActive(row, m_Targets[i] == m_ObjTarget);
			++rows;
		}

		float height = (4 + rows * 34) * u;
		m_TargetDropup.SetSize(183 * u, height);

		float buttonX = 0;
		float buttonY = 0;
		float parentX = 0;
		float parentY = 0;
		m_TargetButton.GetScreenPos(buttonX, buttonY);
		m_TargetDropup.GetParent().GetScreenPos(parentX, parentY);
		m_TargetDropup.SetPos(buttonX - parentX - 4 * u, buttonY - parentY - height);
	}

	protected bool IsTargetWidget(Widget w) {
		if (!w) return false;
		if (w == m_TargetButton || w == m_TargetDropup) return true;

		ButtonWidget btn = ButtonWidget.Cast(w);
		return btn && m_TargetButtons.Find(btn) != -1;
	}

	void SetPlayersPanel(ZBerryUIPlayersPanel playersPanel) {
		m_PlayersPanel = playersPanel;
	}

	//The first allowed target after current, top to bottom in the drop-up, then from the top again
	protected string NextAllowedTarget(string current) {
		int index = m_Targets.Find(current);

		for (int step = 1; step <= m_Targets.Count(); ++step) {
			string candidate = m_Targets[(index + step) % m_Targets.Count()];
			if (IsTargetAllowed(candidate)) return candidate;
		}

		return current;
	}

	protected bool IsTargetAllowed(string objTarget) {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		if (objTarget == "Player's inv.") return auth.Has(ZBerryPermissions.SPAWN_PLAYER) && auth.Has(ZBerryPermissions.SPAWN_INVENTORY);
		if (objTarget == "Player's feet") return auth.Has(ZBerryPermissions.SPAWN_PLAYER) && auth.Has(ZBerryPermissions.SPAWN_GROUND);
		if (objTarget == "Inventory") return auth.Has(ZBerryPermissions.SPAWN_INVENTORY);

		return auth.Has(ZBerryPermissions.SPAWN_GROUND);
	}

	override void OnPermissionsChanged() {
		UpdateLoadoutMode();
		if (!IsTargetAllowed(m_ObjTarget)) m_ObjTarget = NextAllowedTarget(m_ObjTarget);
		ZBerryUIUtils.SetButtonText(m_TargetButton, m_ObjTarget);
		if (m_TargetDropup && m_TargetDropup.IsVisible()) LayoutDropup();

		m_ListDirty = true;
		if (IsVisible()) UpdateObjList();
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button) {
		if (w == m_LoadoutsList) {
			ApplySelected();
			return true;
		}
		if (w != m_ObjectsList) return false;

		int selectedObjRow = m_ObjectsList.GetSelectedRow();
		if ( selectedObjRow == -1 ) return true;

		string currentObject;
		Param2<string, string> objData; //Row type ("Category"/"Entry"), class name
		m_ObjectsList.GetItemText(selectedObjRow, 0, currentObject);
		m_ObjectsList.GetItemData(selectedObjRow, 0, objData);

		if (objData.param1 == "Category") {
			currentObject.Replace(" == ", "");
			m_CategoryHidden.Set(currentObject, !m_CategoryHidden.Get(currentObject));
			UpdateObjList();
		}

		if (objData.param1 == "Entry") {
			vector targetPlace = ZBerryUIUtils.GetCursorPos();
			int mode = ZBerrySpawner.AT_POSITION;
			int playerId = -1;
			if (m_ObjTarget == "Ground" && GetGame().GetPlayer()) targetPlace = GetGame().GetPlayer().GetPosition(); //In FreeCam: at the cursor
			if (m_ObjTarget == "Inventory") mode = ZBerrySpawner.OWN_INVENTORY;
			if (m_ObjTarget == "Player's feet") mode = ZBerrySpawner.PLAYER_FEET;
			if (m_ObjTarget == "Player's inv.") mode = ZBerrySpawner.PLAYER_INVENTORY;

			if (mode == ZBerrySpawner.PLAYER_FEET || mode == ZBerrySpawner.PLAYER_INVENTORY) {
				if (!m_PlayersPanel || !m_PlayersPanel.GetSelectedPlayerId(playerId)) {
					ZBerryUIUtils.Message("Double-click a player in the list first");
					return true;
				}
			}

			GetZBerryRPC().SendRPC( ZBerryRPCOp.SpawnObject, new Param4< string, vector, int, int >( objData.param2, targetPlace, mode, playerId ), true );
		}
		return true;
	}

	override bool OnChange(Widget w, int x, int y, bool finished) {
		if (w == m_SearchInput) {
			UpdateObjList();
			return true;
		}
		return false;
	}

	//Moving between the target button, the drop-up and its rows keeps the drop-up open
	override bool OnMouseEnter(Widget w, int x, int y) {
		if (w == m_SearchInput) {
			m_MouseOnSearch = true;
			return true;
		}
		if (w == m_LoadoutName) {
			m_MouseOnName = true;
			return true;
		}
		if (IsTargetWidget(w)) {
			if (m_TargetDropup && !m_TargetDropup.IsVisible()) ShowDropup(true);
			m_MouseOnTargets = true;
			m_DropupTimer = 0;
		}
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y) {
		if (w == m_SearchInput) {
			m_MouseOnSearch = false;
			return true;
		}
		if (w == m_LoadoutName) {
			m_MouseOnName = false;
			return true;
		}
		if (IsTargetWidget(w) && !IsTargetWidget(enterW)) {
			m_MouseOnTargets = false;
			m_DropupTimer = 0;
		}
		return false;
	}

	protected void ToggleFilter(int filterIdx) {
		string filter = m_SpawnMenuGroups[filterIdx].BaseClassNames;

		if (m_ObjFilter == filter) {
			m_ObjFilter = "All";
		} else {
			m_ObjFilter = filter;
		}

		for (int i = 0; i < m_FilterButtons.Count(); ++i) {
			ZBerryUIUtils.SetButtonActive(m_FilterButtons[i], i == filterIdx && m_ObjFilter != "All");
		}

		UpdateObjList();
	}

	protected void UpdateModeButton() {
		if (m_ExpertMode) {
			SetActionButton("Expert", COLOR_RED);
		} else {
			SetActionButton("Safe", COLOR_GREEN);
		}
	}

	protected void UpdateObjList() {
		m_ListDirty = false;
		m_ObjectsList.ClearItems();

		TStringArray configs = { CFG_VEHICLESPATH, CFG_WEAPONSPATH, CFG_MAGAZINESPATH };
		ZBerryClientAuth clientAuth = GetZBerryClientAuth();
		int classCount = 0;
		int deniedCount = 0;

		map<string, int> categoryIndex = new map<string, int>;
		TStringArray splitFilters = new TStringArray;
		m_ObjFilter.Split(",", splitFilters);
		string strSearch = m_SearchInput.GetText();

		if (!m_CategoryHidden.Contains(splitFilters[0])) {
			for (int pop = 0; pop < splitFilters.Count(); ++pop) {
				m_CategoryHidden.Insert( splitFilters[pop], false );
			}
		}

		for (int i = 0; i < splitFilters.Count(); ++i) {
			categoryIndex.Insert( splitFilters[i], i );
			m_ObjectsList.AddItem( " == " + splitFilters[i] + " == ", new Param2<string, string>("Category", ""), 0, i );

			if (m_CategoryHidden.Get(splitFilters[i])) {
				m_ObjectsList.SetItemColor(i, 0, COLOR_CATEGORY_HIDDEN);
			} else {
				m_ObjectsList.SetItemColor(i, 0, COLOR_CATEGORY);
			}
		}

		strSearch.ToLower();

		for ( int nConfig = 0; nConfig < configs.Count(); ++nConfig ) {
			string strConfigPath = configs.Get( nConfig );
			int nClasses = g_Game.ConfigGetChildrenCount( strConfigPath );

			for ( int nClass = 0; nClass < nClasses; ++nClass ) {
				string strName;
				g_Game.ConfigGetChildName( strConfigPath, nClass, strName );

				int scope = g_Game.ConfigGetInt( strConfigPath + " " + strName + " scope" );
				if ( scope == 0 ) continue;
				++classCount;
				if ( !clientAuth.CanSpawn(strName) ) { //Not allowed for this admin's group
					++deniedCount;
					continue;
				}

				string displayName;
				g_Game.ConfigGetText( strConfigPath + " " + strName + " displayName", displayName );

				string strNameLower = strName;
				string displayNameLower = displayName;
				strNameLower.ToLower();
				displayNameLower.ToLower();

				for (int sFilter = 0; sFilter < splitFilters.Count(); ++sFilter) {
					if (m_CategoryHidden.Get(splitFilters[sFilter])) continue;
					if (!GetGame().IsKindOf( strNameLower, splitFilters[sFilter] ) && splitFilters[sFilter] != "All") continue;
					if (strSearch != "" && !(strNameLower.Contains(strSearch) || displayNameLower.Contains(strSearch))) continue;
					if (!m_ExpertMode && (displayName == "" || displayName.Contains("$UNT$"))) continue;

					int catPlace = categoryIndex.Get( splitFilters[sFilter] );
					m_ObjectsList.AddItem(strName + " [" + displayName + "]", new Param2<string, string>("Entry", strName), 0, catPlace + 1);

					//Shift every category header placed below the inserted row
					for (int m = 0; m < categoryIndex.Count(); ++m) {
						if (categoryIndex.GetElement(m) >= catPlace + 1) {
							categoryIndex.Set(categoryIndex.GetKey(m), categoryIndex.GetElement(m) + 1);
						}
					}
				}
			}
		}

		//Make restrictions visible instead of silently missing entries
		if (deniedCount > 0) {
			int infoRow = m_ObjectsList.AddItem(" (" + (classCount - deniedCount) + " of " + classCount + " classes allowed by your group's spawn rules)", new Param2<string, string>("Info", ""), 0);
			m_ObjectsList.SetItemColor(infoRow, 0, COLOR_CATEGORY_HIDDEN);
		}
	}

	//Loadouts mode: the spawn list and its controls make way for the loadout lists
	protected void UpdateLoadoutMode() {
		bool allowed = GetZBerryClientAuth().Has(ZBerryPermissions.LOADOUTS_USE);
		if (!allowed) m_LoadoutMode = false;
		if (m_LoadoutsButton) {
			m_LoadoutsButton.Show(allowed);
			ZBerryUIUtils.SetButtonActive(m_LoadoutsButton, m_LoadoutMode);
		}
		if (m_ContentWrapper) m_ContentWrapper.Show(!m_LoadoutMode);
		if (m_LoadoutsWrapper) m_LoadoutsWrapper.Show(m_LoadoutMode);

		for (int i = 0; i < m_FilterButtons.Count(); ++i) {
			bool active = !m_LoadoutMode && m_SpawnMenuGroups && i < m_SpawnMenuGroups.Count() && m_ObjFilter == m_SpawnMenuGroups[i].BaseClassNames;
			ZBerryUIUtils.SetButtonActive(m_FilterButtons[i], active);
		}

		if (m_LoadoutMode) {
			ShowDropup(false);
			RefreshLoadouts();
		} else {
			DisarmDelete();
		}
		UpdateLoadoutButtons();
	}

	protected void UpdateLoadoutButtons() {
		if (!m_LoadoutSave) return;

		ZBerryClientAuth auth = GetZBerryClientAuth();
		m_LoadoutShare.Show(auth.Has(ZBerryPermissions.LOADOUTS_SHARE));
		bool mayGive = auth.Has(ZBerryPermissions.LOADOUTS_GIVE);
		if (!mayGive) m_ApplyToPlayers = false;
		m_LoadoutTarget.Show(mayGive);
		if (m_ApplyToPlayers) {
			ZBerryUIUtils.SetButtonText(m_LoadoutTarget, "Apply to: selected players");
			m_LoadoutApply.Show(true);
			m_LoadoutUndo.Show(true);
		} else {
			ZBerryUIUtils.SetButtonText(m_LoadoutTarget, "Apply to: myself");
			m_LoadoutApply.Show(auth.Has(ZBerryPermissions.SPAWN_INVENTORY));
			m_LoadoutUndo.Show(GetZBerryClientLoadouts().IsOriginalSaved());
		}
	}

	//Apply: on the admin, or on the selected players after a confirmation
	protected void ApplySelected() {
		if (m_ApplyToPlayers) {
			AskGive();
		} else {
			TrySelected();
		}
	}

	//Players joined, left or were renamed: their rows follow (state updates alone don't rebuild the list)
	protected void OnRoster() {
		if (!m_LoadoutMode || !IsVisible()) return;
		if (BuildRosterKey() != m_RosterKey) RefreshLoadouts();
	}

	//Everyone online but the admin
	protected ZBerryPlayerArray GetOtherPlayers() {
		ZBerryPlayerArray others = new ZBerryPlayerArray;
		ZBerryPlayerArray players = GetZBerryStreams().BuildPlayers();
		int selfId = ZBerryUIUtils.GetLocalPlayerId();
		foreach (ZBerryPlayer ply : players) {
			if (ply.m_PlayerID != selfId) others.Insert(ply);
		}
		return others;
	}

	protected string BuildRosterKey() {
		string key = "";
		ZBerryPlayerArray others = GetOtherPlayers();
		foreach (ZBerryPlayer ply : others) {
			string part = ply.m_PlayerID.ToString() + ":" + ply.m_PlayerName + "|";
			key += part;
		}
		return key;
	}

	void OnLoadoutsChanged() {
		UpdateLoadoutButtons();
		if (m_LoadoutMode) RefreshLoadouts();
	}

	//The admin's own first, then the shared ones. Those with items this admin may not spawn say how many
	protected void RefreshLoadouts() {
		if (!m_LoadoutsList) return;
		m_LoadoutsList.ClearItems();

		ZBerryClientLoadouts loadouts = GetZBerryClientLoadouts();
		string selectedKey = ZBerryLoadoutKit.GetKey(m_SelectedName);
		int selectedRow = -1;

		ZBerryLoadoutArray own = loadouts.GetLocal();
		foreach (ZBerryLoadout mine : own) {
			int ownRestricted = mine.CountRestricted();
			int ownMissing = mine.CountMissing();
			string ownText = mine.Name + DescribeLeftOut(ownRestricted, ownMissing);
			int ownRow = m_LoadoutsList.AddItem(ownText, new Param3<bool, string, int>(false, mine.Name, -1), 0);
			if (ownRestricted + ownMissing > 0) m_LoadoutsList.SetItemColor(ownRow, 0, COLOR_PARTLY);
			if (m_SelectedPlayer < 0 && !m_SelectedShared && ZBerryLoadoutKit.GetKey(mine.Name) == selectedKey) selectedRow = ownRow;
		}

		ZBerryLoadoutHeaderArray shared = loadouts.GetShared();
		foreach (ZBerryLoadoutHeader header : shared) {
			string sharedText = header.LoadoutName + "  (shared by " + header.OwnerName + ")" + DescribeLeftOut(header.RestrictedCount, header.MissingCount);
			int sharedRow = m_LoadoutsList.AddItem(sharedText, new Param3<bool, string, int>(true, header.LoadoutName, -1), 0);
			int color = COLOR_CATEGORY;
			if (header.RestrictedCount + header.MissingCount > 0) color = COLOR_PARTLY;
			m_LoadoutsList.SetItemColor(sharedRow, 0, color);
			if (m_SelectedPlayer < 0 && m_SelectedShared && ZBerryLoadoutKit.GetKey(header.LoadoutName) == selectedKey) selectedRow = sharedRow;
		}

		if (m_LoadoutsList.GetNumItems() == 0) {
			int infoRow = m_LoadoutsList.AddItem("No loadouts yet: type a name, then Save keeps what you wear", new Param3<bool, string, int>(false, "", -1), 0);
			m_LoadoutsList.SetItemColor(infoRow, 0, COLOR_CATEGORY_HIDDEN);
		}

		ZBerryPlayerArray others = GetOtherPlayers();
		m_RosterKey = BuildRosterKey();
		foreach (ZBerryPlayer ply : others) {
			int playerRow = m_LoadoutsList.AddItem(ply.m_PlayerName + "  (wearing now)", new Param3<bool, string, int>(false, ply.m_PlayerName, ply.m_PlayerID), 0);
			m_LoadoutsList.SetItemColor(playerRow, 0, COLOR_PLAYER);
			if (m_SelectedPlayer == ply.m_PlayerID) selectedRow = playerRow;
		}

		if (selectedRow != -1) {
			m_LoadoutsList.SelectRow(selectedRow);
			m_LoadoutsList.EnsureVisible(selectedRow);
		} else {
			m_SelectedName = "";
			m_SelectedPlayer = -1;
		}
		RefreshItems();
	}

	//"  (2 not allowed, 3 not on this server)" or ""
	protected static string DescribeLeftOut(int restricted, int missing) {
		string text = "";
		if (restricted > 0) text = restricted.ToString() + " not allowed";
		if (missing > 0) {
			if (text != "") text += ", ";
			text += missing.ToString() + " not on this server";
		}
		if (text == "") return "";
		return "  (" + text + ")";
	}

	override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn) {
		if (w != m_LoadoutsList) return false;

		Param3<bool, string, int> data; //Shared, name ("" for the info row), a player's session id (-1 for loadouts)
		if (row >= 0) m_LoadoutsList.GetItemData(row, 0, data);
		DisarmDelete();
		if (!data || data.param2 == "") {
			m_SelectedName = "";
			m_SelectedPlayer = -1;
			RefreshItems();
			return true;
		}

		m_SelectedShared = data.param1;
		m_SelectedName = data.param2;
		m_SelectedPlayer = data.param3;
		ZBerryClientLoadouts loadouts = GetZBerryClientLoadouts();
		if (m_SelectedPlayer >= 0) {
			string gearName = m_SelectedName + "'s gear";
			m_LoadoutName.SetText(ZBerryLoadoutKit.CleanName(gearName)); //Save keeps it under this name
			loadouts.RequestPlayer(m_SelectedPlayer); //Again on every selection: what they wear changes
		} else {
			m_LoadoutName.SetText(m_SelectedName);
		}
		if (m_SelectedPlayer < 0 && m_SelectedShared && !loadouts.GetSharedDetails(m_SelectedName)) loadouts.RequestDetails(m_SelectedName);
		RefreshItems();
		return true;
	}

	//What the selected loadout holds, items the spawn rules don't allow and ones this server doesn't have in red
	void RefreshItems() {
		if (!m_LoadoutItemsList) return;
		m_LoadoutItemsList.ClearItems();

		ZBerryClientLoadouts loadouts = GetZBerryClientLoadouts();
		if (loadouts.IsOriginalSaved()) AddInfoRow("Your own gear is saved: Undo puts it back", COLOR_SAVED);
		if (m_SelectedName == "") {
			AddInfoRow("Select a loadout to see what's in it, double-click to apply it", COLOR_CATEGORY_HIDDEN);
			return;
		}

		ZBerryLoadout loadout;
		if (m_SelectedPlayer >= 0) {
			string reason;
			if (!loadouts.GetPlayerGear(m_SelectedPlayer, loadout, reason)) {
				AddInfoRow(m_SelectedName + " is wearing...", COLOR_CATEGORY_HIDDEN);
				return;
			}
			if (!loadout) {
				AddInfoRow("Can't copy " + m_SelectedName + "'s gear: " + reason, COLOR_PARTLY);
				return;
			}
			AddInfoRow(m_SelectedName + " is wearing " + loadout.CountAll() + " items: Save keeps a copy", COLOR_PLAYER);
		} else if (m_SelectedShared) {
			ZBerryLoadoutHeader header = loadouts.FindShared(m_SelectedName);
			if (!header) return;
			AddInfoRow(header.LoadoutName + ": shared by " + header.OwnerName + ", " + header.ItemCount + " items", COLOR_CATEGORY);
			loadout = loadouts.GetSharedDetails(m_SelectedName);
			if (!loadout) {
				AddInfoRow("Loading...", COLOR_CATEGORY_HIDDEN);
				return;
			}
		} else {
			loadout = loadouts.FindLocal(m_SelectedName);
			if (!loadout) return;
			AddInfoRow(loadout.Name + ": yours, " + loadout.CountAll() + " items", COLOR_CATEGORY);
		}

		int restricted = loadout.CountRestricted();
		if (restricted > 0) AddInfoRow(restricted.ToString() + " items your spawn rules don't allow are left out when it's applied", COLOR_PARTLY);
		int missing = loadout.CountMissing();
		if (missing > 0) AddInfoRow(missing.ToString() + " items this server's mods don't have are left out when it's applied", COLOR_PARTLY);
		ZBerryLoadoutRows.Add(m_LoadoutItemsList, loadout.Items, 0);
	}

	protected void AddInfoRow(string text, int color) {
		int row = m_LoadoutItemsList.AddItem(text, NULL, 0);
		m_LoadoutItemsList.SetItemColor(row, 0, color);
	}

	protected bool OnLoadoutClick(Widget w) {
		ZBerryClientLoadouts loadouts = GetZBerryClientLoadouts();
		if (w == m_LoadoutSave) {
			string name = ZBerryLoadoutKit.CleanName(m_LoadoutName.GetText());
			if (name == "") {
				ZBerryUIUtils.Message("Type a name for the loadout first");
				return true;
			}
			if (m_SelectedPlayer >= 0) { //A copy of their gear
				ZBerryLoadout gear = GetSelectedPlayerGear();
				if (gear) loadouts.SaveCopy(gear, name);
				return true;
			}
			m_SelectedShared = false; //Selected once it's saved
			m_SelectedName = name;
			loadouts.SaveCurrentGear(name);
			return true;
		}
		if (w == m_LoadoutShare) {
			if (m_SelectedName == "" || m_SelectedShared || m_SelectedPlayer >= 0) {
				ZBerryUIUtils.Message("Select one of your own loadouts to share it");
			} else {
				loadouts.Share(m_SelectedName);
			}
			return true;
		}
		if (w == m_LoadoutDelete) {
			DeleteSelected();
			return true;
		}
		if (w == m_LoadoutApply) {
			ApplySelected();
			return true;
		}
		if (w == m_LoadoutUndo) {
			if (m_ApplyToPlayers) {
				AskUndo();
			} else {
				loadouts.Restore();
			}
			return true;
		}
		if (w == m_LoadoutTarget) {
			m_ApplyToPlayers = !m_ApplyToPlayers;
			UpdateLoadoutButtons();
			return true;
		}
		return false;
	}

	//The selected player's gear, NULL (and the admin told) while it isn't there
	protected ZBerryLoadout GetSelectedPlayerGear() {
		ZBerryLoadout gear;
		string reason;
		if (!GetZBerryClientLoadouts().GetPlayerGear(m_SelectedPlayer, gear, reason)) {
			ZBerryUIUtils.Message("Wait until " + m_SelectedName + "'s gear is loaded");
			return NULL;
		}
		if (!gear) ZBerryUIUtils.Message("Can't copy " + m_SelectedName + "'s gear: " + reason);
		return gear;
	}

	//The players selected in the players panel, NULL (and the admin told) if nobody is
	protected TIntArray GetGiveTargets() {
		TIntArray targets;
		if (m_PlayersPanel) targets = m_PlayersPanel.GetSelectedPlayerIds();
		if (!targets || targets.Count() == 0) {
			ZBerryUIUtils.Message("Select the players in the list on the left first (Ctrl or Shift + double-click for several)");
			return NULL;
		}
		return targets;
	}

	//"A", "A and B", "A, B and 3 more"
	protected static string DescribePlayers(TIntArray targets) {
		ZBerryStreamClient streams = GetZBerryStreams();
		string text = "";
		for (int i = 0; i < targets.Count(); ++i) {
			if (i == 3 && targets.Count() > 4) {
				int more = targets.Count() - 3;
				text += " and " + more + " more";
				break;
			}
			if (i > 0 && i == targets.Count() - 1) {
				text += " and ";
			} else if (i > 0) {
				text += ", ";
			}
			text += streams.GetName(targets[i]);
		}
		return text;
	}

	protected void AskGive() {
		if (m_SelectedName == "") {
			ZBerryUIUtils.Message("Select a loadout first");
			return;
		}
		if (m_SelectedPlayer >= 0 && !GetSelectedPlayerGear()) return;
		TIntArray targets = GetGiveTargets();
		if (!targets || !m_ConfirmDialog) return;

		m_PendingTargets = targets;
		m_PendingUndo = false;
		string what = m_SelectedName;
		if (m_SelectedPlayer >= 0) what = m_SelectedName + "'s current gear";
		string text = "Put " + what + " on " + DescribePlayers(targets) + "?\n\nWhat they wear now is replaced. It's kept, and Undo gives it back. Your spawn rules apply.";
		m_ConfirmDialog.Open("Give loadout", text, "Give");
		m_ConfirmDialog.OnConfirm.Insert(OnConfirmed);
	}

	protected void AskUndo() {
		TIntArray targets = GetGiveTargets();
		if (!targets || !m_ConfirmDialog) return;

		m_PendingTargets = targets;
		m_PendingUndo = true;
		string text = "Give " + DescribePlayers(targets) + " back what they wore before the first loadout put on them?\n\nWhat they wear now is replaced.";
		m_ConfirmDialog.Open("Undo loadout", text, "Undo");
		m_ConfirmDialog.OnConfirm.Insert(OnConfirmed);
	}

	protected void OnConfirmed() {
		ZBerryClientLoadouts loadouts = GetZBerryClientLoadouts();
		if (m_PendingUndo) {
			loadouts.Undo(m_PendingTargets);
			return;
		}

		if (m_SelectedPlayer >= 0) {
			ZBerryLoadout gear = GetSelectedPlayerGear();
			if (gear) loadouts.Give(false, gear.Name, gear, m_PendingTargets);
		} else if (m_SelectedShared) {
			loadouts.Give(true, m_SelectedName, NULL, m_PendingTargets);
		} else {
			ZBerryLoadout mine = loadouts.FindLocal(m_SelectedName);
			if (mine) loadouts.Give(false, mine.Name, mine, m_PendingTargets);
		}
	}

	protected void TrySelected() {
		if (m_SelectedName == "") {
			ZBerryUIUtils.Message("Select a loadout first");
			return;
		}

		ZBerryClientLoadouts loadouts = GetZBerryClientLoadouts();
		if (m_SelectedPlayer >= 0) {
			ZBerryLoadout gear = GetSelectedPlayerGear();
			if (gear) loadouts.TryLoadout(gear);
		} else if (m_SelectedShared) {
			loadouts.TryShared(m_SelectedName);
		} else {
			loadouts.TryLocal(m_SelectedName);
		}
	}

	//The first click asks, a second one within DELETE_CONFIRM deletes
	protected void DeleteSelected() {
		if (m_SelectedName == "") {
			ZBerryUIUtils.Message("Select a loadout first");
			return;
		}
		if (m_SelectedPlayer >= 0) {
			ZBerryUIUtils.Message("That's what " + m_SelectedName + " wears now, not a loadout");
			return;
		}

		ZBerryClientLoadouts loadouts = GetZBerryClientLoadouts();
		if (m_SelectedShared) {
			ZBerryLoadoutHeader header = loadouts.FindShared(m_SelectedName);
			if (header && !header.IsEditable) {
				ZBerryUIUtils.Message("You may not delete " + header.OwnerName + "'s loadouts");
				return;
			}
		}
		if (m_DeleteTimer < 0) {
			m_DeleteTimer = 0;
			ZBerryUIUtils.SetButtonText(m_LoadoutDelete, "Sure?");
			ZBerryUIUtils.SetButtonTextColor(m_LoadoutDelete, COLOR_RED);
			return;
		}

		DisarmDelete();
		string name = m_SelectedName;
		m_SelectedName = "";
		if (m_SelectedShared) {
			loadouts.DeleteShared(name); //The new list comes from the server
		} else if (loadouts.DeleteLocal(name)) {
			ZBerryUIUtils.Message("Deleted " + name);
		}
		RefreshLoadouts();
	}

	protected void DisarmDelete() {
		m_DeleteTimer = -1;
		if (!m_LoadoutDelete) return;
		ZBerryUIUtils.SetButtonText(m_LoadoutDelete, "Delete");
		ZBerryUIUtils.SetButtonTextColor(m_LoadoutDelete, ZBerryUIUtils.TEXT_DEFAULT);
	}
};
