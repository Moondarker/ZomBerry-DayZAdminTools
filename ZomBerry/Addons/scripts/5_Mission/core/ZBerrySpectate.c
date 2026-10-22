//Spectating a player (#60), both sides.
//Server: switches the admin to a ZomBerryCamSpec camera and detaches their body (ZBerryBodies), so the network bubble
//follows the camera. Tells their client whom to follow (zomberry_spectate feature, args: network id low/high, third
//person), asks the target's client to report its 1st/3rd person mode (zomberry_report_camera feature, CameraMode opcode
//back) and its camera heading and pitch (CameraView), and relays both. Ends when asked, when the target dies or leaves,
//or when the admin loses the permission.
//Client: the spectator side sets up the camera, the target side reports its camera while it's watched
class ZBerrySpectate {
	static const string FEATURE_SPECTATE = "zomberry_spectate";
	static const string FEATURE_REPORT = "zomberry_report_camera";
	static const int CHECK_MS = 1000;
	static const int REPORT_MS = 500;
	static const int REPORT_REPEAT_MS = 5000; //Reported again even unchanged, in case a message got lost
	static const int VIEW_MS = 100; //Camera heading and pitch: often, unguaranteed, only when they changed
	static const float VIEW_STEP = 0.5; //Degrees: smaller changes aren't sent

	//Server: admin session id -> target session id, and the targets' last reported mode
	protected ref map<int, int> m_Watching = new map<int, int>;
	protected ref map<int, string> m_RequiredNodes = new map<int, string>;
	protected ref map<int, bool> m_ThirdPerson = new map<int, bool>;
	protected ref map<int, vector> m_Views = new map<int, vector>; //Target session id -> camera heading, pitch, 0
	protected bool m_Checking;

	//Client, target side
	protected bool m_Reporting;
	protected bool m_LastReported;
	protected int m_LastReportTime;
	protected float m_LastYaw, m_LastPitch;
	protected int m_LastViewTime;

	void ZBerrySpectate() {
		GetZBerryRPC().AddRPC( ZBerryRPCOp.CameraMode, "CameraMode", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.CameraView, "CameraView", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.SpectateView, "SpectateView", this, ZBerrySPExec.Client );
		if (GetGame().IsServer()) GetZBerryClientFeatures().OnServerFeatureChanged.Insert(OnFeatureChanged);
		GetZomberryCmdAPI().RegisterClientFeature(FEATURE_SPECTATE, this, "OnSpectateFeature");
		GetZomberryCmdAPI().RegisterClientFeature(FEATURE_REPORT, this, "OnReportFeature");
	}

