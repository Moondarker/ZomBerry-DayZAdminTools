//First startup: no admins exist yet, the player enters the token the server printed into its log
class ZBerryTokenDialog extends UIScriptedMenu {
	protected EditBoxWidget m_TokenInput;
	protected ButtonWidget m_OkButton, m_CancelButton;

	override Widget Init() {
		if (layoutRoot) return layoutRoot;

		layoutRoot = GetGame().GetWorkspace().CreateWidgets( "ZomBerry/AT/scripts/5_Mission/layouts/TokenDialog.layout" );
		if (!layoutRoot) {
			ZomberryBase.Log( "ZomBerryUI", "ERROR: Can't load TokenDialog.layout" );
			return NULL;
		}

		m_TokenInput = EditBoxWidget.Cast( layoutRoot.FindAnyWidget("TokenInput") );
		m_OkButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("TokenOk") );
		m_CancelButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("TokenCancel") );

		return layoutRoot;
	}

	override void OnShow() {
		super.OnShow();

		m_TokenInput.SetText("");
		SetFocus(m_TokenInput);
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_OkButton) {
			Submit();
			return true;
		}
		if (w == m_CancelButton) {
			CloseDialog();
			return true;
		}
		return false;
	}

	override bool OnChange(Widget w, int x, int y, bool finished) {
		if (w == m_TokenInput && finished) {
			Submit();
			return true;
		}
		return false;
	}

	protected void Submit() {
		string token = m_TokenInput.GetText().Trim();
		if (token == "") return;

		GetZBerryClientAuth().SubmitToken(token);
		CloseDialog();
	}

	protected void CloseDialog() {
		MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
		if (mission) mission.CloseTokenDialog();
	}
};
