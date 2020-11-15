typedef array<ref ZBerryPlayer> ZBerryPlayerArray;

class ZBerryPlayer {
	int PlayerID;
	string PlayerName;
	bool IsAdmin;
	vector PlayerPos;
	vector PlayerDir;
	int PlayerHealth;
	int PlayerBlood;

	void ZBerryPlayer(int plyId, string plyName, bool isAdmin, vector plyPos, int plyHP, int plyBld, vector plyDir = "0 0 0") {
		PlayerID = plyId;
		PlayerName = plyName;
		IsAdmin = isAdmin;
		PlayerPos = plyPos;
		PlayerHealth = plyHP;
		PlayerBlood = plyBld;
		PlayerDir = plyDir;
	}
}
