//Client: shows why the server kicked or banned this player (received by ZBerryKickNoticeData, 3_Game) once the main menu
//is back: the engine's disconnect screen can't show a reason
class ZBerryKickNotice {
	static const int SHOW_DELAY = 1500; //ms after the main menu starts: after the engine's own disconnect message

	//From the main menu
	static void ShowLater() {
		if (!ZBerryKickNoticeData.IsPending()) return;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(Show, SHOW_DELAY, false);
	}

	protected static void Show() {
		bool banned;
		string reason;
		int until, remaining;
		if (!ZBerryKickNoticeData.Take(banned, reason, until, remaining)) return;

		string caption = "Kicked";
		string text = "An admin kicked you from the server. Reason: " + reason;
		if (banned) {
			caption = "Banned";
			text = "You are banned from this server permanently. Reason: " + reason;
		}
		if (banned && until > 0) {
			string untilText = ZBerryTime.FormatLocal(until, ZBerryTime.GetLocalOffset());
			text = "You are banned from this server for " + ZBerryTime.FormatDuration(remaining) + " more, until " + untilText + ". Reason: " + reason;
		}
		GetGame().GetUIManager().ShowDialog(caption, text, 0, DBT_OK, DBB_OK, DMT_INFO, NULL);
	}
};

modded class MissionMainMenu {
	override void OnMissionStart() {
		super.OnMissionStart();
		ZBerryKickNotice.ShowLater();
	}
};
