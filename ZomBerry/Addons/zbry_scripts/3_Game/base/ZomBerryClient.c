class ZomberryClient {
	autoptr protected ref ZBerryPlayerArray playerListC = new ZBerryPlayerArray;
	autoptr protected ref ZBerrySubscriptionArray subbedInstances = new ZBerrySubscriptionArray;
	autoptr protected ref ZomberryESPManager m_ZomberryESPManager;
	protected int sUpTime = 0;
	protected int subCount = 0;

	void ZomberryClient() {

		GetRPCManager().AddRPC("ZomBerryAT", "SyncPlayers", this, SingeplayerExecutionType.Client);
		GetZomberryESPManager().HUDLoop(true);
	}

	void ~ZomberryClient() {

		if (m_ZomberryESPManager) delete m_ZomberryESPManager;
	}

	void SyncPlayers( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param2<ref ZBerryPlayerArray, int> playerListS;

		if ( type == CallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer() ) {
			if ( !ctx.Read( playerListS ) ) {
				/*Message("Player sync data read error - possible version mismatch");*/
				return;
			}

			playerListC.Clear();
			playerListC = playerListS.param1;
			sUpTime = Math.Round(playerListS.param2/1000);
		}

		if (subbedInstances.Count() > 0) {
			foreach(ZBerrySubscription sub: subbedInstances)
				GetGame().GameScript.CallFunctionParams(sub.m_Instance, sub.m_FncName, NULL, new Param2<ref ZBerryPlayerArray, int>(playerListC, sUpTime));
		}
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
		return playerListC;
	}

	int GetServerUpTime() {
		return sUpTime;
	}

	ref ZomberryESPManager GetZomberryESPManager() {
	    if (!m_ZomberryESPManager) {
	        m_ZomberryESPManager = new ref ZomberryESPManager;
	    }

	    return m_ZomberryESPManager;
	}
}

static ref ZomberryClient g_ZomberryClient;
static ref ZomberryClient GetZomberryClient() {
    if (!g_ZomberryClient) {
        g_ZomberryClient = new ref ZomberryClient;
    }

    return g_ZomberryClient;
}
