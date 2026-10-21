//Message dialog over the admin menu: a private message to the selected player, or an announcement to everyone.
//Like the kick/ban dialog it's part of the menu's own layout; while it's open the menu key and Esc don't close the menu
class ZBerryUIMessageDialog extends ZBerryUIComponent {
	protected TextWidget m_Length;
	protected EditBoxWidget m_Input;
	protected ButtonWidget m_RecipientButton, m_SendButton, m_CancelButton;

	protected ref TIntArray m_Recipients = new TIntArray; //Session ids the button steps through, -1 for everyone
	protected ref TStringArray m_RecipientNames = new TStringArray;
	protected int m_RecipientIndex;

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/MessageDialog.layout", parent)) return;

		m_Length = TextWidget.Cast( m_Root.FindAnyWidget("MessageLength") );
		m_Input = EditBoxWidget.Cast( BindWidget("MessageInput") );
		m_RecipientButton = ButtonWidget.Cast( BindWidget("MessageRecipient") );
		m_SendButton = ButtonWidget.Cast( BindWidget("MessageSend") );
		m_CancelButton = ButtonWidget.Cast( BindWidget("MessageCancel") );
		Show(false);
	}

	//Whether the admin may send anything at all: the players panel shows its button only then
	static bool CanSend() {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		return auth.Has(ZBerryPermissions.MESSAGES_PRIVATE) || auth.Has(ZBerryPermissions.MESSAGES_ANNOUNCE);
	}

	//sessionId -1: nobody selected. Starts on the selected player when there is one and private messages are allowed
	void Open(int sessionId, string name) {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		m_Recipients.Clear();
		m_RecipientNames.Clear();
		if (sessionId >= 0 && auth.Has(ZBerryPermissions.MESSAGES_PRIVATE)) {
			m_Recipients.Insert(sessionId);
			m_RecipientNames.Insert(name + " (private)");
		}
		if (auth.Has(ZBerryPermissions.MESSAGES_ANNOUNCE)) {
			m_Recipients.Insert(-1);
			m_RecipientNames.Insert("Everyone (announcement)");
		}

		if (m_Recipients.Count() == 0) {
			ZBerryUIUtils.Message("Double-click a player in the list first");
			return;
		}

		m_RecipientIndex = 0;
		UpdateRecipient();
		m_Input.SetText("");
		UpdateLength();

		Show(true);
		SetFocus(m_Input);
	}

	void Close() {
		if (!IsVisible()) return;

		Show(false);
		SetFocus(NULL);
	}

	protected void UpdateRecipient() {
		string caption = m_RecipientNames[m_RecipientIndex];
		if (m_Recipients.Count() > 1) caption += "  >";
		ZBerryUIUtils.SetButtonText(m_RecipientButton, caption);
	}

	protected void UpdateLength() {
		int length = m_Input.GetText().Length();
		m_Length.SetText(length.ToString() + " / " + ZBerryMessages.MAX_LENGTH);
		if (length > ZBerryMessages.MAX_LENGTH) {
			m_Length.SetColor(ZBerryUILogsTab.COLOR_REFUSED);
		} else {
			m_Length.SetColor(ZBerryUIUtils.TEXT_DEFAULT);
		}
	}

	protected void Submit() {
		string text = m_Input.GetText().Trim();
		if (text == "") {
			SetFocus(m_Input);
			return;
		}
		if (text.Length() > ZBerryMessages.MAX_LENGTH) {
			ZBerryUIUtils.Message("The message is too long: " + text.Length() + " of " + ZBerryMessages.MAX_LENGTH + " characters");
			SetFocus(m_Input);
			return;
		}

		GetZBerryRPC().SendRPC( ZBerryRPCOp.Message, new Param2<int, string>(m_Recipients[m_RecipientIndex], text), true, NULL );
		Close();
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_SendButton) {
			Submit();
			return true;
		}
		if (w == m_CancelButton) {
			Close();
			return true;
		}
		if (w == m_RecipientButton) {
			m_RecipientIndex = (m_RecipientIndex + 1) % m_Recipients.Count();
			UpdateRecipient();
			return true;
		}
		return false;
	}

	override bool OnChange(Widget w, int x, int y, bool finished) {
		if (w != m_Input) return false;

		UpdateLength();
		if (finished) Submit();
		return true;
	}
};
