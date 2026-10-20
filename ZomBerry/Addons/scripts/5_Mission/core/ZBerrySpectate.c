//Spectating a player (#60), both sides.
//Server: switches the admin to a ZomBerryCamSpec camera, tells their client whom to follow (zomberry_spectate feature,
//args: network id low/high, third person), asks the target's client to report its 1st/3rd person mode
//(zomberry_report_camera feature, CameraMode opcode back) and relays it. Ends when asked, or when the target dies or leaves.
//Client: the spectator side sets up the camera, the target side reports its camera mode while it's watched
class ZBerrySpectate {
	static const string FEATURE_SPECTATE = "zomberry_spectate";
	static const string FEATURE_REPORT = "zomberry_report_camera";
	static const int CHECK_MS = 1000;
	static const int REPORT_MS = 500;
	static const int REPORT_REPEAT_MS = 5000; //Reported again even unchanged, in case a message got lost

	//Server: admin session id -> target session id, and the targets' last reported mode
	protected ref map<int, int> m_Watching = new map<int, int>;
	protected ref map<int, string> m_RequiredNodes = new map<int, string>;
	protected ref map<int, bool> m_ThirdPerson = new map<int, bool>;
	protected bool m_Checking;

	//Client, target side
	protected bool m_Reporting;
	protected bool m_LastReported;
	protected int m_LastReportTime;

	void ZBerrySpectate() {
		GetZBerryRPC().AddRPC( ZBerryRPCOp.CameraMode, "CameraMode", this, ZBerrySPExec.Client );
		GetZomberryCmdAPI().RegisterClientFeature(FEATURE_SPECTATE, this, "OnSpectateFeature");
		GetZomberryCmdAPI().RegisterClientFeature(FEATURE_REPORT, this, "OnReportFeature");
	}

