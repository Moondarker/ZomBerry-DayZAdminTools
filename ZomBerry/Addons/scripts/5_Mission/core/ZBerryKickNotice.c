//Client: why the server kicked or banned this player. The engine's disconnect screen can't show a reason, so the
//server sends it just before (ZBerryBans.SendKickNotice) and the main menu shows it once it's back. Statics survive
//leaving the server: the scripts stay loaded
class ZBerryKickNotice {
	static const int SHOW_DELAY = 1500; //ms after the main menu starts: after the engine's own disconnect message

	protected static string s_Caption;
	protected static string s_Text;

	static void Remember(bool banned, string reason, int until, int remaining) {
		if (!banned) {
			s_Caption = "Kicked";
			s_Text = "An admin kicked you from the server. Reason: " + reason;
			return;
		}

		s_Caption = "Banned";
		if (until <= 0) {
			s_Text = "You are banned from this server permanently. Reason: " + reason;
			return;
		}
		string untilText = ZBerryTime.FormatLocal(until, ZBerryTime.GetLocalOffset());
		s_Text = "You are banned from this server for " + ZBerryTime.FormatDuration(remaining) + " more, until " + untilText + ". Reason: " + reason;
	}

	//From the main menu
	static void ShowLater() {
		if (s_Text == "") return;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(Show, SHOW_DELAY, false);
	}

	protected static void Show() {
		if (s_Text == "") return;

		GetGame().GetUIManager().ShowDialog(s_Caption, s_Text, 0, DBT_OK, DBB_OK, DMT_INFO, NULL);
		s_Text = "";
	}
};

modded class MissionMainMenu {
	override void OnMissionStart() {
		super.OnMissionStart();
		ZBerryKickNotice.ShowLater();
	}
};
