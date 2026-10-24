//"Are you sure?" over the admin menu: a title, what will happen, OK and Cancel. Part of the menu's own layout like the
//other dialogs; Esc cancels. Whoever opens it inserts into OnConfirm right after Open, which only fires for OK
class ZBerryUIConfirmDialog extends ZBerryUIComponent {
	ref ScriptInvoker OnConfirm = new ScriptInvoker;

	protected TextWidget m_Title;
	protected MultilineTextWidget m_Text;
	protected ButtonWidget m_OkButton, m_CancelButton;

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/ConfirmDialog.layout", parent)) return;

		m_Title = TextWidget.Cast( m_Root.FindAnyWidget("ConfirmTitle") );
		m_Text = MultilineTextWidget.Cast( m_Root.FindAnyWidget("ConfirmText") );
		m_OkButton = ButtonWidget.Cast( BindWidget("ConfirmOk") );
		m_CancelButton = ButtonWidget.Cast( BindWidget("ConfirmCancel") );
		Show(false);
	}

	//Forgets whoever listened before
	void Open(string title, string text, string okLabel) {
		if (!m_Root) return;

		m_Title.SetText(title);
		m_Text.SetText(text);
		ZBerryUIUtils.SetButtonText(m_OkButton, okLabel);
		OnConfirm.Clear();
		Show(true);
	}

	void Close() {
		Show(false);
		OnConfirm.Clear();
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_CancelButton) {
			Close();
			return true;
		}
		if (w != m_OkButton) return false;

		Show(false);
		OnConfirm.Invoke();
		OnConfirm.Clear();
		return true;
	}
};
