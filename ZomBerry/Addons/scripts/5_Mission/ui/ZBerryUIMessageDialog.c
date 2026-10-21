//Message dialog over the admin menu: a private message to the selected player, or an announcement to everyone.
//Like the kick/ban dialog it's part of the menu's own layout; while it's open the menu key and Esc don't close the menu.
//The text area looks like the notification players get. Typing goes into an invisible edit box over it, focused while
//the mouse is over the area: the engine still turns keys into characters (any keyboard layout, paste), and the
//area shows the whole message wrapped. The multi-line edit box was no option: it sends no change events and doesn't wrap
class ZBerryUIMessageDialog extends ZBerryUIComponent {
	static const int COLOR_PLACEHOLDER = 0xFF808080;
	static const string CARET = "|"; //At the end: the hidden box's own caret can't be seen, so arrow keys are blocked

	protected TextWidget m_Length;
	protected RichTextWidget m_Preview;
	protected EditBoxWidget m_Input; //Invisible, over the preview
	protected bool m_Hovering;
	protected ButtonWidget m_RecipientButton, m_SendButton, m_CancelButton;

	protected ref TIntArray m_Recipients = new TIntArray; //Session ids the button steps through, -1 for everyone
	protected ref TStringArray m_RecipientNames = new TStringArray;
	protected int m_RecipientIndex;

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/MessageDialog.layout", parent)) return;

		m_Length = TextWidget.Cast( m_Root.FindAnyWidget("MessageLength") );
		m_Preview = RichTextWidget.Cast( m_Root.FindAnyWidget("MessagePreview") );
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
		m_Hovering = false;
		m_Input.SetText("");
		UpdateLength();
		Show(true);
	}

	void Close() {
		if (!IsVisible()) return;

		m_Hovering = false;
		Show(false);
		SetFocus(NULL);
	}

	protected void UpdateRecipient() {
		string caption = m_RecipientNames[m_RecipientIndex];
		if (m_Recipients.Count() > 1) caption += "  >";
		ZBerryUIUtils.SetButtonText(m_RecipientButton, caption);
	}

	//Counter and preview: the box is one line, the preview shows the whole message wrapped, as players see it.
	//Text past the limit is cut while typing
	protected void UpdateLength() {
		string text = m_Input.GetText();
		if (text.Length() > ZBerryMessages.MAX_LENGTH) {
			text = text.Substring(0, ZBerryMessages.MAX_LENGTH);
			m_Input.SetText(text);
		}

		int length = text.Length();
		m_Length.SetText(length.ToString() + " / " + ZBerryMessages.MAX_LENGTH);
		if (length >= ZBerryMessages.MAX_LENGTH) {
			m_Length.SetColor(ZBerryUILogsTab.COLOR_REFUSED);
		} else {
			m_Length.SetColor(ZBerryUIUtils.TEXT_DEFAULT);
		}

		if (length == 0 && !m_Hovering) {
			m_Preview.SetText("Point here and type your message");
			m_Preview.SetColor(COLOR_PLACEHOLDER);
		} else if (m_Hovering) {
			m_Preview.SetText(text + CARET);
			m_Preview.SetColor(ZBerryUIUtils.TEXT_DEFAULT);
		} else {
			m_Preview.SetText(text);
			m_Preview.SetColor(ZBerryUIUtils.TEXT_DEFAULT);
		}
	}

	protected void Submit() {
		if (!IsVisible()) return; //Enter can arrive both as a key and as the box's "finished"

		string text = m_Input.GetText().Trim();
		if (text == "") return;
		if (text.Length() > ZBerryMessages.MAX_LENGTH) {
			ZBerryUIUtils.Message("The message is too long: " + text.Length() + " of " + ZBerryMessages.MAX_LENGTH + " characters");
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

	//"finished" also comes when the box loses focus: only Enter while pointing at the area sends
	override bool OnChange(Widget w, int x, int y, bool finished) {
		if (w != m_Input) return false;

		UpdateLength();
		if (finished && m_Hovering) Submit();
		return true;
	}

	override bool OnKeyDown(Widget w, int x, int y, int key) {
		if (w != m_Input) return false;

		if (key == KeyCode.KC_RETURN || key == KeyCode.KC_NUMPADENTER) {
			Submit();
			return true;
		}
		//The caret is drawn at the end, so it can't be moved elsewhere
		return key == KeyCode.KC_LEFT || key == KeyCode.KC_RIGHT || key == KeyCode.KC_HOME || key == KeyCode.KC_END || key == KeyCode.KC_UP || key == KeyCode.KC_DOWN;
	}

	override bool OnMouseEnter(Widget w, int x, int y) {
		if (w != m_Input) return false;

		m_Hovering = true;
		SetFocus(m_Input);
		UpdateLength();
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y) {
		if (w != m_Input) return false;

		m_Hovering = false;
		SetFocus(NULL);
		UpdateLength();
		return true;
	}
};
