class ZomberryMenu extends UIScriptedMenu {
	protected TextWidget m_TxtTitle;
	protected TextListboxWidget m_PlayersList;
	protected TextListboxWidget m_FunctionsList;
	protected TextListboxWidget m_ObjectsList;
	protected Widget m_SpawnPage;
	protected EditBoxWidget m_SearchInput;

	protected ButtonWidget m_FunctionsButton;
	protected ButtonWidget m_SpawnButton;

	protected ButtonWidget m_FilterItemsButton;
	protected ButtonWidget m_FilterObjectsButton;
	protected ButtonWidget m_FilterAIButton;

	protected ButtonWidget m_SpawnTargetButton;

	protected autoptr map< string, bool > m_oCategoryHiddenStatus = new map< string, bool >;

	protected int m_lastSelPlayer = -1;

	protected string m_lastSelObj;
	protected string m_objFilter;
	protected string m_objTarget = "Ground";

	void ZomberryMenu() {
		//Print ("[ZomBerryUI] INFO: ZomberryMenu::ZomberryMenu()");
	}

	void ~ZomberryMenu() {
		//Print ("[ZomBerryUI] INFO: ZomberryMenu::~ZomberryMenu()");
	}

	override Widget Init() {
		layoutRoot = GetGame().GetWorkspace().CreateWidgets( "ZomBerry\\AT\\scripts\\5_Mission\\layouts\\AdminDialog.layout" );

		m_TxtTitle = TextWidget.Cast( layoutRoot.FindAnyWidget("TitleText") );
		m_PlayersList = TextListboxWidget.Cast( layoutRoot.FindAnyWidget("PlayerList") );
		m_FunctionsList = TextListboxWidget.Cast( layoutRoot.FindAnyWidget("FunctionsList") );
		m_ObjectsList = TextListboxWidget.Cast( layoutRoot.FindAnyWidget("ObjectsList") );
		m_SpawnPage = layoutRoot.FindAnyWidget( "SpawnWrapper" );

		m_FunctionsButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("FunctionsButton") );
		m_SpawnButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("SpawnButton") );
		m_FilterItemsButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("ItemsButton") );
		m_FilterObjectsButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("ObjectsButton") );
		m_FilterAIButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("AIButton") );
		m_SpawnTargetButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("TargetButton") );

		m_SearchInput = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("SearchInput"));

		m_FunctionsButton.SetTextColor(COLOR_GREEN);
		m_objFilter = "All";

		for ( int i = 0; i < m_FunctionsList.GetNumItems(); ++i ) {
			Param2<string, int> params;
			m_FunctionsList.GetItemData(i, 0, params);
			m_FunctionsList.SetItemColor(i, 0, params.param2);
		}

		return layoutRoot;
	}

	override void OnShow() {
		super.OnShow();
		GetGame().GetInput().ChangeGameFocus( 1 );

		UpdateObjList("All");

		m_TxtTitle.SetText( "Players in game: ... | ZomBerry Admin Tools v" + g_zbryVer + " by Vaker" );
		GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayersRequest", new Param1< int >( 0 ), true, NULL );
		GetRPCManager().SendRPC( "ZomBerryAT", "SyncFunctionsRequest", new Param1< int >( 0 ), true, NULL );

		if ( m_objTarget != "Ground" ) m_SpawnTargetButton.SetText( m_objTarget );
	}

	override void OnHide() {
		super.OnHide();
		GetGame().GetInput().ResetGameFocus();

		m_PlayersList.ClearItems();
		m_FunctionsList.ClearItems();
	}

	override bool OnChange( Widget w, int x, int y, bool finished ) {
		if ( w == m_SearchInput ){
			UpdateObjList( m_objFilter );
			return true;
		}
		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y) {
		return false;
	}

	override bool OnMouseLeave( Widget w, Widget enterW, int x, int y ) {
		return false;
	}

	override bool OnKeyPress( Widget w, int x, int y, int key ) {
		return false;
	}

	override bool OnClick( Widget w, int x, int y, int button ) {
		string ItemsFilter = "Edible_Base,Weapon_Base,Magazine_Base,Clothing_Base";
		string ObjectsFilter = "Transport,House";
		string AIFilter = "DZ_LightAI";

		if ( w == m_FunctionsButton ) {
			m_FunctionsButton.SetTextColor(COLOR_GREEN);
			m_SpawnButton.SetTextColor(0xFFFFFFFF);
			m_FunctionsList.Show( true );
			m_SpawnPage.Show( false );
		}

		if ( w == m_SpawnButton ) {
			m_FunctionsButton.SetTextColor(0xFFFFFFFF);
			m_SpawnButton.SetTextColor(COLOR_GREEN);
			m_FunctionsList.Show( false );
			m_SpawnPage.Show( true );
		}

		if ( w == m_FilterItemsButton ) {
			if (m_objFilter == ItemsFilter) {
				m_FilterItemsButton.SetTextColor(0xFFFFFFFF);
				m_objFilter = "All";
			} else {
				m_FilterItemsButton.SetTextColor(COLOR_GREEN);
				m_objFilter = ItemsFilter;
			}

			m_FilterObjectsButton.SetTextColor(0xFFFFFFFF);
			m_FilterAIButton.SetTextColor(0xFFFFFFFF);
			UpdateObjList( m_objFilter );
		}

		if ( w == m_FilterObjectsButton ) {
			if (m_objFilter == ObjectsFilter) {
				m_FilterObjectsButton.SetTextColor(0xFFFFFFFF);
				m_objFilter = "All";
			} else {
				m_FilterObjectsButton.SetTextColor(COLOR_GREEN);
				m_objFilter = ObjectsFilter;
			}

			m_FilterItemsButton.SetTextColor(0xFFFFFFFF);
			m_FilterAIButton.SetTextColor(0xFFFFFFFF);
			UpdateObjList( m_objFilter );
		}

		if ( w == m_FilterAIButton ) {
			if (m_objFilter == AIFilter) {
				m_FilterAIButton.SetTextColor(0xFFFFFFFF);
				m_objFilter = "All";
			} else {
				m_FilterAIButton.SetTextColor(COLOR_GREEN);
				m_objFilter = AIFilter;
			}

			m_FilterItemsButton.SetTextColor(0xFFFFFFFF);
			m_FilterObjectsButton.SetTextColor(0xFFFFFFFF);
			UpdateObjList( m_objFilter );
		}

		if ( w == m_SpawnTargetButton ) {
			if (m_objTarget == "Ground") {
				m_objTarget = "Cursor";
			} else if (m_objTarget == "Cursor") {
				m_objTarget = "Inventory";
			} else m_objTarget = "Ground";
			m_SpawnTargetButton.SetText(m_objTarget);
		}

		return false;
	}

	override bool OnDoubleClick( Widget w, int x, int y, int button ) {
		Param2<int, bool> plyData;
		Man adminPly = GetGame().GetPlayer();
		int adminId;
		if (GetGame().IsMultiplayer()) {
			adminId = adminPly.GetIdentity().GetPlayerId();
		} else {
			adminId = 0;
		}

		if ( w == m_PlayersList ) {

			if ( m_lastSelPlayer != -1 ) {
				m_PlayersList.GetItemData( m_lastSelPlayer, 0, plyData );

			    if ( plyData.param2 ) {
					m_PlayersList.SetItemColor( m_lastSelPlayer, 0, 0xFF47EB00 );
				} else {
					m_PlayersList.SetItemColor( m_lastSelPlayer, 0, 0xFFFFFFFF );
				}
			};

			m_lastSelPlayer = m_PlayersList.GetSelectedRow();
			if ( m_lastSelPlayer == -1 ) return true;

			m_PlayersList.SetItemColor( m_lastSelPlayer, 0, 0xFFFF751A );
		}

		if ( w == m_FunctionsList ) {
			Param2<string, bool> funcData;

			int targetId;
			string funcName;
			TIntArray funcParams;

			int selectedFuncRow = m_FunctionsList.GetSelectedRow();
			if ( selectedFuncRow == -1 ) return true;

			m_FunctionsList.GetItemData( selectedFuncRow, 0, funcData );
			funcName = funcData.param1;
			funcParams = {1, 2, 3};

			if (funcName == "Category") return true;

			if ( funcData.param2 ) {
				if ( m_lastSelPlayer == -1 ) { Message("No player selected"); return true; }

				m_PlayersList.GetItemData( m_lastSelPlayer, 0, plyData );
				targetId = plyData.param1;
			} else {
				targetId = adminId;
			}

			GetRPCManager().SendRPC( "ZomBerryAT", "ExecuteCommand", new Param5< string, int, int, vector, ref TIntArray >( funcName, adminId, targetId, GetCursorPos(), funcParams ), true );
		}

		if ( w == m_ObjectsList ) {
			string currentObject;

			int selectedObjRow = m_ObjectsList.GetSelectedRow();
			if ( selectedObjRow == -1 ) return true;

			m_ObjectsList.GetItemText(selectedObjRow, 0, currentObject);

			//if (m_lastSelObj == currentObject) {
			Param1<string> currentObjectParam;
			m_ObjectsList.GetItemData(selectedObjRow, 0, currentObjectParam);
			string currentObjectData = currentObjectParam.param1;

			if (currentObjectData == "Category") {
				currentObject.Replace(" == ", "");
				bool catStatus = m_oCategoryHiddenStatus.Get(currentObject);
				m_oCategoryHiddenStatus.Set(currentObject, !catStatus);
				UpdateObjList(m_objFilter);
			}
			if (currentObjectData == "Entry") {
				vector targetPlace = GetCursorPos();
				bool targetInventory = false;
				if (m_objTarget == "Ground") targetPlace = adminPly.GetPosition();
				if (m_objTarget == "Inventory") targetInventory = true;

				GetRPCManager().SendRPC( "ZomBerryAT", "SpawnObject", new Param4< string, int, vector, bool >( currentObject, adminId, targetPlace, targetInventory ), true );
			}
			m_lastSelObj = "";
			//} else {
			//  m_lastSelObj = currentObject;
			//}
		}
		return true;
	}

	static void Message( string txt ) {
		GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "[ZomBerry]", txt, ""));
	}

	vector GetCursorPos() { //<3 Arkensor and CO
		if ( !GetGame().GetPlayer() ) { return "0 0 0"; }

		vector rayStart = GetGame().GetCurrentCameraPosition();
		vector rayEnd = rayStart + GetGame().GetCurrentCameraDirection() * 10000;
		vector hitPos; vector hitNormal; int hitComponentIndex;
		DayZPhysics.RaycastRV(rayStart, rayEnd, hitPos, hitNormal, hitComponentIndex, NULL, NULL, GetGame().GetPlayer());

		return hitPos;
	}

	void SyncPlayers( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param1<ref ZBerryPlayerArray> playerListS;
		ref ZBerryPlayerArray playerListC;

		if ( type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer() ) {
			if ( !ctx.Read( playerListS ) ) return;

			playerListC = playerListS.param1;
		}

		if (layoutRoot.IsVisible()) {
			int playerId;
			bool playerAdmin;
			string playerName;

			m_TxtTitle.SetText( "Players in game: " + playerListC.Count() + " | ZomBerry Admin Tools v" + g_zbryVer + " by Vaker");

			for ( int i = 0; i < playerListC.Count(); ++i ) {
				playerId = playerListC.Get(i).m_PlayerID;
				playerName = playerListC.Get(i).m_PlayerName;
				playerAdmin = playerListC.Get(i).m_IsAdmin;

				m_PlayersList.AddItem(" " + i.ToString() + ": " + playerName, new Param2<int, bool>(playerId, playerAdmin), 0);

				if (playerAdmin) m_PlayersList.SetItemColor( i, 0, 0xFF47EB00 );
			}

			if ( m_lastSelPlayer != -1 ) m_PlayersList.SetItemColor( m_lastSelPlayer, 0, 0xFFFF751A );
		}
	}

	void SyncFunctions( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param1<ref ZBerryCategoryArray> catListS;
		ref ZBerryCategoryArray catList;

		if ( type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer() ) {
			if ( !ctx.Read( catListS ) ) return;

			catList = catListS.param1;
		}

		int lastId = 0;
		string entryName = "";

		for (int i = 0; i < catList.Count(); ++i) {
			ref ZBerryCategory singleCat = catList.Get(i);

			entryName = singleCat.GetName();
			m_FunctionsList.AddItem( "==== "+entryName+" ====", new Param2<string, bool>("Category", false), 0, lastId );

			++lastId;

			ref ZBerryFunctionArray funcArray;
			funcArray = catList.Get(i).GetAll();

			for (int j = 0; j < funcArray.Count(); ++j) {
				ref ZBerryFunction funcData;
				funcData = funcArray.Get(j);
				entryName = funcData.GetName();

				m_FunctionsList.AddItem( funcData.GetDisplayName(), new Param2<string, bool>(entryName, funcData.IsTargetRequired()), 0, lastId );
				m_FunctionsList.SetItemColor(lastId, 0, funcData.GetColor());

				++lastId;
			}
		}
	}

	void UpdateObjList( string classFilter ) { // All default
		m_ObjectsList.ClearItems();

		TStringArray configs = new TStringArray;
		configs.Insert( CFG_VEHICLESPATH );
		configs.Insert( CFG_WEAPONSPATH );
		configs.Insert( CFG_MAGAZINESPATH );

		autoptr map< string, int > m_oCategoryIndex = new map< string, int >;
		TStringArray splitFilters = new TStringArray;
		classFilter.Split(",", splitFilters);
		string strSearch = m_SearchInput.GetText();

		if (!m_oCategoryHiddenStatus.Contains(splitFilters[0])) {
			for (int pop = 0; pop < splitFilters.Count(); ++pop) {
				m_oCategoryHiddenStatus.Insert( splitFilters[pop], false );
			}
		}

		for (int i = 0; i < splitFilters.Count(); ++i) {
			int color;
			m_oCategoryIndex.Insert( splitFilters[i], i );
			m_ObjectsList.AddItem( " == "+splitFilters[i]+" == ", new Param1<string>("Category"), 0, i );

			if (m_oCategoryHiddenStatus.Get(splitFilters[i])) {color = 0xFF6767E0;} else {color = 0xFFBCCDE8;};
			m_ObjectsList.SetItemColor(i, 0, color);
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

				if ( strName == "Mag_Scout_5Rnd") continue; // fix crash for this dumb item. dont spawn it

				string strNameLower = strName;

				strNameLower.ToLower();
				for (int sFilter = 0; sFilter < splitFilters.Count(); ++sFilter) {
					if (m_oCategoryHiddenStatus.Get(splitFilters[sFilter])) continue;

					if ( GetGame().IsKindOf( strNameLower, splitFilters[sFilter] ) || splitFilters[sFilter] == "All" ) {
						if ( (strSearch != "" && (!strNameLower.Contains( strSearch ))) ) continue;

						if ( strName == "ItemOptics" ) continue; // Fix crash

						int catPlace = m_oCategoryIndex.Get( splitFilters[sFilter] );
						m_ObjectsList.AddItem( strName, new Param1<string>( "Entry" ), 0, catPlace+1);
						//Print("[VKRDBG] Placed " + strName + " after " + splitFilters[sFilter] + " at pos " + (catPlace+1).ToString());

						for (int m = 0; m < m_oCategoryIndex.Count(); ++m) { //m+1
							if (m_oCategoryIndex.GetElement(m) >= catPlace+1) {
								m_oCategoryIndex.Set(m_oCategoryIndex.GetKey(m), m_oCategoryIndex.GetElement(m)+1);
							}
						}
					}
				}
			}
		}
	}
}
