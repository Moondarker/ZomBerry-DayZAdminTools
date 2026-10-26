//Right-click menu: a title and up to MAX_ACTIONS buttons at the mouse. Part of the menu's own layout, above everything
//else; a click anywhere outside closes it. The owner fills it (Clear, Add), opens it and listens to OnAction
class ZBerryUIActionsMenu extends ZBerryUIComponent {
	static const int MAX_ACTIONS = 4;
	static const float BOX_WIDTH = 220; //Layout units, as in ActionsMenu.layout
	static const float TITLE_HEIGHT = 36;
	static const float ROW_HEIGHT = 36;
	static const float PADDING = 4;

	ref ScriptInvoker OnAction = new ScriptInvoker; //(int action)

	protected Widget m_Backdrop, m_Box;
	protected TextWidget m_Title;
	protected ref array<ButtonWidget> m_Buttons = new array<ButtonWidget>;
	protected ref TIntArray m_Actions = new TIntArray;

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/ActionsMenu.layout", parent)) return;

		m_Backdrop = BindWidget("ActionsBackdrop");
		m_Box = m_Root.FindAnyWidget("ActionsBox");
		m_Title = TextWidget.Cast( m_Root.FindAnyWidget("ActionsTitle") );
		for (int i = 1; i <= MAX_ACTIONS; ++i) {
			ButtonWidget btn = ButtonWidget.Cast( BindWidget("Action" + i) );
			if (btn) m_Buttons.Insert(btn);
		}
		Show(false);
	}

	void Clear() {
		m_Actions.Clear();
	}

	void Add(string label, int action) {
		if (m_Actions.Count() >= m_Buttons.Count()) return;

		ZBerryUIUtils.SetButtonText(m_Buttons[m_Actions.Count()], label);
		m_Actions.Insert(action);
	}

	int Count() {
		return m_Actions.Count();
	}

	//x, y: the mouse, in screen pixels. Kept inside the menu (the frame covers the admin menu, not the whole screen)
	void Open(int x, int y, string title) {
		if (!m_Root || m_Actions.Count() == 0) return;

		m_Title.SetText(title);
		for (int i = 0; i < m_Buttons.Count(); ++i) m_Buttons[i].Show(i < m_Actions.Count());

		float scale = ZBerryUIUtils.GetUIScale(); //SetPos and SetSize take pixels, the layout's units are scaled
		float width = BOX_WIDTH * scale;
		float height = (TITLE_HEIGHT + m_Actions.Count() * ROW_HEIGHT + PADDING) * scale;
		m_Box.SetSize(width, height);

		Show(true); //Measured while shown
		float frameX, frameY, frameWidth, frameHeight; //SetPos is relative to the frame
		m_Root.GetScreenPos(frameX, frameY);
		m_Root.GetScreenSize(frameWidth, frameHeight);
		float left = Math.Min(x - frameX, frameWidth - width);
		float top = Math.Min(y - frameY, frameHeight - height);
		m_Box.SetPos(Math.Max(left, 0), Math.Max(top, 0));
	}

	void Close() {
		Show(false);
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		ButtonWidget btn = ButtonWidget.Cast(w);
		int index = -1;
		if (btn) index = m_Buttons.Find(btn);
		if (index < 0 || index >= m_Actions.Count()) return false;

		int action = m_Actions[index];
		Close();
		OnAction.Invoke(action);
		return true;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button) {
		if (w != m_Backdrop) return false;

		Close();
		return true;
	}
};
