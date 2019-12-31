class ZomberryClient {
	protected ref ZBerryPlayerArray playerListC;
	protected ref ZBerrySubscriptionArray subbedInstances;
	protected ref ZomberryESPManager m_ZomberryESPManager;
	protected bool espEnabled;
	protected int sUpTime, subCount, selectedId, cliSrvTDelta;

	void ZomberryClient() {
		playerListC = new ZBerryPlayerArray();
		subbedInstances = new ZBerrySubscriptionArray();
		espEnabled = false;
		sUpTime = 0;
		subCount = 0;
		selectedId = -1;
		cliSrvTDelta = 0;

		GetRPCManager().AddRPC("ZomBerryAT", "SyncPlayers", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("ZomBerryAT", "ToggleESPState", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("ZomBerryAT", "FreezePlayer", this, SingeplayerExecutionType.Client);
	}

	void ~ZomberryClient() {
		if (playerListC) delete playerListC;
		if (subbedInstances) delete subbedInstances;
		if (m_ZomberryESPManager) delete m_ZomberryESPManager;
	}

	void ReportActive(bool isActive) {
		GetRPCManager().SendRPC( "ZomBerryAT", "SetActiveAdmin", new Param2<bool, bool> (isActive, (playerListC.Count() == 0)), true, NULL );
	}

	void SyncPlayers( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param2<ref ZBerryPlayerArray, int> playerListS;

		if (type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer()) {
			if (!ctx.Read(playerListS) || !playerListC || !subbedInstances) return;

			playerListC.Clear();
			playerListC = playerListS.param1;

			if (GetGame().IsMultiplayer())
				cliSrvTDelta = playerListS.param2 - GetGame().GetTime();

			SendUpdToSubs();
		}
	}

	private void SendUpdToSubs(int subId = -1) {
		if (subbedInstances.Count() > 0) {
			foreach(ZBerrySubscription sub: subbedInstances)
				if (sub && (subId == -1 || subId == sub.m_SubId))
					GetGame().GameScript.CallFunctionParams(sub.m_Instance, sub.m_FncName, NULL, new Param2<ref ZBerryPlayerArray, int>(playerListC, cliSrvTDelta));
		}
	}

	void ToggleESPState( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param1<int> espParams;

		if (type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer()) {
			if (!ctx.Read( espParams )) return;

			SetESPState(!espEnabled, espParams.param1);
		}
	}

	void FreezePlayer( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param1<bool> isFrozen;

		if (type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer()) {
			if (!ctx.Read( isFrozen )) return;

			GetGame().GetPlayer().GetInputController().SetDisabled(isFrozen.param1);
		}
	}

	private void SetESPState(bool espEn, int espRng) {
		espEnabled = espEn;
		GetZomberryESPManager().HUDLoop(espEn, espRng);

		if (espEn)
			Message("Toggled ESP ON, range is " + espRng.ToString() + "m");
		else
			Message("Toggled ESP OFF");
	}

	int Subscribe(Class instance, string fncName) {
		subCount++;
		subbedInstances.Insert(new ZBerrySubscription(subCount, instance, fncName));
		SendUpdToSubs(subCount);
		return subCount;
	}

	void UnSubscribe(int subId) {
		for (int i = 0; i < subbedInstances.Count(); ++i)
			if (subbedInstances[i].m_SubId == subId)
				subbedInstances.Remove(i);
	}

	ref ZBerryPlayerArray GetPlayerList() {
		//Message(playerListC[0].m_PlayerPos.ToString());
		//Message(GetGame().GetScreenPos(playerListC[0].m_PlayerPos + "0 1.85 0").ToString());
		return playerListC;
	}

	bool IsESPEnabled() {
		return espEnabled;
	}

	int GetCliSrvTDelta() {
		return cliSrvTDelta;
	}

	void SetSelectedId(int sId) {
		selectedId = sId;
	}

	int GetSelectedId() {
		return selectedId;
	}

	ref ZomberryESPManager GetZomberryESPManager() {
	    if (!m_ZomberryESPManager) {
	        m_ZomberryESPManager = new ref ZomberryESPManager;
	    }

	    return m_ZomberryESPManager;
	}

	static void Message( string txt ) {
		GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "", "[ZomBerry] " + txt, ""));
	}
}

static ref ZomberryClient g_ZomberryClient;
static ref ZomberryClient GetZomberryClient() {
    if (!g_ZomberryClient) {
        g_ZomberryClient = new ref ZomberryClient;
    }

    return g_ZomberryClient;
}
