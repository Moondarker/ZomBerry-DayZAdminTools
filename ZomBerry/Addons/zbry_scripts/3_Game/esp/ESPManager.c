class ZomberryESPManager {
	protected ref ZBerryESPIconArray m_Icons;
	private ScriptCallQueue m_UpdQueue;
	protected int m_espRange;

	void ZomberryESPManager() {
		Print("ZomberryESPManager() called");
		m_Icons = new ZBerryESPIconArray();
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
		autoptr map<int, int> cliToRealIds = new map<int, int>;
		vector adminPos = "0 0 0";
		vector adminDir = "0 0 0";

		if (!playerListC) return;

		if (GetGame().GetPlayer()) {
			adminPos = GetGame().GetCurrentCameraPosition();
			adminDir = GetGame().GetCurrentCameraDirection().VectorToAngles();// + "180 0 0";
		}

		if (!m_Icons) return;
		SyncPlayerCount(playerListC.Count());

		if (GetGame().IsMultiplayer()) {
			foreach (int secId, Man ply: ClientData.m_PlayerBaseList) {
				if (ply && ply.GetIdentity()) {
					cliToRealIds.Insert(ply.GetIdentity().GetPlayerId(), secId);
				}
			}
		}

		foreach (int i, ZBerryESPIcon espIcon: m_Icons) {
			if (!espIcon) continue;

			int color = 0xFFFFFFFF;
			int plyId = playerListC[i].m_PlayerID;
			int plyHealth = playerListC[i].m_PlayerHealth;
			int plyHead;
			vector plyPos, plyDir;

			if (playerListC[i].m_IsAdmin) color = COLOR_GREEN;
			if (plyId == GetZomberryClient().GetSelectedId()) color = 0xFFFF751A;

			if (cliToRealIds.Find(plyId, plyId)) {
				plyHead = Human.Cast(ClientData.m_PlayerBaseList[plyId]).GetBoneIndexByName("Head");
				plyDir = ClientData.m_PlayerBaseList[plyId].GetOrientation();

				if (!ClientData.m_PlayerBaseList[plyId].IsAlive()) plyHealth = 0;
				if (plyHead == -1) plyPos = ClientData.m_PlayerBaseList[plyId].GetPosition()[1] + 1.6;
					else plyPos = Human.Cast(ClientData.m_PlayerBaseList[plyId]).GetBonePositionWS(plyHead);
			} else {
				plyPos = playerListC[i].m_PlayerPos;
				plyDir = playerListC[i].m_PlayerDir;
			}

			espIcon.Update(adminPos, playerListC[i].m_PlayerName, plyHealth, plyPos, plyDir - adminDir, color);
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
