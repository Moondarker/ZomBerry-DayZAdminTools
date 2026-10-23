//ZomBerry's own RPC layer: every ZomBerry message travels on one engine RPC id and starts with an opcode.
//The API follows CF's RPCManager (AddRPC/SendRPC, same handler signature), but every name is ZomBerry-prefixed:
//CF defines RPCManager, GetRPCManager and CallType itself, and both mods are often loaded together
const int ZBERRY_RPC_ID = 74635;

//Sent with every message, client and server must agree on them: only ever append, never renumber or reuse.
//Retired in 0.8.0, never reuse: 2, 3 (player list polling), 19, 20 (map marker polling), 21 (player headings)
enum ZBerryRPCOp {
	AdminAuth = 1,
	SyncFunctionsRequest = 4,
	SyncFunctions = 5,
	ExecuteCommand = 6,
	SpawnObject = 7,
	MapTeleport = 8,
	AuthState = 9, //Server -> client: version, state, own permissions and spawn rules
	SetupToken = 10, //Client -> server: first-startup token
	PermsRequest = 11,
	PermsData = 12, //Server -> client: groups, admins, known nodes, online players
	PermsUpdate = 13, //Client -> server: new groups and admins
	Kick = 14, //Client -> server: session id, reason
	Ban = 15, //Client -> server: session id (-1: offline), offline id, reason, duration in seconds (0: permanent)
	Unban = 16, //Client -> server: Steam64 id or GUID
	BansRequest = 17,
	BansData = 18, //Server -> client: ban list, server time
	Subscribe = 22, //Client -> server: what this client shows (ZBerryStreamFlags, marker categories, selected player, view)
	Roster = 23, //Server -> client: who is online, when it changed
	PlayerState = 24, //Server -> client: packed positions, headings, health (ZBerryStatePacker)
	Markers = 25, //Server -> client: the map markers of one category, when they changed
	ClientFeature = 26, //Server -> client: switch a client-side feature (name, on/off, arguments)
	CameraMode = 27, //Client -> server: a watched player's 1st/3rd person mode (ZBerrySpectate)
	LogEvents = 28, //Server -> client: start over, a batch of log events (ZBerryEventLog)
	LogDamage = 29, //Server -> client: start over, new player and weapon names, a batch of damage records
	Message = 30, //Client -> server: recipient's session id (-1: everyone), text
	PlayerDetails = 31, //Server -> client: the selected player's id, ping, position, time online, status (ZBerryPlayerDetails)
	LocationAdd = 32, //Client -> server: name; saved at the admin's position
	LocationDelete = 33, //Client -> server: name
	LocationTeleport = 34, //Client -> server: name, session id of the player to move (-1: the admin)
	Locations = 35, //Server -> client: version, the saved locations (ZBerryLocations)
	Chat = 36, //Server -> client: a chat line, for admins in FreeCam or spectating (their client has no character to get it)
	CameraView = 37, //Client -> server: a watched player's camera heading and pitch (ZBerrySpectate)
	SpectateView = 38, //Server -> client: the watched player's camera heading and pitch, relayed to whoever spectates them
	KickNotice = 39, //Server -> client, just before a kick or ban: banned, reason, ban end, seconds left (ZBerryKickNotice)
	ExecuteCommandTargets = 40 //Client -> server: function id, session ids of the selected players (last: the one single-target functions act on), cursor, values
}

//Which side received the call
enum ZBerryCallType {
	Server = 0,
	Client
}

//Singleplayer runs server and client in one game: which handler side gets called there
enum ZBerrySPExec {
	Server = 0,
	Client,
	Both
}

class ZBerryRPCHandler {
	protected Class m_Instance; //Weak, and not cleared when a plain class instance is deleted: see RemoveMissionHandlers
	protected string m_FuncName;
	protected int m_SPExec;
	protected bool m_Persistent;

	void ZBerryRPCHandler(Class instance, string funcName, int spExec, bool persistent = false) {
		m_Instance = instance;
		m_FuncName = funcName;
		m_SPExec = spExec;
		m_Persistent = persistent;
	}

	bool IsPersistent() {
		return m_Persistent;
	}

	Class GetInstance() {
		return m_Instance;
	}

	string GetFuncName() {
		return m_FuncName;
	}

	int GetSPExec() {
		return m_SPExec;
	}
};

