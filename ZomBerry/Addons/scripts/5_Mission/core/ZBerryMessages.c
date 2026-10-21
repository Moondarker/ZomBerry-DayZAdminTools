//Admin messages to players: a vanilla notification that's hard to miss, plus the same text in chat, where it can
//still be read after the notification faded. Server side (and singleplayer)
class ZBerryMessages {
	static const int MAX_LENGTH = 300;
	static const float SHOW_TIME = 15; //Seconds the notification stays
	//Must exist in the imageset, or the notification keeps its warning triangle. Square like the notification's 54x54 slot,
	//and 170 px: the HUD's 34x40 walkie_talkie came out squished and blurry
	static const string ICON = "set:dayz_inventory image:walkietalkie";
	static const string TITLE_ANNOUNCE = "Server announcement";
	static const string TITLE_PRIVATE = "Message from an admin";

	static string Clean(string text) {
		text = text.Trim();
		if (text.Length() > MAX_LENGTH) text = text.Substring(0, MAX_LENGTH);
		return text;
	}

	//Free text inside an event text: braces would be read as {actor}/{target} or as a name tag
	static string ForLog(string text) {
		text.Replace("{", "(");
		text.Replace("}", ")");
		return text;
	}

	//Everyone online; returns how many players got it
	static int Announce(string text) {
		if (!GetGame().IsMultiplayer()) {
			ShowLocally(TITLE_ANNOUNCE, text);
			return 1;
		}

		NotificationSystem.SendNotificationToPlayerIdentityExtended(NULL, SHOW_TIME, TITLE_ANNOUNCE, text, ICON); //NULL: everyone

		int count = 0;
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man man : players) {
			if (!man.GetIdentity()) continue;

			SendToChat(man, "Announcement: " + text);
			++count;
		}
		return count;
	}

	static void SendPrivate(PlayerBase player, string text) {
		if (!GetGame().IsMultiplayer()) {
			ShowLocally(TITLE_PRIVATE, text);
			return;
		}
		if (!player || !player.GetIdentity()) return;

		NotificationSystem.SendNotificationToPlayerExtended(player, SHOW_TIME, TITLE_PRIVATE, text, ICON);
		SendToChat(player, "Admin: " + text);
	}

	protected static void SendToChat(Man player, string line) {
		Param1<string> param = new Param1<string>(line);
		GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, param, true, player.GetIdentity());
	}

	//Singleplayer: server and client are the same game
	protected static void ShowLocally(string title, string text) {
		NotificationSystem.AddNotificationExtended(SHOW_TIME, title, text, ICON);
		GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "[ZomBerry]", title + ": " + text, ""));
	}
};
