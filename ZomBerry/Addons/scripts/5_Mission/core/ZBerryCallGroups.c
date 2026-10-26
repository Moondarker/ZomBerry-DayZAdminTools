//One run of repeated calls: the same function, by the same admin, on the same target
class ZBerryCallGroup {
	int Type;
	int Flags;
	string ActorId, ActorName, TargetId, TargetName; //Kept as text: the players may have left when it's logged
	string Text; //The last call's: it says where that one went
	int Count;
	bool HasPositions;
	vector From, To; //Where the moved player was before the first call and after the last
	int LastMs; //GetGame().GetTime() of the last call
	ref ZBerryTeleportRecordArray Moves = new ZBerryTeleportRecordArray; //RecordMove's, the first per player: reverting goes back to the run's start
};

//Some functions are run again and again in quick succession: stepping forward 10 m at a time, putting a player at the
//cursor until the spot is right. Their calls are held back while they keep coming (gaps under GAP_MS) and logged as one
//event when the run ends: how often, and from where to where. A different function starts its own entry
class ZBerryCallGroups {
	static const int GAP_MS = 3000;
	static const int TICK_MS = 1000;

	protected ref map<string, ref ZBerryCallGroup> m_Groups = new map<string, ref ZBerryCallGroup>;

	void ZBerryCallGroups() {
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Tick, TICK_MS, true);
	}

	void ~ZBerryCallGroups() {
		if (GetGame()) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Tick);
	}

	//Stock functions that move one player a short way each time
	static bool IsGrouped(string funcName) {
		return funcName == "TPForward" || funcName == "TPCur";
	}

	//Which player such a function moves: the target for Target to Cursor, the admin otherwise
	static bool MovesTarget(string funcName) {
		return funcName == "TPCur";
	}

	void Add(string key, int type, PlayerIdentity actor, PlayerIdentity target, string text, int flags, bool hasPositions, vector from, vector to, ZBerryTeleportRecordArray moves = NULL) {
		int now = GetGame().GetTime();
		ZBerryCallGroup group = m_Groups.Get(key);
		if (group && now - group.LastMs > GAP_MS) {
			Flush(key);
			group = NULL;
		}

		if (!group) {
			group = new ZBerryCallGroup;
			group.Type = type;
			group.Flags = flags;
			group.Text = text;
			group.HasPositions = hasPositions;
			group.From = from;
			ZomberryBase.DescribeIdentity(actor, group.ActorId, group.ActorName);
			ZomberryBase.DescribeIdentity(target, group.TargetId, group.TargetName);
			m_Groups.Set(key, group);
		}

		++group.Count;
		group.Text = text;
		group.To = to;
		group.LastMs = now;
		if (!moves) return;
		foreach (ZBerryTeleportRecord move : moves) {
			bool known = false;
			foreach (ZBerryTeleportRecord kept : group.Moves) {
				if (kept.SteamId == move.SteamId) known = true;
			}
			if (!known) group.Moves.Insert(move);
		}
	}

	protected void Tick() {
		int now = GetGame().GetTime();
		TStringArray ended = new TStringArray;
		foreach (string key, ZBerryCallGroup group : m_Groups) {
			if (now - group.LastMs > GAP_MS) ended.Insert(key);
		}
		foreach (string endedKey : ended) Flush(endedKey);
	}

	//The server is stopping: nothing may wait any longer
	void FlushAll() {
		TStringArray keys = m_Groups.GetKeyArray();
		foreach (string key : keys) Flush(key);
	}

	protected void Flush(string key) {
		ZBerryCallGroup group = m_Groups.Get(key);
		if (!group) return;

		string text = group.Text;
		string from = ZBDescribePosition(group.From);
		if (group.Count > 1) {
			text += ", " + group.Count + " times in a row";
			if (group.HasPositions) text += " from " + from;
		} else if (group.HasPositions) {
			text += ", from " + from;
		}
		int flags = group.Flags;
		if (group.Moves.Count() > 0) flags = flags | ZBerryEventFlags.REVERTABLE;
		ZBerryLogEvent logged = ZomberryBase.LogEventRaw(group.Type, group.ActorId, group.ActorName, group.TargetId, group.TargetName, text, flags);
		ZBerryServerRPC.AttachMoves(logged, group.Moves);
		m_Groups.Remove(key);
	}
};
