class ZomberryClient {
	autoptr protected ref ZBerryPlayerArray playerListC = new ZBerryPlayerArray;
	autoptr protected ref ZBerrySubscriptionArray subbedInstances = new ZBerrySubscriptionArray;
	autoptr protected ref ZomberryESPManager m_ZomberryESPManager;
	protected bool espEnabled = false;
	protected int sUpTime = 0;
	protected int subCount = 0;
	protected int selectedId = -1;

	void ZomberryClient() {

		GetRPCManager().AddRPC("ZomBerryAT", "SyncPlayers", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("ZomBerryAT", "ToggleESPState", this, SingeplayerExecutionType.Client);
	}

	void ~ZomberryClient() {

		if (m_ZomberryESPManager) delete m_ZomberryESPManager;
	}

	void SyncPlayers( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param2<ref ZBerryPlayerArray, int> playerListS;

		if (type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer()) {
			if (!ctx.Read( playerListS )) return;

			playerListC.Clear();
			playerListC = playerListS.param1;
			sUpTime = Math.Round(playerListS.param2/1000);
		}

		if (subbedInstances.Count() > 0) {
			foreach(ZBerrySubscription sub: subbedInstances)
				if (sub)
					GetGame().GameScript.CallFunctionParams(sub.m_Instance, sub.m_FncName, NULL, new Param2<ref ZBerryPlayerArray, int>(playerListC, sUpTime));
		}
	}


	void ToggleESPState( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param1<int> espParams;

		if (type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer()) {
			if (!ctx.Read( espParams )) return;

			SetESPState(!espEnabled, espParams.param1);
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

	int GetServerUpTime() {
		return sUpTime;
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
