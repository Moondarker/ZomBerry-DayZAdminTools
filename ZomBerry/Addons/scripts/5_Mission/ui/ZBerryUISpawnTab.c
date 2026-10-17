//Object spawner: category filters, search, spawn target and Safe/Expert mode
class ZBerryUISpawnTab extends ZBerryUITab {
	static const int COLOR_CATEGORY = 0xFFBCCDE8;
	static const int COLOR_CATEGORY_HIDDEN = 0xFF6767E0;

	protected TextListboxWidget m_ObjectsList;
	protected EditBoxWidget m_SearchInput;
	protected ButtonWidget m_TargetButton;
	protected ref array<ButtonWidget> m_FilterButtons = new array<ButtonWidget>;

	protected ref ZBerryJsonSpawnMenuGroupArray m_SpawnMenuGroups;
	protected ref map<string, bool> m_CategoryHidden = new map<string, bool>;

	protected string m_ObjFilter = "All";
	protected string m_ObjTarget = "Cursor";
	protected bool m_ExpertMode = false;
	protected bool m_MouseOnSearch = false;
	protected bool m_ListDirty = true;

	protected override string GetLayoutName() {
		return "SpawnTab.layout";
	}

	protected override void OnInit() {
		m_ObjectsList = TextListboxWidget.Cast( BindWidget("ObjectsList") );
		m_SearchInput = EditBoxWidget.Cast( BindWidget("SearchInput") );
		m_TargetButton = ButtonWidget.Cast( BindWidget("TargetButton") );

		m_FilterButtons.Insert( ButtonWidget.Cast( BindWidget("ItemsButton") ) );
		m_FilterButtons.Insert( ButtonWidget.Cast( BindWidget("ObjectsButton") ) );
		m_FilterButtons.Insert( ButtonWidget.Cast( BindWidget("AIButton") ) );
	}

	protected override void OnActivate() {
		UpdateModeButton();
		if (m_ListDirty) UpdateObjList();
	}

	override void OnMenuShow() {
		m_SpawnMenuGroups = ZomberryBase.GetConfig().GetSpawnMenuData();

		for (int i = 0; i < m_FilterButtons.Count(); ++i) {
			if (i < m_SpawnMenuGroups.Count()) {
				m_FilterButtons[i].SetText(m_SpawnMenuGroups[i].CategoryName);
				m_FilterButtons[i].Show(true);
			} else {
				m_FilterButtons[i].Show(false);
			}
		}

		m_TargetButton.SetText(m_ObjTarget);

		//Config list is huge, so only rebuild it when the tab is actually visible
		m_ListDirty = true;
		if (IsVisible()) UpdateObjList();
	}

	override bool BlocksClose() {
		return m_MouseOnSearch;
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
			ToggleFilter(filterIdx);
			return true;
		}

		if (w == m_TargetButton) {
			if (m_ObjTarget == "Ground") {
				m_ObjTarget = "Cursor";
			} else if (m_ObjTarget == "Cursor") {
				m_ObjTarget = "Inventory";
			} else {
				m_ObjTarget = "Ground";
			}
			m_TargetButton.SetText(m_ObjTarget);
			return true;
		}
		return false;
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button) {
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
			bool targetInventory = false;
			if (m_ObjTarget == "Ground") targetPlace = GetGame().GetPlayer().GetPosition();
			if (m_ObjTarget == "Inventory") targetInventory = true;

			GetRPCManager().SendRPC( "ZomBerryAT", "SpawnObject", new Param4< string, int, vector, bool >( objData.param2, ZBerryUIUtils.GetLocalPlayerId(), targetPlace, targetInventory ), true );
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

	override bool OnMouseEnter(Widget w, int x, int y) {
		if (w == m_SearchInput) {
			m_MouseOnSearch = true;
			return true;
		}
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y) {
		if (w == m_SearchInput) {
			m_MouseOnSearch = false;
			return true;
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
	}
};
