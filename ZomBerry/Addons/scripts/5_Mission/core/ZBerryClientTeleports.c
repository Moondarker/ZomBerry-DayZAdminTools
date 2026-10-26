//Client side of "Teleport back" in the Logs tab: an event's players as the server last described them
class ZBerryClientTeleports {
	ref ScriptInvoker OnReturns = new ScriptInvoker; //(int eventId): the server answered RequestReturns or a Revert

	protected ref map<int, ref ZBerryTeleportRowArray> m_Rows = new map<int, ref ZBerryTeleportRowArray>;
	protected ref map<int, string> m_Reasons = new map<int, string>; //Why an event's players aren't kept, "" if they are

	void ZBerryClientTeleports() {
		GetZBerryRPC().AddRPC( ZBerryRPCOp.TeleportReturns, "TeleportReturns", this, ZBerrySPExec.Client );
	}

	void RequestReturns(int eventId) {
		m_Rows.Remove(eventId);
		m_Reasons.Remove(eventId);
		GetZBerryRPC().SendRPC( ZBerryRPCOp.TeleportReturnsRequest, new Param1< int >( eventId ), true );
	}

	//False until the server answered for this event
	bool GetReturns(int eventId, out string reason, out ZBerryTeleportRowArray rows) {
		if (!m_Reasons.Contains(eventId)) return false;
		reason = m_Reasons.Get(eventId);
		rows = m_Rows.Get(eventId);
		return true;
	}

	//steamIds empty: everyone of the event who can be moved back. The server answers with the event's players again
	void Revert(int eventId, TStringArray steamIds) {
		TStringArray ids = new TStringArray; //A local: a temporary in the Param would be freed before it's sent
		if (steamIds) ids.InsertAll(steamIds);
		GetZBerryRPC().SendRPC( ZBerryRPCOp.TeleportRevert, new Param2< int, ref TStringArray >( eventId, ids ), true );
	}

	void TeleportReturns( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param3< int, string, ref ZBerryTeleportRowArray > data;
		if (type != ZBerryCallType.Client || !ctx.Read(data)) return;

		ZBerryTeleportRowArray rows = data.param3;
		if (!rows) rows = new ZBerryTeleportRowArray;
		m_Rows.Set(data.param1, rows);
		m_Reasons.Set(data.param1, data.param2);
		OnReturns.Invoke(data.param1);
	}
};

static ref ZBerryClientTeleports g_ZBerryClientTeleports;

static ZBerryClientTeleports GetZBerryClientTeleports() {
	if (!g_ZBerryClientTeleports) g_ZBerryClientTeleports = new ZBerryClientTeleports();

	return g_ZBerryClientTeleports;
}
