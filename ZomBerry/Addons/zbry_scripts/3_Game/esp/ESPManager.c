class ZomberryESPManager {
	protected ref ZBerryESPIconArray m_Icons;
	private ScriptCallQueue m_UpdQueue;
	protected int m_espRange;

	void ZomberryESPManager() {
		Print("ZomberryESPManager() called");
		m_Icons = new ZBerryESPIconArray;
		m_UpdQueue = GetGame().GetCallQueue(CALL_CATEGORY_GUI);
	}

	void ~ZomberryESPManager() {
		Print("~ZomberryESPManager() called");
		HUDLoop(false);
		delete m_Icons;
	}

	void HUDLoop(bool start, int maxDistance = 1700) {
		if (!start) {
			m_UpdQueue.Clear();
			SyncPlayerCount(0);
			m_Icons.Clear();
		} else {
			m_UpdQueue.Clear();
			m_espRange = maxDistance;
			m_UpdQueue.CallLater(Update, 1, true);
		}
	}

	protected void Update() {
		autoptr ZBerryPlayerArray playerListC = GetZomberryClient().GetPlayerList();
		vector adminPos = "0 0 0";
		vector adminDir = "0 0 0";

		if (!playerListC) return;

		if (GetGame().GetPlayer()) {
			adminPos = GetGame().GetCurrentCameraPosition();
			adminDir = GetGame().GetCurrentCameraDirection().VectorToAngles();// + "180 0 0";
		}

		if (!m_Icons) return;
		SyncPlayerCount(playerListC.Count());

		foreach (int i, ZBerryESPIcon espIcon: m_Icons) {
			if (!espIcon) continue;

			int color = 0xFFFFFFFF;
			if (playerListC[i].m_IsAdmin) color = COLOR_GREEN;
			if (playerListC[i].m_PlayerID == GetZomberryClient().GetSelectedId()) color = 0xFFFF751A;
			espIcon.Update(adminPos, playerListC[i].m_PlayerName, playerListC[i].m_PlayerHealth, playerListC[i].m_PlayerPos, playerListC[i].m_PlayerDir - adminDir, color);
		}
	}

	void SyncPlayerCount(int plyInRangeCount) {
		while (m_Icons.Count() < plyInRangeCount)
			m_Icons.Insert(new ZBerryESPIcon(m_espRange));

		while (m_Icons.Count() > plyInRangeCount) {
			delete m_Icons.Get(plyInRangeCount);
			m_Icons.Remove(plyInRangeCount);
		}
	}
}
