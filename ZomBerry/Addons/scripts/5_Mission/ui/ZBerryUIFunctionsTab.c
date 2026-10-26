//Function list, function parameters, execution and keybinding. Each function's key is shown after its name; the header
//button binds the selected function, and has no caption when it can't get a key (parameters, or marked unbindable)
class ZBerryUIFunctionsTab extends ZBerryUITab {
	static const int MAX_PARAMS = 3; //Server-side functions always receive this many values

	protected TextListboxWidget m_FunctionsList; //The visible variant of m_FunctionsLists
	protected ref ZBerryScaledList m_FunctionsLists;
	protected TextWidget m_FncName;
	protected ButtonWidget m_ExecButton;
	protected ref array<ref ZBerryUIFuncParamRow> m_ParamRows = new array<ref ZBerryUIFuncParamRow>;
	protected ref array<ref ZBerryFuncParamArray> m_FuncParams = new array<ref ZBerryFuncParamArray>;
	protected ref TIntArray m_Unbindable = new TIntArray; //Ids of functions that can't get a key
	protected ref TIntArray m_MultiTarget = new TIntArray; //Ids of functions that run on every selected player
	protected ref TStringArray m_RowNames = new TStringArray; //Display name per list row, without the key
	protected ref TIntArray m_RowColors = new TIntArray;
	protected ZBerryUIPlayersPanel m_PlayersPanel;

	protected int m_LastSelFunc = -1;
	protected int m_SelectedFuncId = -1; //Survives the list being rebuilt (every time the menu opens)
	protected int m_ActiveParamCount = 0;
	static const int BIND_IDLE = 0;
	static const int BIND_WAITING_FOR_KEY = 1;
	static const int BIND_CONFIRM_CLEAR_ALL = 2; //Asked with A held, confirmed by pressing the button again with A held

	protected int m_BindMode = BIND_IDLE;

	void SetPlayersPanel(ZBerryUIPlayersPanel playersPanel) {
		m_PlayersPanel = playersPanel;
	}

	protected override void OnInit() {
		m_FunctionsLists = new ZBerryScaledList(m_Root, "FunctionsList", this);
		m_FunctionsLists.Pick();
		m_FunctionsList = m_FunctionsLists.GetActive();
		m_ExecButton = ButtonWidget.Cast( BindWidget("SecondaryExecButton") );
		m_FncName = TextWidget.Cast( m_Root.FindAnyWidget("FuncName") );
		m_FncName.SetText("");

		for (int i = 0; i < MAX_PARAMS; ++i) {
			Widget slot = m_Root.FindAnyWidget("FuncParamSlot" + i.ToString());
			if (!slot) break;

			ZBerryUIFuncParamRow row = new ZBerryUIFuncParamRow();
			row.Init(slot);
			row.Show(false);
			m_ParamRows.Insert(row);
		}
	}

	protected override void OnActivate() {
		SetActionButton("Bind", ZBerryUIUtils.TEXT_DEFAULT);
		if (m_SelectedFuncId != -1) UpdateBindButtonText(m_SelectedFuncId);
	}

	//A bind waiting for its key is dropped: the key would go to whatever tab is open then
	protected override void OnDeactivate() {
		m_BindMode = BIND_IDLE;
	}

