//Server: bodies of admins in FreeCam (spectating keeps the body, see ZBerrySpectate). Their client controls only the
//camera then (SelectPlayer NULL), so the network bubble follows the camera (UpdateSpectatorPosition) instead of staying
//around the frozen body.
//A detached body may have no identity: ZBGetIdentity and ZBGetPlayerById find it here
class ZBerryBodies {
	protected static ref map<int, PlayerBase> s_Bodies; //Session id -> body; weak, NULL once the body is deleted

	//After SelectSpectator: the camera stays, the body is left behind. Multiplayer only, singleplayer has no bubble
	static void Detach(PlayerBase body) {
		if (!GetGame().IsMultiplayer() || !body || !body.GetIdentity()) return;
		if (!s_Bodies) s_Bodies = new map<int, PlayerBase>;

		PlayerIdentity identity = body.GetIdentity();
		s_Bodies.Set(identity.GetPlayerId(), body);
		GetGame().SelectPlayer(identity, NULL);
	}

	//Back into the body, detached or not (the camera is dropped either way)
	static void Reattach(PlayerBase body) {
		if (!body) return;

		PlayerIdentity identity = ZBGetIdentity(body); //Before forgetting the body: it may be the only link
		int sessionId = FindSessionId(body);
		if (sessionId != -1) s_Bodies.Remove(sessionId);
		GetGame().SelectPlayer(identity, body);
	}

	//NULL when this player's body isn't detached, or they left
	static PlayerBase GetBody(int sessionId) {
		PlayerBase body;
		if (!s_Bodies || !s_Bodies.Find(sessionId, body)) return NULL;
		if (!ZBGetIdentityById(sessionId)) return NULL;

		return body;
	}

	//-1 when the body isn't detached
	static int FindSessionId(PlayerBase body) {
		if (!s_Bodies || !body) return -1;

		foreach (int sessionId, PlayerBase detached : s_Bodies) {
			if (detached == body) return sessionId;
		}
		return -1;
	}
};
