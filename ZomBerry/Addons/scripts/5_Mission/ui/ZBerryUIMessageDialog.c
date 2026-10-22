//Message dialog over the admin menu: a private message to the selected player, or an announcement to everyone.
//Like the kick/ban dialog it's part of the menu's own layout; while it's open the menu key and Esc don't close the menu.
//The text area looks like the notification players get. Typing goes into an invisible edit box over it, focused while
//the mouse is over the area: the engine still turns keys into characters (any keyboard layout, paste), and the
//area shows the whole message wrapped. The multi-line edit box was no option: it sends no change events and doesn't wrap.
//Edit boxes don't tell scripts where their caret is, so the dialog follows it: arrow keys, Home and End move it, and
//every change of the text puts it right after the changed part (which also corrects it after a missed key repeat).
//The box ignores the mouse, so a click can't move its caret somewhere the dialog can't know
class ZBerryUIMessageDialog extends ZBerryUIComponent {
	static const int COLOR_PLACEHOLDER = 0xFF808080;
	static const string CARET = "|"; //Drawn where the hidden box's caret is

	protected TextWidget m_Length;
	protected RichTextWidget m_Preview;
	protected EditBoxWidget m_Input; //Invisible, over the preview; ignores the mouse
	protected Widget m_InputArea; //The panel behind the text: pointing at it focuses the box
	protected bool m_Hovering;
	protected string m_Text = ""; //As of the last change
	protected int m_Caret; //Position in m_Text
	protected ButtonWidget m_RecipientButton, m_SendButton, m_CancelButton;

	protected ref TIntArray m_Recipients = new TIntArray; //Session ids the button steps through, -1 for everyone
	protected ref TStringArray m_RecipientNames = new TStringArray;
	protected int m_RecipientIndex;

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/MessageDialog.layout", parent)) return;

		m_Length = TextWidget.Cast( m_Root.FindAnyWidget("MessageLength") );
		m_Preview = RichTextWidget.Cast( m_Root.FindAnyWidget("MessagePreview") );
		m_Input = EditBoxWidget.Cast( BindWidget("MessageInput") );
		m_InputArea = BindWidget("MessagePreviewBack");
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
		m_Text = "";
		m_Caret = 0;
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

	//Where the caret is after the text changed from 'before' to 'after'. An edit at the known caret (typing, paste,
	//Backspace, Delete) is taken as that; anything else (a selection replaced) as right after the changed part
	static int CaretAfterEdit(string before, string after, int caret) {
		int beforeLength = before.Length();
		int afterLength = after.Length();
		int changed = afterLength - beforeLength;
		caret = Math.Clamp(caret, 0, beforeLength);

		//Each piece in its own local: nested Substring temporaries can overwrite each other
		string head = before.Substring(0, caret);
		string tail = before.Substring(caret, beforeLength - caret);
		if (changed > 0) {
			string afterHead = after.Substring(0, caret);
			string afterTail = after.Substring(afterLength - tail.Length(), tail.Length());
			if (afterHead == head && afterTail == tail) return caret + changed; //Typed or pasted at the caret
		}
		if (changed < 0 && caret + changed >= 0) {
			string kept = before.Substring(0, caret + changed);
			string joined = kept + tail;
			if (joined == after) return caret + changed; //Backspace
		}
		if (changed < 0 && caret - changed <= beforeLength) {
			string rest = before.Substring(caret - changed, beforeLength - caret + changed);
			string joinedRest = head + rest;
			if (joinedRest == after) return caret; //Delete
		}

		int prefix = 0;
		while (prefix < beforeLength && prefix < afterLength && before.Get(prefix) == after.Get(prefix)) ++prefix;
		int suffix = 0;
		while (suffix < beforeLength - prefix && suffix < afterLength - prefix && before.Get(beforeLength - 1 - suffix) == after.Get(afterLength - 1 - suffix)) ++suffix;
		return afterLength - suffix;
	}

	//Arrow keys, Home and End, with Ctrl for whole words, as the box itself moves its caret
	static int CaretAfterKey(string text, int caret, int key, bool ctrl) {
		int length = text.Length();
		if (key == KeyCode.KC_HOME) return 0;
		if (key == KeyCode.KC_END) return length;
		if (key == KeyCode.KC_LEFT) {
			if (!ctrl) return Math.Max(0, caret - 1);
			while (caret > 0 && text.Get(caret - 1) == " ") --caret;
			while (caret > 0 && text.Get(caret - 1) != " ") --caret;
			return caret;
		}
		if (key == KeyCode.KC_RIGHT) {
			if (!ctrl) return Math.Min(length, caret + 1);
			while (caret < length && text.Get(caret) != " ") ++caret;
			while (caret < length && text.Get(caret) == " ") ++caret;
			return caret;
		}
		return caret;
	}

	//Raw key presses, forwarded by the menu while it's open
	void OnKeyPressed(int key) {
		if (!IsVisible() || !m_Hovering) return;

		bool ctrl = KeyState(KeyCode.KC_LCONTROL) != 0 || KeyState(KeyCode.KC_RCONTROL) != 0;
		int caret = CaretAfterKey(m_Text, m_Caret, key, ctrl);
		if (caret == m_Caret) return;

		m_Caret = caret;
		UpdateLength();
	}

	//Counter and preview: the box is one line, the preview shows the whole message wrapped, as players see it.
	//Text past the limit is cut while typing
	protected void UpdateLength() {
		string text = m_Input.GetText();
		if (text != m_Text) {
			m_Caret = CaretAfterEdit(m_Text, text, m_Caret);
			m_Text = text;
		}
		if (text.Length() > ZBerryMessages.MAX_LENGTH) {
			text = text.Substring(0, ZBerryMessages.MAX_LENGTH);
			m_Input.SetText(text);
			m_Text = text;
			m_Caret = text.Length(); //Setting the text puts the box's caret at the end
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
			int caret = Math.Clamp(m_Caret, 0, length);
			string before = text.Substring(0, caret);
			string after = text.Substring(caret, length - caret);
			string shown = before + CARET;
			shown += after;
			m_Preview.SetText(shown);
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
		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y) {
		if (w != m_InputArea) return false;

		m_Hovering = true;
		SetFocus(m_Input);
		UpdateLength();
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y) {
		if (w != m_InputArea) return false;

		m_Hovering = false;
		SetFocus(NULL);
		UpdateLength();
		return true;
	}
};