	override void OnMenuShow() {
		if (m_FunctionsLists.Pick()) m_FunctionsList = m_FunctionsLists.GetActive(); //Filled by the list requested below
		GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncFunctionsRequest, new Param1< int >( 0 ), true, NULL );
	}

	override void OnMenuHide() {
		m_LastSelFunc = -1;
	}

	//Rebuilding drops the list's selection and the parameter definitions the rows point at, so the selected function
	//is selected again afterwards, with its values (or the bottom area is cleared if it's gone)
	void SetFunctions(ZBerryCategoryArray catList) {
		int keptFuncId = m_SelectedFuncId;
		TIntArray keptValues = GetParamValues();
		ClearSelection();

		m_FunctionsList.ClearItems();
		m_FuncParams.Clear();
		m_Unbindable.Clear();
		m_MultiTarget.Clear();
		m_RowNames.Clear();
		m_RowColors.Clear();
		ZomberryBase.GetKeyBindsMgr().ClearFuncList();

		int row = 0;

		for (int i = 0; i < catList.Count(); ++i) {
			ZBerryCategory singleCat = catList.Get(i);
			m_FunctionsList.AddItem( "==== " + singleCat.GetName() + " ====", new Param3<int, bool, int>(-1, false, -1), 0, row );
			m_RowNames.Insert("");
			m_RowColors.Insert(ZBerryUIUtils.TEXT_DEFAULT);
			++row;

			ZBerryFunctionArray funcArray = singleCat.GetAll();

			for (int j = 0; j < funcArray.Count(); ++j) {
				ZBerryFunction funcData = funcArray.Get(j);
				ZBerryFuncParamArray funcParams = new ZBerryFuncParamArray;
				funcParams.Clone(funcData.GetParams());

				int fParamId = m_FuncParams.Insert(funcParams);
				ZomberryBase.GetKeyBindsMgr().AddFunc(funcData.GetId(), funcData.GetName(), funcData.IsKeybindable());
				if (!funcData.IsKeybindable() || funcParams.Count() > 0) m_Unbindable.Insert(funcData.GetId());
				if (funcData.IsMultiTarget()) m_MultiTarget.Insert(funcData.GetId());

				m_RowNames.Insert(funcData.GetDisplayName());
				m_RowColors.Insert(funcData.GetColor());
				m_FunctionsList.AddItem( GetRowText(row, funcData.GetId()), new Param3<int, bool, int>(funcData.GetId(), funcData.IsTargetRequired(), fParamId), 0, row );
				m_FunctionsList.SetItemColor(row, 0, funcData.GetColor());
				++row;
			}
		}

		if (keptFuncId != -1) Reselect(keptFuncId, keptValues);
	}

	//"Heal   [F5]": the name, and the key when it has one
	protected string GetRowText(int row, int funcId) {
		string text = m_RowNames[row];
		int key = ZomberryBase.GetKeyBindsMgr().GetKey(funcId);
		if (key != -1) text = text + "   " + ZBerryUIUtils.FormatKeyName(key);
		return text;
	}

	//A key was bound or removed: the rows say so again, updated in place so the list keeps its scroll
	protected void RefreshRowKeys() {
		Param3<int, bool, int> funcData;
		for (int row = 0; row < m_FunctionsList.GetNumItems() && row < m_RowNames.Count(); ++row) {
			m_FunctionsList.GetItemData(row, 0, funcData);
			if (!funcData || funcData.param1 == -1) continue;

			m_FunctionsList.SetItem(row, GetRowText(row, funcData.param1), funcData, 0);
			m_FunctionsList.SetItemColor(row, 0, m_RowColors[row]);
		}
	}

	protected void Reselect(int funcId, TIntArray values) {
		Param3<int, bool, int> funcData;

		for (int row = 0; row < m_FunctionsList.GetNumItems(); ++row) {
			m_FunctionsList.GetItemData(row, 0, funcData);
			if (!funcData || funcData.param1 != funcId) continue;

			m_FunctionsList.SelectRow(row);
			SelectFunction();
			if (GetCurrentValues(funcId)) return; //The list is rebuilt whenever the menu opens: show the time now
			for (int i = 0; i < m_ActiveParamCount; ++i) m_ParamRows[i].SetValue(values[i]);
			return;
		}
	}

	//Nothing selected: no name, no parameter rows, the bind button back to "Bind", every selected player highlighted
	protected void ClearSelection() {
		if (m_PlayersPanel) m_PlayersPanel.SetSingleFocus(false);
		m_LastSelFunc = -1;
		m_SelectedFuncId = -1;
		m_FncName.SetText("");
		ZBerryFuncParamArray noParams = new ZBerryFuncParamArray; //Local: a temporary argument may be freed early
		ShowParams(noParams);
		if (IsVisible() && m_BindMode == BIND_IDLE) ZBerryUIUtils.SetButtonText(m_ActionButton, "Bind");
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_FunctionsList) {
			SelectFunction();
			return true;
		}
		if (w == m_ExecButton) {
			ExecuteSelected();
			return true;
		}
		return false;
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button) {
		if (w == m_FunctionsList) {
			ExecuteSelected();
			return true;
		}
		return false;
	}

	override void OnActionButtonClick() {
		Param3<int, bool, int> funcData;

		int selFunc = m_FunctionsList.GetSelectedRow();
		if (selFunc == -1) { ZBerryUIUtils.Message("No function selected. [Dbl click] to delete bind, [A] + [Dbl click] to remove ALL binds."); return; }

		m_FunctionsList.GetItemData( selFunc, 0, funcData );
		if (funcData.param3 == -1) return;
		if (m_FuncParams[funcData.param3].Count() > 0) { ZBerryUIUtils.Message("This function takes parameters, so it can't be bound to a key. Run it from the menu."); return; }
		if (m_Unbindable.Find(funcData.param1) != -1) { ZBerryUIUtils.Message("This function can't be bound to a key, so it never runs by accident. Run it from the menu."); return; }
		bool clearAllHeld = (KeyState(KeyCode.KC_A) & 0x00000001) != 0;

		if (m_BindMode != BIND_CONFIRM_CLEAR_ALL && clearAllHeld) {
			ZBerryUIUtils.SetButtonText(m_ActionButton, "CLRALL?");
			m_BindMode = BIND_CONFIRM_CLEAR_ALL;
		} else if (m_BindMode == BIND_IDLE) {
			ZBerryUIUtils.SetButtonText(m_ActionButton, "[...]");
			m_BindMode = BIND_WAITING_FOR_KEY;
		} else if (m_BindMode == BIND_CONFIRM_CLEAR_ALL) {
			ZBerryUIUtils.SetButtonText(m_ActionButton, "Bind");
			if (clearAllHeld) ZomberryBase.GetKeyBindsMgr().ClearAll();
			m_BindMode = BIND_IDLE;
			RefreshRowKeys();
		} else { //Pressed again while waiting for a key: remove the bind
			ZBerryUIUtils.SetButtonText(m_ActionButton, "Bind");
			ZomberryBase.GetKeyBindsMgr().Unbind(funcData.param1);
			m_BindMode = BIND_IDLE;
			RefreshRowKeys();
		}
	}

	override void OnBindKeyPress(int key) {
		if (key == KeyCode.KC_ESCAPE) return;
		if (m_BindMode != BIND_WAITING_FOR_KEY || m_LastSelFunc == -1) return;

		Param3<int, bool, int> funcData;
		m_FunctionsList.GetItemData( m_LastSelFunc, 0, funcData );
		ZomberryBase.GetKeyBindsMgr().Bind(funcData.param1, key);

		ZBerryUIUtils.SetButtonText(m_ActionButton, ZBerryUIUtils.FormatKeyName(key));
		m_BindMode = BIND_IDLE;
		RefreshRowKeys();
	}

	protected void SelectFunction() {
		Param3<int, bool, int> funcData;
		string fncName;

		int selectedFuncRow = m_FunctionsList.GetSelectedRow();
		if ( selectedFuncRow == -1 ) return;
		if ( selectedFuncRow == m_LastSelFunc ) return;
		m_LastSelFunc = selectedFuncRow;

		m_FunctionsList.GetItemData( selectedFuncRow, 0, funcData );
		if (selectedFuncRow < m_RowNames.Count()) fncName = m_RowNames[selectedFuncRow]; //Without the key
		if (funcData.param3 == -1) return;

		m_SelectedFuncId = funcData.param1;
		UpdateBindButtonText(funcData.param1);
		if (m_PlayersPanel) m_PlayersPanel.SetSingleFocus(m_MultiTarget.Find(funcData.param1) == -1); //Others dimmed: it acts on the last selected only
		m_FncName.SetText(fncName);
		ShowParams(m_FuncParams[funcData.param3]);

		TIntArray currentValues = GetCurrentValues(funcData.param1);
		if (!currentValues) return;
		for (int i = 0; i < m_ActiveParamCount && i < currentValues.Count(); ++i) m_ParamRows[i].SetValue(currentValues[i]);
	}

	//Functions whose parameters start from the game's current state instead of fixed defaults, NULL for the others.
	//This client's world clock and weather follow the server's
	protected TIntArray GetCurrentValues(int funcId) {
		string funcName = ZomberryBase.GetKeyBindsMgr().GetFuncName(funcId);
		if (funcName == "SetWeather") return GetCurrentWeather();
		if (funcName != "SetTime" && funcName != "SetDate") return NULL;

		int year, month, day, hour, minute;
		GetGame().GetWorld().GetDate(year, month, day, hour, minute);
		TIntArray values = {hour, minute};
		if (funcName == "SetDate") values = {day, month, year};
		return values;
	}

	//Fog, overcast and rain in percent, as Set weather takes them. Typed float locals: mixed method results and
	//literals don't do plain maths in Enforce
	protected static TIntArray GetCurrentWeather() {
		Weather weather = GetGame().GetWeather();
		float fog = weather.GetFog().GetActual() * 100.0;
		float overcast = weather.GetOvercast().GetActual() * 100.0;
		float rain = weather.GetRain().GetActual() * 100.0;
		TIntArray values = {Math.Round(fog), Math.Round(overcast), Math.Round(rain)};
		return values;
	}

	//The header button is shared by all tabs: only while this one is open (the list is rebuilt whenever the menu opens,
	//whichever tab is showing)
	protected void UpdateBindButtonText(int funcId) {
		if (!IsVisible()) return;

		int fKeyBind = ZomberryBase.GetKeyBindsMgr().GetKey(funcId);

		if (m_Unbindable.Find(funcId) != -1) {
			ZBerryUIUtils.SetButtonText(m_ActionButton, "");
		} else if (m_BindMode == BIND_WAITING_FOR_KEY) {
			ZBerryUIUtils.SetButtonText(m_ActionButton, "[...]");
		} else if (fKeyBind != -1) {
			ZBerryUIUtils.SetButtonText(m_ActionButton, ZBerryUIUtils.FormatKeyName(fKeyBind));
		} else {
			ZBerryUIUtils.SetButtonText(m_ActionButton, "Bind");
		}
	}

	protected void ShowParams(ZBerryFuncParamArray funcParams) {
		m_ActiveParamCount = Math.Min(funcParams.Count(), m_ParamRows.Count());

		for (int i = 0; i < m_ParamRows.Count(); ++i) {
			if (i < m_ActiveParamCount) {
				m_ParamRows[i].Setup(funcParams.Get(i));
				m_ParamRows[i].Show(true);
			} else {
				m_ParamRows[i].Show(false);
			}
		}
	}

	protected TIntArray GetParamValues() {
		TIntArray values = new TIntArray;

		for (int i = 0; i < MAX_PARAMS; ++i) {
			if (i < m_ActiveParamCount) {
				values.Insert(m_ParamRows[i].GetValue());
			} else {
				values.Insert(0);
			}
		}

		return values;
	}

	protected void ExecuteSelected() {
		Param3<int, bool, int> funcData;
		int targetId;
		int selectedPlyId;

		int selectedFuncRow = m_FunctionsList.GetSelectedRow();
		if ( selectedFuncRow == -1 ) return;

		m_FunctionsList.GetItemData( selectedFuncRow, 0, funcData );
		int funcId = funcData.param1;
		if (funcId == -1) return;

		int adminId = ZBerryUIUtils.GetLocalPlayerId();
		bool hasTarget = m_PlayersPanel && m_PlayersPanel.GetSelectedPlayerId(selectedPlyId);

		//Several players selected: the server runs functions made for that on each of them, others on the last one
		TIntArray selectedIds;
		if (m_PlayersPanel) selectedIds = m_PlayersPanel.GetSelectedPlayerIds();
		if (hasTarget && selectedIds && selectedIds.Count() > 1 && m_MultiTarget.Find(funcId) != -1) {
			TIntArray values = GetParamValues(); //Locals: temporaries inside the Param would be freed before it's sent
			GetZBerryRPC().SendRPC( ZBerryRPCOp.ExecuteCommandTargets, new Param4< int, autoptr TIntArray, vector, autoptr TIntArray >( funcId, selectedIds, ZBerryUIUtils.GetCursorPos(), values ), true );
			return;
		}

		if (hasTarget) {
			targetId = selectedPlyId;
		} else {
			if ( funcData.param2 ) { ZBerryUIUtils.Message("No player selected"); return; }
			targetId = adminId;
		}

		GetZBerryRPC().SendRPC( ZBerryRPCOp.ExecuteCommand, new Param4< int, int, vector, autoptr TIntArray >( funcId, targetId, ZBerryUIUtils.GetCursorPos(), GetParamValues() ), true );
	}
};
