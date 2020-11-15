typedef array<ref ZBerryBanInfo> ZBerryBanInfoArray;

class ZBerryBanInfo {
	string Author;
	string SteamID;
	string Reason;
	int EndTimestamp;  // -1 = indefinite

	void ZBerryBanInfo(string admin, string steamID, string reason, int timestamp) {
		Author = admin;
		SteamID = steamID;
		Reason = reason;
		EndTimestamp = timestamp;
	}
}
