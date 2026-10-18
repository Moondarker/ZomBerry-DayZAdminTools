//Requests from the admin menu, handled on the server (or locally in singleplayer).
//Each handler reads its request, checks CanRun, then does the work: one code path for multiplayer and singleplayer
class ZBerryServerRPC {
	protected ZBerryAuth m_Auth;

	void ZBerryServerRPC(ZBerryAuth auth) {
		m_Auth = auth;

		GetZBerryRPC().AddRPC( ZBerryRPCOp.SyncPlayersRequest, "SyncPlayersRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.SyncFunctionsRequest, "SyncFunctionsRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.ExecuteCommand, "ExecuteCommand", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.SpawnObject, "SpawnObject", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.MapTeleport, "MapTeleport", this, ZBerrySPExec.Client );
	}

	//Singleplayer: always (the local player is the admin). Multiplayer: only on the server, only for admins
	protected bool CanRun(ZBerryCallType type, PlayerIdentity sender, string action) {
		if (!GetGame().IsMultiplayer()) return true;
		if (type != ZBerryCallType.Server || !GetGame().IsServer()) return false;
		if (m_Auth.IsAdmin(sender)) return true;

		ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + Who(sender) + " tried to " + action + ", but IS NOT AN ADMIN");
		return false;
	}

	//For log lines
	protected string Who(PlayerIdentity sender) {
		if (!sender) return "Singleplayer";

		return sender.GetName() + " (" + sender.GetId() + ")";
	}

	//Never trust a player id sent by the client: the sender is the admin
	protected int GetSenderId(PlayerIdentity sender) {
		if (!sender) return 0;

		return sender.GetPlayerId();
	}

	void SyncPlayersRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (!CanRun(type, sender, "get the player list")) return;

		ZBerryPlayerArray playerList = BuildPlayerList();
		GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncPlayers, new Param2<ref ZBerryPlayerArray, int> (playerList, GetGame().GetTime()), true, sender );
		ZomberryBase.DebugLog( 1, "ZomBerryDbg", Who(sender) + " - player list sync"); //Every 2 seconds while the map is open
	}

	protected ZBerryPlayerArray BuildPlayerList() {
		ZBerryPlayerArray playerList = new ZBerryPlayerArray;

		if (!GetGame().IsMultiplayer()) {
			PlayerBase localPlayer = PlayerBase.Cast(GetGame().GetPlayer());
			string localName = "Player";
			if (localPlayer.GetItemInHands()) localName += " [" + localPlayer.GetItemInHands().GetInventoryItemType().GetName() + "]";

			playerList.Insert(new ZBerryPlayer(0, localName, true, localPlayer.GetPosition(), localPlayer.GetHealth(), localPlayer.GetHealth("", "Blood")));
			return playerList;
		}

		if (ZBerryInstallMode.IsActive()) {
			playerList.Insert(new ZBerryPlayer(3, "ZomBerry", false, Vector(0,0,0), 0, 0));
			playerList.Insert(new ZBerryPlayer(3, "Installation Mode", false, Vector(0,0,0), 0, 0));
			playerList.Insert(new ZBerryPlayer(3, "Active", true, Vector(0,0,0), 0, 0));
			playerList.Insert(new ZBerryPlayer(3, "No real players displayed!", false, Vector(0,0,0), 0, 0));
			return playerList;
		}

		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);

		foreach (Man man : players) {
			PlayerBase player = PlayerBase.Cast(man);
			if (!player || !player.GetIdentity()) continue; //Connecting/disconnecting players have no identity

			string plyName = player.GetIdentity().GetName();
			if (player.GetItemInHands() && !player.GetCommand_Vehicle()) plyName += " [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]";
			if (player.GetCommand_Vehicle()) plyName += " [" + player.GetCommand_Vehicle().GetTransport().GetDisplayName() + "]";

			playerList.Insert(new ZBerryPlayer(player.GetIdentity().GetPlayerId(), plyName, m_Auth.IsAdmin(player.GetIdentity()), player.GetPosition(), player.GetHealth(), player.GetHealth("", "Blood")));
		}

		return playerList;
	}

	void SyncFunctionsRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (!CanRun(type, sender, "get the function list")) return;

		SendFunctionList(sender);
		ZomberryBase.Log( "ZomBerryDbg", Who(sender) + " - function list sync");
	}

	//Also used when the function list changes while the menu is open (installation mode steps)
	void SendFunctionList(PlayerIdentity recipient) {
		ZBerryCategoryArray catList = GetZomberryCmdAPI().GetList();

		GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncFunctions, new Param1<ref ZBerryCategoryArray> (catList), true, recipient );
	}

	void ExecuteCommand( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param5< int, int, int, vector, autoptr TIntArray > funcParam; //Function id, Admin Id (ignored), Target Id, Admin cursor, Function params
		if ( !ctx.Read( funcParam ) ) return;

		if (!CanRun(type, sender, "execute function #" + funcParam.param1)) return;

		ZBerryFunction cmdFunc = GetZomberryCmdAPI().GetFunc(funcParam.param1);
		if (!cmdFunc) {
			ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + Who(sender) + " tried to execute unknown function #" + funcParam.param1);
			return;
		}

		int adminId = GetSenderId(sender);
		int targetId = funcParam.param3;
		RunFunction(cmdFunc, adminId, targetId, funcParam.param4, funcParam.param5);

		PlayerBase targetPly = ZBGetPlayerById(targetId);
		if (targetId != adminId && targetPly && targetPly.GetIdentity()) {
			ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " executed " + cmdFunc.GetName() + " on target " + Who(targetPly.GetIdentity()));
		} else {
			ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " executed " + cmdFunc.GetName());
		}
	}

	//Calls the function with the (funcName, adminId, targetId, cursor, values) signature
	protected void RunFunction(ZBerryFunction cmdFunc, int adminId, int targetId, vector cursor, TIntArray values) {
		//Keep the array in a local: a temporary passed straight into the Param5 was gone by the time the function ran
		TIntArray callValues = PadFuncValues(values);
		Param5< string, int, int, vector, autoptr TIntArray > callParams = new Param5< string, int, int, vector, autoptr TIntArray >(cmdFunc.GetName(), adminId, targetId, cursor, callValues);

		GetGame().GameScript.CallFunctionParams( cmdFunc.GetInstance(), cmdFunc.GetName(), NULL, callParams );
	}

	//Functions index up to 3 values, a short or missing array would throw inside them
	protected TIntArray PadFuncValues(TIntArray values) {
		TIntArray result = new TIntArray;
		if (values) result.InsertAll(values);
		while (result.Count() < 3) result.Insert(0);

		return result;
	}

	void SpawnObject( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4< string, int, vector, bool > tgtParam; //Class name, Admin Id (ignored), Ground position, Into inventory
		if (ZBerryInstallMode.IsActive()) return;
		if ( !ctx.Read( tgtParam ) ) return;

		if (!CanRun(type, sender, "spawn " + tgtParam.param1)) return;

		string result = ZBerrySpawner.Spawn(tgtParam.param1, ZBGetPlayerById(GetSenderId(sender)), tgtParam.param3, tgtParam.param4);
		ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " " + result);
	}

	void MapTeleport( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1< vector > teleParam;
		if ( !ctx.Read( teleParam ) ) return;

		if (!CanRun(type, sender, "teleport")) return;

		PlayerBase adminPly = ZBGetPlayerById(GetSenderId(sender));
		vector position = Vector(teleParam.param1[0], GetGame().SurfaceY(teleParam.param1[0], teleParam.param1[2]), teleParam.param1[2]);

		if (!adminPly) {
			ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " tried to teleport, but has no character");
		} else if (adminPly.GetCommand_Vehicle()) {
			ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " tried to teleport, but was in vehicle");
		} else {
			adminPly.SetPosition(position);
			ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " teleported to position " + position.ToString());
		}
	}
};
