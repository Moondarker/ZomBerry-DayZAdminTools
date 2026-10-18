//Function list, function parameters, execution and keybinding
class ZBerryUIFunctionsTab extends ZBerryUITab {
	static const int MAX_PARAMS = 3; //Server-side functions always receive this many values

	protected TextListboxWidget m_FunctionsList;
	protected TextWidget m_FncName;
	protected ButtonWidget m_ExecButton;
	protected ref array<ref ZBerryUIFuncParamRow> m_ParamRows = new array<ref ZBerryUIFuncParamRow>;
	protected ref array<ref ZBerryFuncParamArray> m_FuncParams = new array<ref ZBerryFuncParamArray>;
	protected ZBerryUIPlayersPanel m_PlayersPanel;

	protected int m_LastSelFunc = -1;
	protected int m_ActiveParamCount = 0;
	protected int m_BindMode = 0; //0 - idle, 1 - waiting for a key, -1 - waiting for "clear all" confirmation

	void SetPlayersPanel(ZBerryUIPlayersPanel playersPanel) {
		m_PlayersPanel = playersPanel;
	}

	protected override void OnInit() {
		m_FunctionsList = TextListboxWidget.Cast( BindWidget("FunctionsList") );
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
		GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncFunctionsRequest, new Param1< int >( 0 ), true, NULL );
	}

	override void OnMenuHide() {
		m_LastSelFunc = -1;
	}

	void SetFunctions(ZBerryCategoryArray catList) {
		m_FunctionsList.ClearItems();
		m_FuncParams.Clear();
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
				ZomberryBase.GetKeyBindsMgr().AddFunc(funcData.GetId(), funcData.GetName());

				m_FunctionsList.AddItem( funcData.GetDisplayName(), new Param3<int, bool, int>(funcData.GetId(), funcData.IsTargetRequired(), fParamId), 0, row );
				m_FunctionsList.SetItemColor(row, 0, funcData.GetColor());
				++row;
			}
		}
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
		int oldKey = ZomberryBase.GetKeyBindsMgr().GetFuncKey(funcData.param1);

		if (m_BindMode != -1 && (KeyState(KeyCode.KC_A) & 0x00000001)) {
			m_ActionButton.SetText("CLRALL?");
			m_BindMode = -1;
		} else if (m_BindMode == 0) {
			m_ActionButton.SetText("[...]");
			m_BindMode = 1;
		} else if (m_BindMode == -1) {
			m_ActionButton.SetText("Bind");
			if (KeyState(KeyCode.KC_A) & 0x00000001) ZomberryBase.GetKeyBindsMgr().SetFuncKey(-2, -1);
			m_BindMode = 0;
		} else {
			m_ActionButton.SetText("Bind");
			ZomberryBase.GetKeyBindsMgr().SetFuncKey(-1, oldKey);
			m_BindMode = 0;
		}
	}

	override void OnBindKeyPress(int key) {
		if (key == KeyCode.KC_ESCAPE) return;
		if (m_BindMode != 1 || m_LastSelFunc == -1) return;

		Param3<int, bool, int> funcData;
		m_FunctionsList.GetItemData( m_LastSelFunc, 0, funcData );
		ZomberryBase.GetKeyBindsMgr().SetFuncKey(funcData.param1, key);

		m_ActionButton.SetText(ZBerryUIUtils.FormatKeyName(key));
		m_BindMode = 0;
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

		UpdateBindButtonText(funcData.param1);
		m_FncName.SetText(fncName);
		ShowParams(m_FuncParams[funcData.param3]);
	}

	protected void UpdateBindButtonText(int funcId) {
		int fKeyBind = ZomberryBase.GetKeyBindsMgr().GetFuncKey(funcId);

		if (m_BindMode == 1) {
			m_ActionButton.SetText("[...]");
		} else if (fKeyBind) {
			m_ActionButton.SetText(ZBerryUIUtils.FormatKeyName(fKeyBind));
		} else {
			m_ActionButton.SetText("Bind");
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

		GetZBerryRPC().SendRPC( ZBerryRPCOp.ExecuteCommand, new Param5< int, int, int, vector, autoptr TIntArray >( funcId, adminId, targetId, ZBerryUIUtils.GetCursorPos(), GetParamValues() ), true );
	}
};
