//Function list, function parameters, execution and keybinding
class ZBerryUIFunctionsTab extends ZBerryUITab {
	static const int MAX_PARAMS = 3; //Server-side functions always receive this many values

	protected TextListboxWidget m_FunctionsList; //The visible variant of m_FunctionsLists
	protected ref ZBerryScaledList m_FunctionsLists;
	protected TextWidget m_FncName;
	protected ButtonWidget m_ExecButton;
	protected ref array<ref ZBerryUIFuncParamRow> m_ParamRows = new array<ref ZBerryUIFuncParamRow>;
	protected ref array<ref ZBerryFuncParamArray> m_FuncParams = new array<ref ZBerryFuncParamArray>;
	protected ref TIntArray m_Unbindable = new TIntArray; //Ids of functions that can't get a key
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
		ZomberryBase.GetKeyBindsMgr().ClearFuncList();

		int row = 0;

		for (int i = 0; i < catList.Count(); ++i) {
			ZBerryCategory singleCat = catList.Get(i);
			m_FunctionsList.AddItem( "==== " + singleCat.GetName() + " ====", new Param3<int, bool, int>(-1, false, -1), 0, row );
			++row;

			ZBerryFunctionArray funcArray = singleCat.GetAll();

			for (int j = 0; j < funcArray.Count(); ++j) {
				ZBerryFunction funcData = funcArray.Get(j);
				ZBerryFuncParamArray funcParams = new ZBerryFuncParamArray;
				funcParams.Clone(funcData.GetParams());

				int fParamId = m_FuncParams.Insert(funcParams);
				ZomberryBase.GetKeyBindsMgr().AddFunc(funcData.GetId(), funcData.GetName(), funcData.IsKeybindable());
				if (!funcData.IsKeybindable()) m_Unbindable.Insert(funcData.GetId());

				m_FunctionsList.AddItem( funcData.GetDisplayName(), new Param3<int, bool, int>(funcData.GetId(), funcData.IsTargetRequired(), fParamId), 0, row );
				m_FunctionsList.SetItemColor(row, 0, funcData.GetColor());
				++row;
			}
		}

		if (keptFuncId != -1) Reselect(keptFuncId, keptValues);
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

	//Nothing selected: no name, no parameter rows, the bind button back to "Bind"
	protected void ClearSelection() {
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
		if (m_FuncParams[funcData.param3].Count() > 0) { ZBerryUIUtils.Message("This function requires parameters to be executed, so keybinding is disabled (will be available in the future tho)"); return; }
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
		} else { //Pressed again while waiting for a key: remove the bind
			ZBerryUIUtils.SetButtonText(m_ActionButton, "Bind");
			ZomberryBase.GetKeyBindsMgr().Unbind(funcData.param1);
			m_BindMode = BIND_IDLE;
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
	}

	protected void SelectFunction() {
		Param3<int, bool, int> funcData;
		string fncName;

		int selectedFuncRow = m_FunctionsList.GetSelectedRow();
		if ( selectedFuncRow == -1 ) return;
		if ( selectedFuncRow == m_LastSelFunc ) return;
		m_LastSelFunc = selectedFuncRow;

		m_FunctionsList.GetItemData( selectedFuncRow, 0, funcData );
		m_FunctionsList.GetItemText( selectedFuncRow, 0, fncName );
		if (funcData.param3 == -1) return;

		m_SelectedFuncId = funcData.param1;
		UpdateBindButtonText(funcData.param1);
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

	protected void UpdateBindButtonText(int funcId) {
		int fKeyBind = ZomberryBase.GetKeyBindsMgr().GetKey(funcId);

		if (m_BindMode == BIND_WAITING_FOR_KEY) {
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

		if (hasTarget) {
			targetId = selectedPlyId;
		} else {
			if ( funcData.param2 ) { ZBerryUIUtils.Message("No player selected"); return; }
			targetId = adminId;
		}

		GetZBerryRPC().SendRPC( ZBerryRPCOp.ExecuteCommand, new Param4< int, int, vector, autoptr TIntArray >( funcId, targetId, ZBerryUIUtils.GetCursorPos(), GetParamValues() ), true );
	}
};