class ZBerryRPCManager {
	protected ref map<int, ref ZBerryRPCHandler> m_Handlers = new map<int, ref ZBerryRPCHandler>;

	//Handler signature: void funcName(ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	//persistent: the instance lives as long as the game (DayZGame's), not just the mission
	bool AddRPC(ZBerryRPCOp opcode, string funcName, Class instance, int spExec = ZBerrySPExec.Server, bool persistent = false) {
		m_Handlers.Set(opcode, new ZBerryRPCHandler(instance, funcName, spExec, persistent));
		return true;
	}

	void RemoveRPC(ZBerryRPCOp opcode) {
		m_Handlers.Remove(opcode);
	}

	//Client, when it leaves a server: everything the mission registered goes, its objects are built anew for the next
	//one. A handler outliving its instance crashed the game on the next join (a deleted plain class still looks set)
	void RemoveMissionHandlers() {
		TIntArray stale = new TIntArray;
		foreach (int opcode, ZBerryRPCHandler handler : m_Handlers) {
			if (!handler.IsPersistent()) stale.Insert(opcode);
		}
		foreach (int staleOpcode : stale) m_Handlers.Remove(staleOpcode);
	}

	//Only target-less RPCs are routed here: the engine delivers RPCs with a target to that object instead
	void SendRPC(ZBerryRPCOp opcode, Param params = NULL, bool guaranteed = false, PlayerIdentity sendToIdentity = NULL, Object sendToTarget = NULL) {
		array<ref Param> sendData = new array<ref Param>;
		sendData.Insert(new Param1<int>(opcode));
		if (params) sendData.Insert(params);

		//In singleplayer a "Both" handler reads the data twice (server side, then client side), so send it twice
		ZBerryRPCHandler handler = m_Handlers.Get(opcode);
		if (params && !GetGame().IsMultiplayer() && handler && handler.GetSPExec() == ZBerrySPExec.Both) sendData.Insert(params);

		GetGame().RPC(sendToTarget, ZBERRY_RPC_ID, sendData, guaranteed, sendToIdentity);
	}

	void OnRPC(PlayerIdentity sender, Object target, ParamsReadContext ctx) {
		Param1<int> header;
		if (!ctx.Read(header)) {
			Print("[ZomBerryRPC] WARN: Can't read RPC opcode");
			return;
		}

		string receivedFrom = "server";
		if (GetGame().IsDedicatedServer()) {
			if (!sender) {
				Print("[ZomBerryRPC] WARN: RPC #" + header.param1 + " without sender");
				return;
			}
			receivedFrom = sender.GetPlainId();
		}

		ZBerryRPCHandler handler = m_Handlers.Get(header.param1);
		if (!handler) {
			Print("[ZomBerryRPC] WARN: " + receivedFrom + " sent unknown RPC #" + header.param1);
			return;
		}
		if (!handler.GetInstance()) return;

		bool singleplayer = !GetGame().IsMultiplayer();
		int spExec = handler.GetSPExec();
		Param4<ZBerryCallType, ParamsReadContext, PlayerIdentity, Object> callData = new Param4<ZBerryCallType, ParamsReadContext, PlayerIdentity, Object>(ZBerryCallType.Server, ctx, sender, target);

		if ((GetGame().IsServer() && !singleplayer) || (singleplayer && spExec != ZBerrySPExec.Client)) {
			GetGame().GameScript.CallFunctionParams(handler.GetInstance(), handler.GetFuncName(), NULL, callData);
		}

		if ((GetGame().IsClient() && !singleplayer) || (singleplayer && spExec != ZBerrySPExec.Server)) {
			callData.param1 = ZBerryCallType.Client;
			GetGame().GameScript.CallFunctionParams(handler.GetInstance(), handler.GetFuncName(), NULL, callData);
		}
	}
};

static ref ZBerryRPCManager g_ZBerryRPCManager;

static ZBerryRPCManager GetZBerryRPC() {
	if (!g_ZBerryRPCManager) g_ZBerryRPCManager = new ZBerryRPCManager();

	return g_ZBerryRPCManager;
}

modded class DayZGame {
	override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx) {
		if (rpc_type == ZBERRY_RPC_ID) {
			GetZBerryRPC().OnRPC(sender, target, ctx);
			return;
		}

		super.OnRPC(sender, target, rpc_type, ctx);
	}
};