	void ~ZBerrySpectate() {
		if (!GetGame()) return;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(CheckTargets);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ReportCamera);
	}

	//Server
	bool IsWatching(int adminId) {
		return m_Watching.Contains(adminId);
	}

	//Server: false (with a reason) when it can't start
	bool Start(PlayerBase admin, PlayerBase target, string requiredNode, out string problem) {
		if (!admin || !target || admin == target) {
			problem = "select another player first";
			return false;
		}
		if (!target.IsAlive()) {
			problem = "they're dead";
			return false;
		}

		int adminId = GetSessionId(admin);
		int targetId = GetSessionId(target);
		ZomberryStockFunctions.FreezeBody(admin, true);
		GetGame().SelectSpectator(admin.GetIdentity(), "ZomBerryCamSpec", target.GetPosition() + "0 1.6 0");

		m_Watching.Set(adminId, targetId);
		m_RequiredNodes.Set(adminId, requiredNode);
		SendSpectate(admin.GetIdentity(), target, requiredNode);
		GetZomberryCmdAPI().SetClientFeature(target.GetIdentity(), FEATURE_REPORT, true);

		if (!m_Checking) {
			m_Checking = true;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(CheckTargets, CHECK_MS, true);
		}
		return true;
	}

	//Server: back into the admin's body
	void Stop(int adminId, string message) {
		int targetId;
		if (!m_Watching.Find(adminId, targetId)) return;
		m_Watching.Remove(adminId);
		m_RequiredNodes.Remove(adminId);

		PlayerBase admin = ZBGetPlayerById(adminId);
		if (admin) {
			GetZomberryCmdAPI().SetClientFeature(admin.GetIdentity(), FEATURE_SPECTATE, false);
			ZomberryStockFunctions.FreezeBody(admin, false);
			GetGame().SelectPlayer(admin.GetIdentity(), admin);
			if (message != "") ZomberryStockFunctions.MessagePlayer(admin, message);
		}

		//Nobody watches the target anymore: it can stop reporting
		if (IsWatched(targetId)) return;
		m_ThirdPerson.Remove(targetId);
		PlayerBase target = ZBGetPlayerById(targetId);
		if (target) GetZomberryCmdAPI().SetClientFeature(target.GetIdentity(), FEATURE_REPORT, false);
	}

	protected bool IsWatched(int targetId) {
		foreach (int adminId, int watchedId : m_Watching) {
			if (watchedId == targetId) return true;
		}
		return false;
	}

	protected int GetSessionId(PlayerBase player) {
		if (!GetGame().IsMultiplayer() || !player.GetIdentity()) return 0;
		return player.GetIdentity().GetPlayerId();
	}

	protected void SendSpectate(PlayerIdentity adminIdentity, PlayerBase target, string requiredNode) {
		int low, high;
		target.GetNetworkID(low, high);
		int thirdPerson = 0;
		if (m_ThirdPerson.Get(GetSessionId(target))) thirdPerson = 1;

		TIntArray args = {low, high, thirdPerson};
		GetZomberryCmdAPI().SetClientFeature(adminIdentity, FEATURE_SPECTATE, true, args, requiredNode);
	}

	//Server, every CHECK_MS while anyone spectates: targets that died or left end it, admins who left are forgotten
	protected void CheckTargets() {
		TIntArray adminIds = new TIntArray;
		foreach (int adminId, int targetId : m_Watching) adminIds.Insert(adminId);

		foreach (int id : adminIds) {
			int watchedId = m_Watching.Get(id);
			PlayerBase admin = ZBGetPlayerById(id);
			PlayerBase target = ZBGetPlayerById(watchedId);
			if (!admin) {
				m_Watching.Remove(id);
				m_RequiredNodes.Remove(id);
			} else if (!target) {
				Stop(id, "Stopped spectating: they left");
			} else if (!target.IsAlive()) {
				Stop(id, "Stopped spectating: they died");
			}
		}

		if (m_Watching.Count() > 0) return;
		m_Checking = false;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(CheckTargets);
	}

	//Server: a watched player's client reports its camera mode, passed on to everyone watching them
	void CameraMode( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<bool> mode; //Third person
		if ( !ctx.Read( mode ) ) return;
		if (GetGame().IsMultiplayer() && (type != ZBerryCallType.Server || !sender)) return;

		int targetId = 0;
		if (sender) targetId = sender.GetPlayerId();
		if (!IsWatched(targetId)) return; //Nobody asked
		if (m_ThirdPerson.Contains(targetId) && m_ThirdPerson.Get(targetId) == mode.param1) return;
		m_ThirdPerson.Set(targetId, mode.param1);

		PlayerBase watched = ZBGetPlayerById(targetId);
		if (!watched) return;
		foreach (int adminId, int watchedId : m_Watching) {
			if (watchedId != targetId) continue;
			PlayerBase admin = ZBGetPlayerById(adminId);
			if (admin) SendSpectate(admin.GetIdentity(), watched, m_RequiredNodes.Get(adminId));
		}
	}

	//Client, spectator side. args: network id low, high, third person
	void OnSpectateFeature(bool enabled, TIntArray args) {
		if (!enabled || args.Count() < 3) {
			ZomBerryCamSpec.ClearTarget();
			return;
		}
		ZomBerryCamSpec.SetTarget(args[0], args[1], args[2] != 0);
	}

	//Client, target side: report the camera mode while someone watches
	void OnReportFeature(bool enabled, TIntArray args) {
		m_Reporting = enabled;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ReportCamera);
		if (!enabled) return;

		m_LastReportTime = -REPORT_REPEAT_MS;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ReportCamera, REPORT_MS, true);
	}

	protected void ReportCamera() {
		DayZPlayerImplement player = DayZPlayerImplement.Cast(GetGame().GetPlayer());
		bool thirdPerson = player && player.IsInThirdPerson();
		int now = GetGame().GetTime();
		if (thirdPerson == m_LastReported && now - m_LastReportTime < REPORT_REPEAT_MS) return;

		m_LastReported = thirdPerson;
		m_LastReportTime = now;
		GetZBerryRPC().SendRPC( ZBerryRPCOp.CameraMode, new Param1<bool>(thirdPerson), true, NULL );
	}
};

static ref ZBerrySpectate g_ZBerrySpectate;

static ZBerrySpectate GetZBerrySpectate() {
	if (!g_ZBerrySpectate) g_ZBerrySpectate = new ZBerrySpectate();

	return g_ZBerrySpectate;
}
