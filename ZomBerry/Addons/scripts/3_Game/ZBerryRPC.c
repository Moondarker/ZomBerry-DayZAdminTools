//ZomBerry's own RPC layer: every ZomBerry message travels on one engine RPC id and starts with an opcode.
//The API follows CF's RPCManager (AddRPC/SendRPC, same handler signature), but every name is ZomBerry-prefixed:
//CF defines RPCManager, GetRPCManager and CallType itself, and both mods are often loaded together
const int ZBERRY_RPC_ID = 74635;

//Sent with every message, client and server must agree on them: only ever append, never renumber or reuse
enum ZBerryRPCOp {
	AdminAuth = 1,
	SyncPlayersRequest = 2,
	SyncPlayers = 3,
	SyncFunctionsRequest = 4,
	SyncFunctions = 5,
	ExecuteCommand = 6,
	SpawnObject = 7,
	MapTeleport = 8,
	AuthState = 9, //Server -> client: version, state, own permissions and spawn rules
	SetupToken = 10, //Client -> server: first-startup token
	PermsRequest = 11,
	PermsData = 12, //Server -> client: groups, admins, known nodes, online players
	PermsUpdate = 13 //Client -> server: new groups and admins
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
	protected Class m_Instance;
	protected string m_FuncName;
	protected int m_SPExec;

	void ZBerryRPCHandler(Class instance, string funcName, int spExec) {
		m_Instance = instance;
		m_FuncName = funcName;
		m_SPExec = spExec;
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
	bool AddRPC(ZBerryRPCOp opcode, string funcName, Class instance, int spExec = ZBerrySPExec.Server) {
		m_Handlers.Set(opcode, new ZBerryRPCHandler(instance, funcName, spExec));
		return true;
	}

	void RemoveRPC(ZBerryRPCOp opcode) {
		m_Handlers.Remove(opcode);
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