	void ~ZBerrySpectate() {
		if (!GetGame()) return;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(CheckTargets);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ReportCamera);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ReportView);
		if (g_ZBerryClientFeatures) g_ZBerryClientFeatures.OnServerFeatureChanged.Remove(OnFeatureChanged);
	}

	//Server: the spectate feature was switched off without Stop (permission lost, disconnected): end it properly,
	//or the admin's camera would hang on with the body still detached
	protected void OnFeatureChanged(PlayerIdentity identity, string name, bool enabled) {
		if (name != FEATURE_SPECTATE || enabled || !identity) return;
		Stop(identity.GetPlayerId(), "Stopped spectating: you may no longer spectate");
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
		GetGame().SelectSpectator(ZBGetIdentity(admin), "ZomBerryCamSpec", target.GetPosition() + "0 1.6 0");
		ZBerryBodies.Detach(admin);

		m_Watching.Set(adminId, targetId);
		m_RequiredNodes.Set(adminId, requiredNode);
		SendSpectate(ZBGetIdentity(admin), target, requiredNode);
		GetZomberryCmdAPI().SetClientFeature(ZBGetIdentity(target), FEATURE_REPORT, true);

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
			//Told first: while detached the message reaches their client without the body
			if (message != "") ZomberryStockFunctions.MessagePlayer(admin, message);
			GetZomberryCmdAPI().SetClientFeature(ZBGetIdentity(admin), FEATURE_SPECTATE, false);
			ZomberryStockFunctions.FreezeBody(admin, false);
			ZBerryBodies.Reattach(admin);
		}

		//Nobody watches the target anymore: it can stop reporting
		if (IsWatched(targetId)) return;
		m_ThirdPerson.Remove(targetId);
		m_Views.Remove(targetId);
		PlayerBase target = ZBGetPlayerById(targetId);
		if (target) GetZomberryCmdAPI().SetClientFeature(ZBGetIdentity(target), FEATURE_REPORT, false);
	}

	protected bool IsWatched(int targetId) {
		foreach (int adminId, int watchedId : m_Watching) {
			if (watchedId == targetId) return true;
		}
		return false;
	}

	protected int GetSessionId(PlayerBase player) {
		if (!GetGame().IsMultiplayer() || !ZBGetIdentity(player)) return 0;
		return ZBGetIdentity(player).GetPlayerId();
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
			} else {
				SendView(id, target); //Their position at least once a second, see SendView
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
			if (admin) SendSpectate(ZBGetIdentity(admin), watched, m_RequiredNodes.Get(adminId));
		}
	}

	//Server: a watched player's client reports where its camera looks, passed on to everyone watching them.
	//Unguaranteed both ways: the next report replaces a lost one
	void CameraView( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2<float, float> view; //Heading, pitch in degrees
		if ( !ctx.Read( view ) ) return;
		if (GetGame().IsMultiplayer() && (type != ZBerryCallType.Server || !sender)) return;

		int targetId = 0;
		if (sender) targetId = sender.GetPlayerId();
		if (!IsWatched(targetId)) return; //Nobody asked
		m_Views.Set(targetId, Vector(view.param1, Math.Clamp(view.param2, -90, 90), 0));

		PlayerBase watched = ZBGetPlayerById(targetId);
		if (!watched) return;
		foreach (int adminId, int watchedId : m_Watching) {
			if (watchedId == targetId) SendView(adminId, watched);
		}
	}

	//Server: the target's camera angles (if reported yet) and position. The position lets the spectator's camera
	//follow a target that left its network bubble (teleported, drove off): the camera goes there, the bubble follows
	protected void SendView(int adminId, PlayerBase target) {
		PlayerIdentity adminIdentity = ZBGetIdentityById(adminId);
		if (GetGame().IsMultiplayer() && !adminIdentity) return;

		vector view;
		bool hasView = m_Views.Find(GetSessionId(target), view);
		Param4<bool, float, float, vector> data = new Param4<bool, float, float, vector>(hasView, view[0], view[1], target.GetPosition());
		GetZBerryRPC().SendRPC( ZBerryRPCOp.SpectateView, data, false, adminIdentity );
	}

	//Client, spectator side
	void SpectateView( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4<bool, float, float, vector> view; //Has angles, heading, pitch, position
		if ( !ctx.Read( view ) ) return;
		if (type != ZBerryCallType.Client) return;

		ZomBerryCamSpec.SetView(view.param1, view.param2, view.param3, view.param4);
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
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ReportView);
		if (!enabled) return;

		m_LastReportTime = -REPORT_REPEAT_MS;
		m_LastViewTime = -REPORT_REPEAT_MS;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ReportCamera, REPORT_MS, true);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ReportView, VIEW_MS, true);
	}

	//Where this player's own camera looks, free look included: other clients only get the body's aiming angles,
	//which miss looking up and down
	protected void ReportView() {
		vector angles = GetGame().GetCurrentCameraDirection().VectorToAngles();
		float yaw = angles[0];
		float pitch = angles[1];
		if (pitch > 180) pitch -= 360;

		int now = GetGame().GetTime();
		bool turned = Math.AbsFloat(ZomBerryCamSpec.AngleDifference(yaw, m_LastYaw)) >= VIEW_STEP || Math.AbsFloat(pitch - m_LastPitch) >= VIEW_STEP;
		if (!turned && now - m_LastViewTime < REPORT_REPEAT_MS) return;

		m_LastYaw = yaw;
		m_LastPitch = pitch;
		m_LastViewTime = now;
		GetZBerryRPC().SendRPC( ZBerryRPCOp.CameraView, new Param2<float, float>(yaw, pitch), false, NULL );
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
