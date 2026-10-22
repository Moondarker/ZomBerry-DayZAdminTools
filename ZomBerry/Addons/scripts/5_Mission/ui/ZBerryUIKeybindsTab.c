//Keybinds, under the header's gear: every function this admin may bind, by category, with its key. Binding works as on
//the Functions tab: select a function, press the header's Bind button, then the key; pressing Bind again while it
//waits removes the key. Functions with parameters, or marked not keybindable, aren't listed
class ZBerryUIKeybindsTab extends ZBerryUITab {
	protected TextListboxWidget m_List; //The visible variant of m_Lists
	protected ref ZBerryScaledList m_Lists;
	protected ButtonWidget m_UnbindButton, m_UnbindAllButton;
	protected TextWidget m_Count;
	protected ref ZBerryCategoryArray m_Categories; //The last function list from the server
	protected int m_SelectedFuncId = -1;
	protected bool m_WaitingForKey;
	protected bool m_ConfirmUnbindAll; //Unbind all asks once more before clearing everything

	protected override void OnInit() {
		m_Lists = new ZBerryScaledList(m_Root, "KeybindsList", this);
		m_Lists.Pick();
		m_List = m_Lists.GetActive();
		m_UnbindButton = ButtonWidget.Cast( BindWidget("KeybindsUnbindButton") );
		m_UnbindAllButton = ButtonWidget.Cast( BindWidget("KeybindsUnbindAllButton") );
		m_Count = TextWidget.Cast( m_Root.FindAnyWidget("KeybindsCount") );
	}

	protected override void OnActivate() {
		SetActionButton("Bind", ZBerryUIUtils.TEXT_DEFAULT);
		Refill();
	}

	protected override void OnDeactivate() {
		m_WaitingForKey = false;
		SetConfirmUnbindAll(false);
	}

	override void OnMenuShow() {
		if (m_Lists.Pick()) m_List = m_Lists.GetActive();
		if (IsVisible()) Refill();
	}

	override void OnMenuHide() {
		m_WaitingForKey = false;
		SetConfirmUnbindAll(false);
	}

	//The server sent the function list (as for the Functions tab, which registers the functions for keybinding first)
	void SetFunctions(ZBerryCategoryArray catList) {
		m_Categories = catList;
		Refill();
	}

	protected static bool IsBindable(ZBerryFunction funcData) {
		return funcData.IsKeybindable() && funcData.GetParams().Count() == 0;
	}

	protected void Refill() {
		if (!m_List) return;
		m_List.ClearItems();

		int bound = 0;
		int row = 0;
		int selectedRow = -1;
		ZomberryKeybinds binds = ZomberryBase.GetKeyBindsMgr();
		if (m_Categories) {
			foreach (ZBerryCategory category : m_Categories) {
				ZBerryFunctionArray funcArray = category.GetAll();
				bool headerAdded = false;

				foreach (ZBerryFunction funcData : funcArray) {
					if (!IsBindable(funcData)) continue;
					if (!headerAdded) {
						m_List.AddItem( "==== " + category.GetName() + " ====", new Param1<int>(-1), 0, row );
						++row;
						headerAdded = true;
					}

					string text = funcData.GetDisplayName();
					int key = binds.GetKey(funcData.GetId());
					if (key != -1) {
						text = text + "   " + ZBerryUIUtils.FormatKeyName(key);
						++bound;
					}
					m_List.AddItem( text, new Param1<int>(funcData.GetId()), 0, row );
					m_List.SetItemColor(row, 0, funcData.GetColor());
					if (funcData.GetId() == m_SelectedFuncId) selectedRow = row;
					++row;
				}
			}
		}

		if (selectedRow != -1) {
			m_List.SelectRow(selectedRow);
		} else {
			m_SelectedFuncId = -1;
		}
		if (m_Count) m_Count.SetText(DescribeCount(bound));
		UpdateButtons();
	}

	protected static string DescribeCount(int bound) {
		if (bound == 0) return "No keys bound";
		if (bound == 1) return "1 function has a key";
		return bound.ToString() + " functions have keys";
	}

	//The header button shows the selected function's key, "[...]" while waiting for one
	protected void UpdateButtons() {
		if (!IsVisible()) return;

		int key = -1;
		if (m_SelectedFuncId != -1) key = ZomberryBase.GetKeyBindsMgr().GetKey(m_SelectedFuncId);
		if (m_WaitingForKey) {
			ZBerryUIUtils.SetButtonText(m_ActionButton, "[...]");
		} else if (key != -1) {
			ZBerryUIUtils.SetButtonText(m_ActionButton, ZBerryUIUtils.FormatKeyName(key));
		} else {
			ZBerryUIUtils.SetButtonText(m_ActionButton, "Bind");
		}
	}

	protected void SetConfirmUnbindAll(bool confirm) {
		m_ConfirmUnbindAll = confirm;
		if (!m_UnbindAllButton) return;

		if (confirm) {
			ZBerryUIUtils.SetButtonText(m_UnbindAllButton, "Sure?");
		} else {
			ZBerryUIUtils.SetButtonText(m_UnbindAllButton, "Unbind all");
		}
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_List) {
			SelectFunction();
			return true;
		}
		if (w == m_UnbindButton) {
			SetConfirmUnbindAll(false);
			if (m_SelectedFuncId == -1) {
				ZBerryUIUtils.Message("Select a function to unbind");
				return true;
			}
			m_WaitingForKey = false;
			ZomberryBase.GetKeyBindsMgr().Unbind(m_SelectedFuncId);
			Refill();
			return true;
		}
		if (w == m_UnbindAllButton) {
			m_WaitingForKey = false;
			if (!m_ConfirmUnbindAll) {
				SetConfirmUnbindAll(true);
				return true;
			}
			SetConfirmUnbindAll(false);
			ZomberryBase.GetKeyBindsMgr().ClearAll();
			Refill();
			return true;
		}
		return false;
	}

	protected void SelectFunction() {
		int row = m_List.GetSelectedRow();
		if (row == -1) return;

		Param1<int> rowData;
		m_List.GetItemData(row, 0, rowData);
		if (!rowData || rowData.param1 == -1) return; //A category header

		if (rowData.param1 != m_SelectedFuncId) m_WaitingForKey = false;
		m_SelectedFuncId = rowData.param1;
		SetConfirmUnbindAll(false);
		UpdateButtons();
	}

	override void OnActionButtonClick() {
		SetConfirmUnbindAll(false);
		if (m_SelectedFuncId == -1) {
			ZBerryUIUtils.Message("Select a function first, then press Bind and the key");
			return;
		}

		if (!m_WaitingForKey) {
			m_WaitingForKey = true;
			UpdateButtons();
			return;
		}

		//Pressed again while waiting for a key: remove the bind
		m_WaitingForKey = false;
		ZomberryBase.GetKeyBindsMgr().Unbind(m_SelectedFuncId);
		Refill();
	}

	override void OnBindKeyPress(int key) {
		if (!IsVisible() || !m_WaitingForKey) return;

		m_WaitingForKey = false;
		if (key != KeyCode.KC_ESCAPE) ZomberryBase.GetKeyBindsMgr().Bind(m_SelectedFuncId, key); //Escape: no change
		Refill();
	}
};
