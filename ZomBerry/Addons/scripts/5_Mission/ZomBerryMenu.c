class ZomberryMenu extends UIScriptedMenu {
	protected TextWidget m_TxtTitle, m_FncName, m_MapText;
	protected TextListboxWidget m_PlayersList;
	protected TextListboxWidget m_FunctionsList;
	protected TextListboxWidget m_ObjectsList;
	protected Widget m_SpawnPage, m_FunctionsPage, m_MapPage;
	protected EditBoxWidget m_SearchInput;

	protected TextWidget m_FuncPName0, m_FuncPName1, m_FuncPName2;
	protected SliderWidget m_FuncPSlider0, m_FuncPSlider1, m_FuncPSlider2;
	protected EditBoxWidget m_FuncPEBox0, m_FuncPEBox1, m_FuncPEBox2;

	protected ButtonWidget m_FunctionsButton, m_SpawnButton, m_MapButton;
	protected ButtonWidget m_FilterItemsButton, m_FilterObjectsButton, m_FilterAIButton;
	protected ButtonWidget m_SpawnTargetButton, m_ExecFuncButton;

	protected MapWidget m_MapWidget;

	protected autoptr map< string, bool > m_oCategoryHiddenStatus = new map< string, bool >;

	protected int m_lastSelPlayer = -1;
	protected int m_lastSelFunc = -1;
	protected bool m_mouseOnMap = false;
	protected bool m_mapTPAllowed = true;
	protected autoptr TIntArray m_lastFuncParams = {0,0,0};
	protected autoptr array<ref ZBerryFuncParamArray> m_funcParams = new array<ref ZBerryFuncParamArray>;

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
		m_FunctionsPage = layoutRoot.FindAnyWidget( "FunctionsWrapper" );
		m_MapPage = layoutRoot.FindAnyWidget( "MapWrapper" );

		m_FunctionsButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("FunctionsButton") );
		m_SpawnButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("SpawnButton") );
		m_MapButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("MapButton") );
		m_FilterItemsButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("ItemsButton") );
		m_FilterObjectsButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("ObjectsButton") );
		m_FilterAIButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("AIButton") );
		m_SpawnTargetButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("TargetButton") );
		m_ExecFuncButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("SecondaryExecButton") );

		m_FuncPName0 = TextWidget.Cast( layoutRoot.FindAnyWidget("TextFuncParam0") );
		m_FuncPName1 = TextWidget.Cast( layoutRoot.FindAnyWidget("TextFuncParam1") );
		m_FuncPName2 = TextWidget.Cast( layoutRoot.FindAnyWidget("TextFuncParam2") );

		m_FuncPSlider0 = SliderWidget.Cast( layoutRoot.FindAnyWidget("SliderFuncParam0") );
		m_FuncPSlider1 = SliderWidget.Cast( layoutRoot.FindAnyWidget("SliderFuncParam1") );
		m_FuncPSlider2 = SliderWidget.Cast( layoutRoot.FindAnyWidget("SliderFuncParam2") );

		m_FuncPEBox0 = EditBoxWidget.Cast( layoutRoot.FindAnyWidget("EditBoxFuncParam0") );
		m_FuncPEBox1 = EditBoxWidget.Cast( layoutRoot.FindAnyWidget("EditBoxFuncParam1") );
		m_FuncPEBox2 = EditBoxWidget.Cast( layoutRoot.FindAnyWidget("EditBoxFuncParam2") );

		m_FncName = TextWidget.Cast( layoutRoot.FindAnyWidget("FuncName") );
		m_MapText = TextWidget.Cast( layoutRoot.FindAnyWidget("MapText") );

		m_SearchInput = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("SearchInput"));

		m_MapWidget = MapWidget.Cast( layoutRoot.FindAnyWidget("MapWidget") );

		m_FncName.SetText("");
		SetParams(0, 0);

		m_FunctionsButton.SetTextColor(COLOR_GREEN);
		m_objFilter = "All";
		m_MapPage.Show(false);

		for ( int i = 0; i < m_FunctionsList.GetNumItems(); ++i ) {
			Param3<string, int, ref ZBerryFuncParamArray> params;
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
		m_lastSelFunc = -1;
	}

	override bool OnMouseLeave( Widget w, Widget enterW, int x, int y ) {
		if ( w == m_FuncPEBox0 || w == m_FuncPEBox1 || w == m_FuncPEBox2 ) {
			UpdateFuncBoxes(m_lastFuncParams[0], m_lastFuncParams[1], m_lastFuncParams[2]);
			return true;
		}
		if ( w == m_MapWidget ) {
			m_mouseOnMap = false;
			return true;
		}
		return false;
	}

	override bool OnMouseEnter( Widget w, int x, int y ) {
		if ( w == m_MapWidget ) {
			m_mouseOnMap = true;
			return true;
		}
		return false;
	}

	override bool OnChange( Widget w, int x, int y, bool finished ) {
		if ( w == m_SearchInput ) {
			UpdateObjList( m_objFilter );
			return true;
		}
		if ( w == m_FuncPSlider0 || w == m_FuncPSlider1 || w == m_FuncPSlider2 ) {
			Param3<string, bool, int> funcData;
			int fParamId = -1;
			TIntArray minVs = {0, 0, 0,};
			TIntArray maxVs = {1000, 1000, 1000,};

			m_FunctionsList.GetItemData( m_lastSelFunc, 0, funcData );
			fParamId = funcData.param3;
			if (fParamId == -1) return true;

			for (int i = 0; i < m_funcParams[fParamId].Count(); i++) {
				minVs[i] = m_funcParams[fParamId].Get(i).values.Get(0);
				maxVs[i] = m_funcParams[fParamId].Get(i).values.Get(1);
			}

			m_lastFuncParams = NormalizeFuncValues({
				Math.Round((m_FuncPSlider0.GetCurrent()/100*(maxVs[0]-minVs[0])) + minVs[0] ),
				Math.Round((m_FuncPSlider1.GetCurrent()/100*(maxVs[1]-minVs[1])) + minVs[1] ),
				Math.Round((m_FuncPSlider2.GetCurrent()/100*(maxVs[2]-minVs[2])) + minVs[2] ),
			});
			UpdateFuncBoxes(m_lastFuncParams[0], m_lastFuncParams[1], m_lastFuncParams[2]);
			return true;
		}
		if ( w == m_FuncPEBox0 || w == m_FuncPEBox1 || w == m_FuncPEBox2 ) {
			m_lastFuncParams = NormalizeFuncValues ({
				Math.Round(m_FuncPEBox0.GetText().ToInt()),
				Math.Round(m_FuncPEBox1.GetText().ToInt()),
				Math.Round(m_FuncPEBox2.GetText().ToInt()),
			});
			UpdateFuncSliders(m_lastFuncParams[0], m_lastFuncParams[1], m_lastFuncParams[2]);
			return true;
		}
		return false;
	}

	override void Update(float tDelta) {
		if (m_MapPage.IsVisible() && m_mouseOnMap) {
			int sX, sY;
			vector mapXZY;
			GetMousePos(sX, sY);
			mapXZY = m_MapWidget.ScreenToMap(Vector(sX, sY, 0));
			m_MapText.SetText("X: " + mapXZY[0] + "  Y: " + mapXZY[2]);

			if ((KeyState(KeyCode.KC_LMENU) & 0x00000001) && GetMouseState(MouseState.LEFT) < 0) { //OnClick doesn't work inside MapWidget
				if (m_mapTPAllowed) {
					m_mapTPAllowed = false;
					GetRPCManager().SendRPC( "ZomBerryAT", "MapTeleport", new Param1< vector >( mapXZY ), true );
				}
			} else {
				if (!m_mapTPAllowed) m_mapTPAllowed = true;
			}
		}
	}

	override bool OnClick( Widget w, int x, int y, int button ) {
		string ItemsFilter = "Edible_Base,Weapon_Base,Magazine_Base,Clothing_Base";
		string ObjectsFilter = "Transport,House";
		string AIFilter = "DZ_LightAI";

		if ( w == m_FunctionsButton ) {
			m_FunctionsButton.SetTextColor(COLOR_GREEN);
			m_SpawnButton.SetTextColor(0xFFFFFFFF);
			m_MapButton.SetTextColor(0xFFFFFFFF);
			m_FunctionsPage.Show( true );
			m_SpawnPage.Show( false );
			m_MapPage.Show( false );
		}

		if ( w == m_SpawnButton ) {
			m_FunctionsButton.SetTextColor(0xFFFFFFFF);
			m_SpawnButton.SetTextColor(COLOR_GREEN);
			m_MapButton.SetTextColor(0xFFFFFFFF);
			m_FunctionsPage.Show( false );
			m_SpawnPage.Show( true );
			m_MapPage.Show( false );
		}

		if ( w == m_MapButton ) {
			m_FunctionsButton.SetTextColor(0xFFFFFFFF);
			m_SpawnButton.SetTextColor(0xFFFFFFFF);
			m_MapButton.SetTextColor(COLOR_GREEN);
			m_FunctionsPage.Show( false );
			m_SpawnPage.Show( false );
			m_MapPage.Show( true );
		}

		if ( w == m_FunctionsList ) {
			Param3<string, bool, int> funcData;
			string fncName;
			int fParamId = -1;

			TIntArray defValues = {250, 500, 750};

			int selectedFuncRow = m_FunctionsList.GetSelectedRow();
			if ( selectedFuncRow == -1 ) return true;
			if ( selectedFuncRow == m_lastSelFunc ) return true;
			m_lastSelFunc = selectedFuncRow;

			m_FunctionsList.GetItemData( selectedFuncRow, 0, funcData );
			m_FunctionsList.GetItemText( selectedFuncRow, 0, fncName );
			fParamId = funcData.param3;

			if (fParamId == -1) return true;

			m_FncName.SetText(fncName);

			for (int idx = 0; idx < 3; ++idx) {
				if (m_funcParams[fParamId].Count() > idx) {
					defValues[idx] = m_funcParams[fParamId].Get(idx).values.Get(2);
				}
			}

			m_lastFuncParams = NormalizeFuncValues(defValues);
			UpdateFuncValues(m_lastFuncParams);
			SetParams(m_funcParams[fParamId].Count(), fParamId);
		}

		if ( w == m_ExecFuncButton ) {
			OnDoubleClick( m_FunctionsList, 0, 0, 0 );
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

			int selectedFuncRow = m_FunctionsList.GetSelectedRow();
			if ( selectedFuncRow == -1 ) return true;

			m_FunctionsList.GetItemData( selectedFuncRow, 0, funcData );
			funcName = funcData.param1;

			if (funcName == "Category") return true;

			if ( funcData.param2 ) {
				if ( m_lastSelPlayer == -1 ) { Message("No player selected"); return true; }

				m_PlayersList.GetItemData( m_lastSelPlayer, 0, plyData );
				targetId = plyData.param1;
			} else {
				if ( m_lastSelPlayer == -1 ) {
					targetId = adminId;
				} else {
					m_PlayersList.GetItemData( m_lastSelPlayer, 0, plyData );
					targetId = plyData.param1;
				}
			}

			GetRPCManager().SendRPC( "ZomBerryAT", "ExecuteCommand", new Param5< string, int, int, vector, autoptr TIntArray >( funcName, adminId, targetId, GetCursorPos(), m_lastFuncParams ), true );
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

	private TIntArray NormalizeFuncValues(TIntArray abNormalValues) {
		Param3<string, bool, int> funcData;
		int minV, maxV;
		TIntArray normalValues = {0, 0, 0,};
		int fParamId = -1;

		m_FunctionsList.GetItemData( m_lastSelFunc, 0, funcData );
		fParamId = funcData.param3;

		if (fParamId == -1) return normalValues;

		for (int i = 0; i < m_funcParams[fParamId].Count(); i++) {
			minV = m_funcParams[fParamId].Get(i).values.Get(0);
			maxV = m_funcParams[fParamId].Get(i).values.Get(1);
			normalValues[i] = Math.Min(maxV, Math.Max(minV, abNormalValues[i]));
		}

		return normalValues;
	}

	private void UpdateFuncValues(TIntArray newValues) {
		UpdateFuncSliders(newValues[0], newValues[1], newValues[2]);
		UpdateFuncBoxes(newValues[0], newValues[1], newValues[2]);
	}

	private void UpdateFuncSliders(int p1, int p2, int p3) {
		Param3<string, bool, int> funcData;
		TIntArray minVs = {0, 0, 0,};
		TIntArray maxVs = {1000, 1000, 1000,};
		int fParamId = -1;

		m_FunctionsList.GetItemData( m_lastSelFunc, 0, funcData );
		fParamId = funcData.param3;

		if (fParamId == -1) return;

		for (int i = 0; i < m_funcParams[fParamId].Count(); i++) {
			minVs[i] = m_funcParams[fParamId].Get(i).values.Get(0);
			maxVs[i] = m_funcParams[fParamId].Get(i).values.Get(1);
		}

		m_FuncPSlider0.SetCurrent(((p1-minVs[0])/(maxVs[0]-minVs[0]))*100);
		m_FuncPSlider1.SetCurrent(((p2-minVs[1])/(maxVs[1]-minVs[1]))*100);
		m_FuncPSlider2.SetCurrent(((p3-minVs[2])/(maxVs[2]-minVs[2]))*100);
	}

	private void UpdateFuncBoxes(int p1, int p2, int p3) {
		m_FuncPEBox0.SetText(p1.ToString());
		m_FuncPEBox1.SetText(p2.ToString());
		m_FuncPEBox2.SetText(p3.ToString());
	}

	private void SetParams(int pCount, int fParamId) {
		if (pCount >= 1) {
			m_FuncPName0.SetText(m_funcParams[fParamId].Get(0).name);
			m_FuncPName0.Show(true); m_FuncPSlider0.Show(true); m_FuncPEBox0.Show(true);
		} else {
			m_FuncPName0.Show(false); m_FuncPSlider0.Show(false); m_FuncPEBox0.Show(false);
		}

		if (pCount >= 2) {
			m_FuncPName1.SetText(m_funcParams[fParamId].Get(1).name);
			m_FuncPName1.Show(true); m_FuncPSlider1.Show(true); m_FuncPEBox1.Show(true);
		} else {
			m_FuncPName1.Show(false); m_FuncPSlider1.Show(false); m_FuncPEBox1.Show(false);
		}

		if (pCount >= 3) {
			m_FuncPName2.SetText(m_funcParams[fParamId].Get(2).name);
			m_FuncPName2.Show(true); m_FuncPSlider2.Show(true); m_FuncPEBox2.Show(true);
		} else {
			m_FuncPName2.Show(false); m_FuncPSlider2.Show(false); m_FuncPEBox2.Show(false);
		}
	}

	void SyncPlayers( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param2<ref ZBerryPlayerArray, int> playerListS;
		ref ZBerryPlayerArray playerListC;
		int sUpTime = 0;

		if ( type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer() ) {
			if ( !ctx.Read( playerListS ) ) {
				Message("Player sync data read error - possible version mismatch");
				return;
			}

			playerListC = playerListS.param1;
			sUpTime = Math.Round(playerListS.param2/1000);
		}

		if (layoutRoot.IsVisible()) {
			int playerId;
			bool playerAdmin;
			string playerName;
			vector playerPos;

			m_TxtTitle.SetText( "Players in game: " + playerListC.Count() + " | " + Math.Floor(sUpTime / 3600).ToString() + "h " + ((sUpTime / 60) % 60).ToString() + "min | ZomBerry Admin Tools v" + g_zbryVer + " by Vaker");
			m_MapWidget.ClearUserMarks();

			for ( int i = 0; i < playerListC.Count(); ++i ) {
				playerId = playerListC.Get(i).m_PlayerID;
				playerName = playerListC.Get(i).m_PlayerName;
				playerAdmin = playerListC.Get(i).m_IsAdmin;
				playerPos = playerListC.Get(i).m_PlayerPos;

				m_PlayersList.AddItem(" " + i.ToString() + ": " + playerName, new Param2<int, bool>(playerId, playerAdmin), 0);

				if (playerAdmin) {
					m_PlayersList.SetItemColor( i, 0, 0xFF47EB00 );
					m_MapWidget.AddUserMark(playerPos, playerName, ARGB(255,71,235,0), "\\dz\\gear\\navigation\\data\\map_tree_ca.paa");
				} else {
					m_MapWidget.AddUserMark(playerPos, playerName, ARGB(255,85,170,255), "\\dz\\gear\\navigation\\data\\map_tree_ca.paa");
				}
			}

			if ( m_lastSelPlayer != -1 && m_lastSelPlayer < playerListC.Count() ) m_PlayersList.SetItemColor( m_lastSelPlayer, 0, 0xFFFF751A );
		}
	}

	void SyncFunctions( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param1<ref ZBerryCategoryArray> catListS;
		ref ZBerryCategoryArray catList;

		if ( type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer() ) {
			if ( !ctx.Read( catListS ) ) return;

			catList = catListS.param1;
			m_funcParams.Clear();
		}

		int lastId = 0;
		string entryName = "";

		for (int i = 0; i < catList.Count(); ++i) {
			ref ZBerryCategory singleCat = catList.Get(i);

			entryName = singleCat.GetName();
			m_FunctionsList.AddItem( "==== "+entryName+" ====", new Param3<string, bool, int>("Category", false, -1), 0, lastId );

			++lastId;

			ref ZBerryFunctionArray funcArray;
			funcArray = catList.Get(i).GetAll();

			for (int j = 0; j < funcArray.Count(); ++j) {
				ref ZBerryFunction funcData;
				funcData = funcArray.Get(j);
				entryName = funcData.GetName();

				int fParamId = m_funcParams.Insert(funcData.GetParams());

				m_FunctionsList.AddItem( funcData.GetDisplayName(), new Param3<string, bool, int>(entryName, funcData.IsTargetRequired(), fParamId), 0, lastId );
				m_FunctionsList.SetItemColor(lastId, 0, funcData.GetColor());

				++lastId;
			}
		}
	}

	private void UpdateObjList( string classFilter ) { // All default
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
