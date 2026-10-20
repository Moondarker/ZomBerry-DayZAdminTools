//Kick or ban confirmation over the admin menu: reason, ban duration, and the id for bans of offline players.
//Part of the menu's own layout (not a separate menu), so the menu keeps the input focus; while it's open the menu key and Esc don't close the menu
class ZBerryUIKickBanDialog extends ZBerryUIComponent {
	protected TextWidget m_Title, m_Target, m_DurationTitle;
	protected EditBoxWidget m_IdInput, m_ReasonInput;
	protected ButtonWidget m_DurationButton, m_OkButton, m_CancelButton;

	protected bool m_IsBan;
	protected int m_TargetSessionId = -1; //-1: offline ban by id
	protected int m_DurationIndex = 1;

	static TIntArray GetDurations() {
		return {3600, 86400, 604800, 2592000, 0};
	}

	static TStringArray GetDurationNames() {
		return {"1 hour", "1 day", "7 days", "30 days", "Permanent"};
	}

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/KickBanDialog.layout", parent)) return;

		m_Title = TextWidget.Cast( m_Root.FindAnyWidget("KickBanTitle") );
		m_Target = TextWidget.Cast( m_Root.FindAnyWidget("KickBanTarget") );
		m_DurationTitle = TextWidget.Cast( m_Root.FindAnyWidget("KickBanDurationTitle") );
		m_IdInput = EditBoxWidget.Cast( BindWidget("KickBanIdInput") );
		m_ReasonInput = EditBoxWidget.Cast( BindWidget("KickBanReasonInput") );
		m_DurationButton = ButtonWidget.Cast( BindWidget("KickBanDuration") );
		m_OkButton = ButtonWidget.Cast( BindWidget("KickBanOk") );
		m_CancelButton = ButtonWidget.Cast( BindWidget("KickBanCancel") );
		Show(false);
	}

	void OpenKick(int sessionId, string name) {
		m_IsBan = false;
		m_TargetSessionId = sessionId;
		Open("Kick player", "Player: " + name);
	}

	//sessionId -1: nobody selected, the admin enters a Steam64 id or GUID
	void OpenBan(int sessionId, string name) {
		m_IsBan = true;
		m_TargetSessionId = sessionId;
		m_DurationIndex = 1;
		UpdateDuration();

		if (sessionId < 0) {
			Open("Ban player", "Steam64 id or GUID of the player:");
		} else {
			Open("Ban player", "Player: " + name);
		}
	}

	protected void Open(string title, string target) {
		m_Title.SetText(title);
		m_Target.SetText(target);
		m_ReasonInput.SetText("");
		m_IdInput.SetText("");

		bool offline = m_IsBan && m_TargetSessionId < 0;
		m_IdInput.Show(offline);
		m_DurationTitle.Show(m_IsBan);
		m_DurationButton.Show(m_IsBan);

		Show(true);
		if (offline) {
			SetFocus(m_IdInput);
		} else {
			SetFocus(m_ReasonInput);
		}
	}

	void Close() {
		if (!IsVisible()) return;

		Show(false);
		SetFocus(NULL);
	}

	protected void UpdateDuration() {
		ZBerryUIUtils.SetButtonText(m_DurationButton, GetDurationNames()[m_DurationIndex]);
	}

	protected void Submit() {
		string reason = m_ReasonInput.GetText().Trim();

		if (!m_IsBan) {
			GetZBerryRPC().SendRPC( ZBerryRPCOp.Kick, new Param2<int, string>(m_TargetSessionId, reason), true, NULL );
		} else {
			string offlineId = "";
			if (m_TargetSessionId < 0) {
				offlineId = m_IdInput.GetText().Trim();
				if (offlineId == "") {
					SetFocus(m_IdInput);
					return;
				}
			}

			int duration = GetDurations()[m_DurationIndex];
			GetZBerryRPC().SendRPC( ZBerryRPCOp.Ban, new Param4<int, string, string, int>(m_TargetSessionId, offlineId, reason, duration), true, NULL );
		}

		Close();
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_OkButton) {
			Submit();
			return true;
		}
		if (w == m_CancelButton) {
			Close();
			return true;
		}
		if (w == m_DurationButton) {
			m_DurationIndex = (m_DurationIndex + 1) % GetDurations().Count();
			UpdateDuration();
			return true;
		}
		return false;
	}

	override bool OnChange(Widget w, int x, int y, bool finished) {
		if (w == m_ReasonInput && finished) {
			Submit();
			return true;
		}
		if (w == m_IdInput && finished) {
			SetFocus(m_ReasonInput);
			return true;
		}
		return false;
	}
};
