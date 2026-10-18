//ZomBerry's own RPC layer: every ZomBerry message travels on one engine RPC id and is routed by (mod, function) name.
//The API follows CF's RPCManager (AddRPC/SendRPC, same handler signature), but every name is ZomBerry-prefixed:
//CF defines RPCManager, GetRPCManager and CallType itself, and both mods are often loaded together
const int ZBERRY_RPC_ID = 74635;

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
	protected int m_SPExec;

	void ZBerryRPCHandler(Class instance, int spExec) {
		m_Instance = instance;
		m_SPExec = spExec;
	}

	Class GetInstance() {
		return m_Instance;
	}

	int GetSPExec() {
		return m_SPExec;
	}
};

class ZBerryRPCManager {
	protected ref map<string, ref map<string, ref ZBerryRPCHandler>> m_Handlers = new map<string, ref map<string, ref ZBerryRPCHandler>>;

	//Handler signature: void funcName(ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	bool AddRPC(string modName, string funcName, Class instance, int spExec = ZBerrySPExec.Server) {
		if (!m_Handlers.Contains(modName)) m_Handlers.Insert(modName, new map<string, ref ZBerryRPCHandler>);

		m_Handlers.Get(modName).Set(funcName, new ZBerryRPCHandler(instance, spExec));
		return true;
	}

	void RemoveRPC(string modName, string funcName) {
		if (!m_Handlers.Contains(modName)) return;

		m_Handlers.Get(modName).Remove(funcName);
		if (m_Handlers.Get(modName).Count() == 0) m_Handlers.Remove(modName);
	}

	//Only target-less RPCs are routed here: the engine delivers RPCs with a target to that object instead
	void SendRPC(string modName, string funcName, Param params = NULL, bool guaranteed = false, PlayerIdentity sendToIdentity = NULL, Object sendToTarget = NULL) {
		array<ref Param> sendData = new array<ref Param>;
		sendData.Insert(new Param2<string, string>(modName, funcName));
		if (params) sendData.Insert(params);

		//In singleplayer a "Both" handler reads the data twice (server side, then client side), so send it twice
		ZBerryRPCHandler handler = FindHandler(modName, funcName);
		if (params && !GetGame().IsMultiplayer() && handler && handler.GetSPExec() == ZBerrySPExec.Both) sendData.Insert(params);

		GetGame().RPC(sendToTarget, ZBERRY_RPC_ID, sendData, guaranteed, sendToIdentity);
	}

	void OnRPC(PlayerIdentity sender, Object target, ParamsReadContext ctx) {
		Param2<string, string> header;
		if (!ctx.Read(header)) {
			Print("[ZomBerryRPC] WARN: Can't read RPC header");
			return;
		}

		string receivedFrom = "server";
		if (GetGame().IsDedicatedServer()) {
			if (!sender) {
				Print("[ZomBerryRPC] WARN: RPC " + header.param1 + "::" + header.param2 + " without sender");
				return;
			}
			receivedFrom = sender.GetPlainId();
		}

		ZBerryRPCHandler handler = FindHandler(header.param1, header.param2);
		if (!handler) {
			Print("[ZomBerryRPC] WARN: " + receivedFrom + " sent unknown RPC " + header.param1 + "::" + header.param2);
			return;
		}
		if (!handler.GetInstance()) return;

		bool singleplayer = !GetGame().IsMultiplayer();
		int spExec = handler.GetSPExec();
		Param4<ZBerryCallType, ParamsReadContext, PlayerIdentity, Object> callData = new Param4<ZBerryCallType, ParamsReadContext, PlayerIdentity, Object>(ZBerryCallType.Server, ctx, sender, target);

		if ((GetGame().IsServer() && !singleplayer) || (singleplayer && spExec != ZBerrySPExec.Client)) {
			GetGame().GameScript.CallFunctionParams(handler.GetInstance(), header.param2, NULL, callData);
		}

		if ((GetGame().IsClient() && !singleplayer) || (singleplayer && spExec != ZBerrySPExec.Server)) {
			callData.param1 = ZBerryCallType.Client;
			GetGame().GameScript.CallFunctionParams(handler.GetInstance(), header.param2, NULL, callData);
		}
	}

	protected ZBerryRPCHandler FindHandler(string modName, string funcName) {
		map<string, ref ZBerryRPCHandler> functions;
		if (!m_Handlers.Find(modName, functions)) return NULL;

		ZBerryRPCHandler handler;
		functions.Find(funcName, handler);
		return handler;
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
