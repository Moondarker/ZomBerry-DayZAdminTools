class ZomberryMenu extends UIScriptedMenu {
	protected TextWidget m_TxtTitle, m_FncName, m_MapText, m_PlyHealth, m_PlyBlood;
	protected TextListboxWidget m_PlayersList;
	protected TextListboxWidget m_FunctionsList;
	protected TextListboxWidget m_ObjectsList;
	protected Widget m_SpawnPage, m_FunctionsPage, m_MapPage;
	protected EditBoxWidget m_SearchInput;

	protected TextWidget m_FuncPName0, m_FuncPName1, m_FuncPName2;
	protected SliderWidget m_FuncPSlider0, m_FuncPSlider1, m_FuncPSlider2;
	protected EditBoxWidget m_FuncPEBox0, m_FuncPEBox1, m_FuncPEBox2;

	protected ButtonWidget m_FunctionsButton, m_SpawnButton, m_MapButton, m_MultiButton;
	protected ButtonWidget m_FilterCatOneButton, m_FilterCatTwoButton, m_FilterCatThreeButton;
	protected ButtonWidget m_SpawnTargetButton, m_ExecFuncButton, m_PlayersRefresh;

	protected MapWidget m_MapWidget;
	protected vector m_MapPos;
	protected bool m_MapDiff;
	protected float m_MapScale; 

	protected autoptr map< string, bool > m_oCategoryHiddenStatus = new map< string, bool >;

	protected int m_lastSelPlayer = -1;
	protected int m_lastSelFunc = -1;
	protected int m_setBindMode = 0;
	protected bool m_mouseOnMap = false;
	protected bool m_mouseOnSearch = false;
	protected bool m_mapTPAllowed = true;
	protected bool m_spawnModeExpert = false;
	protected autoptr TIntArray m_lastFuncParams = {0,0,0};
	protected autoptr array<ref ZBerryFuncParamArray> m_funcParams = new array<ref ZBerryFuncParamArray>;
	protected autoptr ZBerryJsonSpawnMenuGroupArray m_spawnMenuGroups;

	protected string m_lastSelObj;
	protected string m_objFilter;
	protected string m_objTarget = "Cursor";

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
		m_MultiButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("BindButton") );
		m_FilterCatOneButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("ItemsButton") );
		m_FilterCatTwoButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("ObjectsButton") );
		m_FilterCatThreeButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("AIButton") );
		m_SpawnTargetButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("TargetButton") );
		m_ExecFuncButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("SecondaryExecButton") );
		m_PlayersRefresh = ButtonWidget.Cast( layoutRoot.FindAnyWidget("PlayersRefresh") );

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
		m_PlyHealth = TextWidget.Cast( layoutRoot.FindAnyWidget("PlayerHealth") );
		m_PlyBlood = TextWidget.Cast( layoutRoot.FindAnyWidget("PlayerBlood") );

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
		m_spawnMenuGroups = ZomberryBase.GetConfig().GetSpawnMenuData();

		m_FilterCatOneButton.SetText(m_spawnMenuGroups[0].CategoryName);
		m_FilterCatTwoButton.SetText(m_spawnMenuGroups[1].CategoryName);
		m_FilterCatThreeButton.SetText(m_spawnMenuGroups[2].CategoryName);

		m_TxtTitle.SetText( "Players in game: ... | ZomBerry Admin Tools v" + g_zbryVer + " by Vaker" );
		GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayersRequest", new Param1< int >( 0 ), true, NULL );
		GetRPCManager().SendRPC( "ZomBerryAT", "SyncFunctionsRequest", new Param1< int >( 0 ), true, NULL );

		if ( m_objTarget != "Ground" ) m_SpawnTargetButton.SetText( m_objTarget );
	}

	override void OnHide() {
		super.OnHide();
		
		m_MapDiff = false;
				
		GetGame().GetInput().ResetGameFocus();

		m_lastSelFunc = -1;
	}

	override bool OnMouseEnter( Widget w, int x, int y ) {
		if ( w == m_MapWidget ) {
			m_mouseOnMap = true;
			return true;
		}
		if ( w == m_SearchInput ) {
			m_mouseOnSearch = true;
			return true;
		}
		return false;
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
		if ( w == m_SearchInput ) {
			m_mouseOnSearch = false;
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
			Param3<int, bool, int> funcData;
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

	void CorrectMapPos(string MapPos, float MapScale) {
		m_MapWidget.SetMapPos(MapPos.ToVector());
		m_MapWidget.SetScale(MapScale);
		m_MapDiff = true;
	}
	
	override void Update(float tDelta) {
		if (m_MapPage.IsVisible()) {
			
			if(!m_MapDiff && m_MapPos && m_MapPos != m_MapWidget.GetMapPos()) {
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(CorrectMapPos, 100, false, m_MapPos.ToString(false), m_MapScale);
			} else {
				m_MapPos = m_MapWidget.GetMapPos();
				m_MapScale = m_MapWidget.GetScale();
			}
			
			if(m_mouseOnMap) {
			
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
	}

	override bool OnClick( Widget w, int x, int y, int button ) {
		string FilterOne = m_spawnMenuGroups[0].BaseClassNames;
		string FilterTwo = m_spawnMenuGroups[1].BaseClassNames;
		string FilterThree = m_spawnMenuGroups[2].BaseClassNames;
		Param3<int, bool, int> funcData;

		if ( w == m_FunctionsButton ) {
			m_FunctionsButton.SetTextColor(COLOR_GREEN);
			m_SpawnButton.SetTextColor(0xFFFFFFFF);
			m_MapButton.SetTextColor(0xFFFFFFFF);
			m_MultiButton.SetTextColor(0xFFFFFFFF);
			m_MultiButton.SetText( "Bind" );
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

			if (!m_spawnModeExpert) {
				m_MultiButton.SetTextColor(COLOR_GREEN);
				m_MultiButton.SetText( "Safe" );
			} else {
				m_MultiButton.SetTextColor(COLOR_RED);
				m_MultiButton.SetText( "Expert" );
			}
		}

		if ( w == m_MapButton ) {
			
			if(!m_MapPos) {
				m_MapPos = m_MapWidget.GetMapPos();
				m_MapScale = m_MapWidget.GetScale();
			} else {
				m_MapWidget.SetMapPos(m_MapPos);
				m_MapWidget.SetScale(m_MapScale);
			}
			
			m_FunctionsButton.SetTextColor(0xFFFFFFFF);
			m_SpawnButton.SetTextColor(0xFFFFFFFF);
			m_MapButton.SetTextColor(COLOR_GREEN);
			m_MultiButton.SetTextColor(0xFFFFFFFF);
			m_MultiButton.SetText( "WIP" );
			m_FunctionsPage.Show( false );
			m_SpawnPage.Show( false );
			m_MapPage.Show( true );
		}

		if ( w == m_FunctionsList ) {
			string fncName;
			int fParamId = -1;
			int fKeyBind;

			TIntArray defValues = {250, 500, 750};

			int selectedFuncRow = m_FunctionsList.GetSelectedRow();
			if ( selectedFuncRow == -1 ) return true;
			if ( selectedFuncRow == m_lastSelFunc ) return true;
			m_lastSelFunc = selectedFuncRow;

			m_FunctionsList.GetItemData( selectedFuncRow, 0, funcData );
			m_FunctionsList.GetItemText( selectedFuncRow, 0, fncName );

			if (funcData.param3 == -1) return true;
			fKeyBind = ZomberryBase.GetKeyBindsMgr().GetFuncKey(funcData.param1);

			if (fKeyBind) {
				string fButtonName = typename.EnumToString(KeyCode, fKeyBind);
				m_MultiButton.SetText("[" + fButtonName.Substring(3, fButtonName.Length() + -3) + "]");
			} else {
				m_MultiButton.SetText("Bind");
			}

			if (m_setBindMode == 1) m_MultiButton.SetText("[...]");

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

		if ( w == m_MultiButton ) {
			if (m_FunctionsPage.IsVisible()) {
				int selFunc = m_FunctionsList.GetSelectedRow();
				if (selFunc == -1) { Message("No function selected. [Dbl click] to delete bind, [A] + [Dbl click] to remove ALL binds."); return true; }

				m_FunctionsList.GetItemData( selFunc, 0, funcData );
				if (funcData.param3 == -1) return true;
				if (m_funcParams[funcData.param3].Count() > 0) { Message("This function requires parameters to be executed, so keybinding is disabled (will be available in the future tho)"); return true; }
				int oldKey = ZomberryBase.GetKeyBindsMgr().GetFuncKey(funcData.param1);

				if (m_setBindMode != -1 && (KeyState(KeyCode.KC_A) & 0x00000001)) {
					m_MultiButton.SetText("CLRALL?");
					m_setBindMode = -1;
				} else if (m_setBindMode == 0) {
					m_MultiButton.SetText("[...]");
					m_setBindMode = 1;
				} else if (m_setBindMode == -1) {
					m_MultiButton.SetText("Bind");
					if (KeyState(KeyCode.KC_A) & 0x00000001) ZomberryBase.GetKeyBindsMgr().SetFuncKey(-2, -1);
					m_setBindMode = 0;
				} else {
					m_MultiButton.SetText("Bind");
					ZomberryBase.GetKeyBindsMgr().SetFuncKey(-1, oldKey);
					m_setBindMode = 0;
				}
			}

			if (m_SpawnPage.IsVisible()) {
				if (m_spawnModeExpert) {
					m_MultiButton.SetTextColor(COLOR_GREEN);
					m_MultiButton.SetText( "Safe" );
					m_spawnModeExpert = false;
					UpdateObjList( m_objFilter );
				} else {
					m_MultiButton.SetTextColor(COLOR_RED);
					m_MultiButton.SetText( "Expert" );
					m_spawnModeExpert = true;
					UpdateObjList( m_objFilter );
				}
			}
		}

		if ( w == m_ExecFuncButton ) {
			OnDoubleClick( m_FunctionsList, 0, 0, 0 );
		}

		if ( w == m_PlayersRefresh ) {
			GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayersRequest", new Param1< int >( 0 ), true, NULL );
		}

		if ( w == m_FilterCatOneButton ) {
			if (m_objFilter == FilterOne) {
				m_FilterCatOneButton.SetTextColor(0xFFFFFFFF);
				m_objFilter = "All";
			} else {
				m_FilterCatOneButton.SetTextColor(COLOR_GREEN);
				m_objFilter = FilterOne;
			}

			m_FilterCatTwoButton.SetTextColor(0xFFFFFFFF);
			m_FilterCatThreeButton.SetTextColor(0xFFFFFFFF);
			UpdateObjList( m_objFilter );
		}

		if ( w == m_FilterCatTwoButton ) {
			if (m_objFilter == FilterTwo) {
				m_FilterCatTwoButton.SetTextColor(0xFFFFFFFF);
				m_objFilter = "All";
			} else {
				m_FilterCatTwoButton.SetTextColor(COLOR_GREEN);
				m_objFilter = FilterTwo;
			}

			m_FilterCatOneButton.SetTextColor(0xFFFFFFFF);
			m_FilterCatThreeButton.SetTextColor(0xFFFFFFFF);
			UpdateObjList( m_objFilter );
		}

		if ( w == m_FilterCatThreeButton ) {
			if (m_objFilter == FilterThree) {
				m_FilterCatThreeButton.SetTextColor(0xFFFFFFFF);
				m_objFilter = "All";
			} else {
				m_FilterCatThreeButton.SetTextColor(COLOR_GREEN);
				m_objFilter = FilterThree;
			}

			m_FilterCatOneButton.SetTextColor(0xFFFFFFFF);
			m_FilterCatTwoButton.SetTextColor(0xFFFFFFFF);
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
		Param4<int, bool, int, int> plyData;
		Man adminPly = GetGame().GetPlayer();
		int adminId;
		if (GetGame().IsMultiplayer()) {
			adminId = adminPly.GetIdentity().GetPlayerId();
		} else {
			adminId = 0;
		}
		
		if ( w == m_PlayersList ) {

			if ( FindPlyInList(m_lastSelPlayer) != -1 ) {
				m_PlayersList.GetItemData( FindPlyInList(m_lastSelPlayer), 0, plyData );

			    if ( plyData.param2 ) {
					m_PlayersList.SetItemColor( FindPlyInList(m_lastSelPlayer), 0, 0xFF47EB00 );
				} else {
					m_PlayersList.SetItemColor( FindPlyInList(m_lastSelPlayer), 0, 0xFFFFFFFF );
				}
			};

			int selPlayer = m_PlayersList.GetSelectedRow();
			if ( selPlayer == -1 ) return true;

			m_PlayersList.SetItemColor( selPlayer, 0, 0xFFFF751A );
			m_PlayersList.GetItemData( selPlayer, 0, plyData );

			m_lastSelPlayer = plyData.param1;
			m_PlyHealth.SetText("  Health: " + plyData.param3.ToString());
			m_PlyBlood.SetText("  Blood: " + plyData.param4.ToString());
		}

		if ( w == m_FunctionsList ) {
			Param2<int, bool> funcData;

			int targetId;
			int funcId;

			int selectedFuncRow = m_FunctionsList.GetSelectedRow();
			if ( selectedFuncRow == -1 ) return true;

			m_FunctionsList.GetItemData( selectedFuncRow, 0, funcData );
			funcId = funcData.param1;

			if (funcId == -1) return true;

			if ( funcData.param2 ) {
				if ( FindPlyInList(m_lastSelPlayer) == -1 ) { Message("No player selected"); return true; }

				m_PlayersList.GetItemData( FindPlyInList(m_lastSelPlayer), 0, plyData );
				targetId = plyData.param1;
			} else {
				if ( FindPlyInList(m_lastSelPlayer) == -1 ) {
					targetId = adminId;
				} else {
					m_PlayersList.GetItemData( FindPlyInList(m_lastSelPlayer), 0, plyData );
					targetId = plyData.param1;
				}
			}

			GetRPCManager().SendRPC( "ZomBerryAT", "ExecuteCommand", new Param5< int, int, int, vector, autoptr TIntArray >( funcId, adminId, targetId, GetCursorPos(), m_lastFuncParams ), true );
		}

		if ( w == m_ObjectsList ) {
			string currentObject;

			int selectedObjRow = m_ObjectsList.GetSelectedRow();
			if ( selectedObjRow == -1 ) return true;

			m_ObjectsList.GetItemText(selectedObjRow, 0, currentObject);

			Param2<string, string> currentObjectParam;
			m_ObjectsList.GetItemData(selectedObjRow, 0, currentObjectParam);
			string currentObjectData = currentObjectParam.param1;
			string currentObjectCName = currentObjectParam.param2;

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

				GetRPCManager().SendRPC( "ZomBerryAT", "SpawnObject", new Param4< string, int, vector, bool >( currentObjectCName, adminId, targetPlace, targetInventory ), true );
			}
			m_lastSelObj = "";
		}
		
		if ( w == m_MapWidget ) {
			m_MapWidget.SetScale(0.94);
		}
		
		return true;
	}

	bool GetCloseClearance() {
		return !m_mouseOnSearch;
	}

	void OnKeyPress( int key ) {
		UIManager UIMgr = GetGame().GetUIManager();
		Param3<int, bool, int> funcData;
		int fKeyBind;

		if (key == KeyCode.KC_ESCAPE) return;

		if (m_setBindMode == 1) {
			if ( m_lastSelFunc == -1 ) return;

			m_FunctionsList.GetItemData( m_lastSelFunc, 0, funcData );
			ZomberryBase.GetKeyBindsMgr().SetFuncKey(funcData.param1, key);

			fKeyBind = ZomberryBase.GetKeyBindsMgr().GetFuncKey(funcData.param1);

			string fButtonName = typename.EnumToString(KeyCode, key);
			m_MultiButton.SetText("[" + fButtonName.Substring(3, fButtonName.Length() + -3) + "]");

			m_setBindMode = 0;
		}
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
		Param3<int, bool, int> funcData;
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
		Param3<int, bool, int> funcData;
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

	private int FindPlyInList(int plyId) {
		Param4<int, bool, int, int> plyData;

		for (int idx = 0; idx < m_PlayersList.GetNumItems(); idx++) {
			m_PlayersList.GetItemData( idx, 0, plyData );

			if (plyData.param1 == plyId) return idx;
		}

		return -1;
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

			m_PlayersList.ClearItems();
			playerListC = playerListS.param1;
			sUpTime = Math.Round(playerListS.param2/1000);
		}

		if (layoutRoot.IsVisible()) {
			int playerId, playerHealth, playerBlood;
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
				playerHealth = playerListC.Get(i).m_PlayerHealth;
				playerBlood = playerListC.Get(i).m_PlayerBlood;

				m_PlayersList.AddItem(" " + i.ToString() + ": " + playerName, new Param4<int, bool, int, int>(playerId, playerAdmin, playerHealth, playerBlood), 0);

				if (playerAdmin) {
					m_PlayersList.SetItemColor( i, 0, 0xFF47EB00 );
					m_MapWidget.AddUserMark(playerPos, playerName, ARGB(255,71,235,0), "\\dz\\gear\\navigation\\data\\map_tree_ca.paa");
				} else {
					m_MapWidget.AddUserMark(playerPos, playerName, ARGB(255,85,170,255), "\\dz\\gear\\navigation\\data\\map_tree_ca.paa");
				}
			}

			if ( FindPlyInList(m_lastSelPlayer) != -1 && FindPlyInList(m_lastSelPlayer) < playerListC.Count() ) {
				Param4<int, bool, int, int> plyData;
				m_PlayersList.SetItemColor( FindPlyInList(m_lastSelPlayer), 0, 0xFFFF751A );
				m_PlayersList.GetItemData( FindPlyInList(m_lastSelPlayer), 0, plyData );

				m_PlyHealth.SetText("  Health: " + plyData.param3.ToString());
				m_PlyBlood.SetText("  Blood: " + plyData.param4.ToString());
			}
		}
	}

	void SyncFunctions( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param1<ref ZBerryCategoryArray> catListS;
		ref ZBerryCategoryArray catList;

		if ( type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer() ) {
			if ( !ctx.Read( catListS ) ) return;

			m_FunctionsList.ClearItems();
			catList = catListS.param1;
			m_funcParams.Clear();
			ZomberryBase.GetKeyBindsMgr().ClearFuncList();
		}

		int lastId = 0;
		int entryId;

		for (int i = 0; i < catList.Count(); ++i) {
			ref ZBerryCategory singleCat = catList.Get(i);

			m_FunctionsList.AddItem( "==== "+singleCat.GetName()+" ====", new Param3<int, bool, int>(-1, false, -1), 0, lastId );

			++lastId;

			ref ZBerryFunctionArray funcArray;
			funcArray = catList.Get(i).GetAll();

			for (int j = 0; j < funcArray.Count(); ++j) {
				ref ZBerryFunction funcData;
				funcData = funcArray.Get(j);
				entryId = funcData.GetId();

				int fParamId = m_funcParams.Insert(funcData.GetParams());

				ZomberryBase.GetKeyBindsMgr().AddFunc(entryId, funcData.GetName());

				m_FunctionsList.AddItem( funcData.GetDisplayName(), new Param3<int, bool, int>(entryId, funcData.IsTargetRequired(), fParamId), 0, lastId );
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
			m_ObjectsList.AddItem( " == "+splitFilters[i]+" == ", new Param2<string, string>("Category", ""), 0, i );

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

				string displayName;
				g_Game.ConfigGetText( strConfigPath + " " + strName + " displayName", displayName );
				int scope = g_Game.ConfigGetInt( strConfigPath + " " + strName + " scope" );

				if ( scope == 0 ) continue;

				string strNameLower = strName;
				string displayNameLower = displayName;

				strNameLower.ToLower();
				displayNameLower.ToLower();

				for (int sFilter = 0; sFilter < splitFilters.Count(); ++sFilter) {
					if (m_oCategoryHiddenStatus.Get(splitFilters[sFilter])) continue;

					if ( GetGame().IsKindOf( strNameLower, splitFilters[sFilter] ) || splitFilters[sFilter] == "All" ) {
						if ((strSearch != "" && !(strNameLower.Contains(strSearch) || displayNameLower.Contains(strSearch)))) continue;
						if (!m_spawnModeExpert && (displayName == "" || displayName.Contains("$UNT$"))) continue;

						int catPlace = m_oCategoryIndex.Get( splitFilters[sFilter] );
						m_ObjectsList.AddItem(strName + " [" + displayName + "]", new Param2<string, string>("Entry", strName), 0, catPlace+1);

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
