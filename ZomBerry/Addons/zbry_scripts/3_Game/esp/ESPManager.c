class ZomberryESPManager {
	protected ref ZBerryESPIconArray m_Icons;
	private ScriptCallQueue m_UpdQueue;

	void ZomberryESPManager() {
		m_Icons = new ZBerryESPIconArray;
		m_UpdQueue = GetGame().GetCallQueue(CALL_CATEGORY_GUI);
	}

	void ~ZomberryESPManager() {
		HUDLoop(false);
	}

	void HUDLoop(bool start) {
		if (!start) {
			m_UpdQueue.Clear();
			SyncPlayerCount(0);
			m_Icons.Clear();
		} else
			m_UpdQueue.CallLater(Update, 1, true);
	}

	protected void Update() {
		autoptr ZBerryPlayerArray playerListC = GetZomberryClient().GetPlayerList();
		vector adminPos = "0 0 0";
		vector adminDir = "0 0 0";

		if (!playerListC) return;

		if (GetGame().GetPlayer()) {
			adminPos = GetGame().GetPlayer().GetPosition();
			adminDir = GetGame().GetPlayer().GetOrientation() + "180 0 0";
		}

		SyncPlayerCount(playerListC.Count());

		foreach (int i, ZBerryESPIcon espIcon: m_Icons)
			espIcon.Update(adminPos, playerListC[i].m_PlayerName, playerListC[i].m_PlayerHealth, playerListC[i].m_PlayerPos, playerListC[i].m_PlayerDir - adminDir, 0x00FF00);
	}

	void SyncPlayerCount(int plyInRangeCount) {
		while (m_Icons.Count() < plyInRangeCount)
			m_Icons.Insert(new ZBerryESPIcon());

		while (m_Icons.Count() > plyInRangeCount) {
			delete m_Icons.Get(plyInRangeCount);
			m_Icons.Remove(plyInRangeCount);
		}
	}
}
