//Requests from the admin menu, handled on the server (or locally in singleplayer).
//Each handler reads its request, checks CanRun, then does the work: one code path for multiplayer and singleplayer
class ZBerryServerRPC {
	protected ZBerryAuth m_Auth;
	protected ZBerryBans m_Bans;
	protected ZBerryLocations m_Locations;
	protected ref ZBerryCallGroups m_CallGroups = new ZBerryCallGroups; //Repeated teleports, logged as one event

	void ZBerryServerRPC(ZBerryAuth auth, ZBerryBans bans, ZBerryLocations locations) {
		m_Auth = auth;
		m_Bans = bans;
		m_Locations = locations;

		GetZBerryRPC().AddRPC( ZBerryRPCOp.SyncFunctionsRequest, "SyncFunctionsRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.ExecuteCommand, "ExecuteCommand", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.ExecuteCommandTargets, "ExecuteCommandTargets", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.SpawnObject, "SpawnObject", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.MapTeleport, "MapTeleport", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Kick, "Kick", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Ban, "Ban", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Unban, "Unban", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.BansRequest, "BansRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Message, "Message", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LocationAdd, "LocationAdd", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LocationDelete, "LocationDelete", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LocationTeleport, "LocationTeleport", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.TeleportReturnsRequest, "TeleportReturnsRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.TeleportRevert, "TeleportRevert", this, ZBerrySPExec.Client );
	}

	//Singleplayer: always (the local player is the admin). Multiplayer: only on the server, only with one of the permission nodes
	protected bool CanRun(ZBerryCallType type, PlayerIdentity sender, TStringArray nodes, string action) {
		if (!GetGame().IsMultiplayer()) return true;
		if (type != ZBerryCallType.Server || !GetGame().IsServer()) return false;

		foreach (string node : nodes) {
			if (m_Auth.HasPermission(sender, node)) return true;
		}

		ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to " + action + " without " + nodes[0] );
		return false;
	}

	//Kicks and bans only exist in multiplayer
	protected bool CanRunOnServer(ZBerryCallType type, PlayerIdentity sender, string node, string action) {
		if (!GetGame().IsMultiplayer() || !sender) return false;

		return CanRun(type, sender, {node}, action);
	}

	protected void Tell(PlayerIdentity recipient, string message) {
		if (recipient) ZomberryStockFunctions.MessagePlayer(ZBGetPlayerById(recipient.GetPlayerId()), message);
	}

	//For log lines
	protected string Who(PlayerIdentity sender) {
		if (!sender) return "Singleplayer";

		return ZBDescribePlayer(sender);
	}

	//Never trust a player id sent by the client: the sender is the admin
	protected int GetSenderId(PlayerIdentity sender) {
		if (!sender) return 0;

		return sender.GetPlayerId();
	}

	//Any admin gets the list, filtered down to what they may run
	void SyncFunctionsRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (GetGame().IsMultiplayer()) {
			if (type != ZBerryCallType.Server || !GetGame().IsServer()) return;
			if (!m_Auth.IsAdmin(sender)) {
				ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to get the function list, but isn't an admin" );
				return;
			}
		}

		SendFunctionList(sender);
		ZomberryBase.Log( "ZomBerryDbg", Who(sender) + " - function list sync");
	}

	//Only the functions this admin may run, empty categories left out
	void SendFunctionList(PlayerIdentity recipient) {
		ZBerryCategoryArray catList = new ZBerryCategoryArray;

		ZBerryCategoryArray allCategories = GetZomberryCmdAPI().GetList();
		foreach (ZBerryCategory category : allCategories) {
			ZBerryCategory allowed = new ZBerryCategory(category.GetName(), category.GetColor());

			ZBerryFunctionArray functions = category.GetAll();
			foreach (ZBerryFunction cmdFunc : functions) {
				if (!GetGame().IsMultiplayer() || m_Auth.HasPermission(recipient, ZBerryPermissions.FunctionNode(cmdFunc.GetName()))) allowed.Insert(cmdFunc);
			}

			if (allowed.GetAll().Count() > 0) catList.Insert(allowed);
		}

		GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncFunctions, new Param1<ref ZBerryCategoryArray> (catList), true, recipient );
	}

	void ExecuteCommand( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4< int, int, vector, autoptr TIntArray > funcParam; //Function id, target session id, admin's cursor, function values
		if ( !ctx.Read( funcParam ) ) return;

		ZBerryFunction cmdFunc = GetRunnableFunction(type, sender, funcParam.param1);
		if (cmdFunc) RunOnTarget(cmdFunc, sender, funcParam.param2, funcParam.param3, funcParam.param4);
	}

	//Several selected players. Functions made for that run on each of them; any other function on the last selected
	//one only, as if it were the only one
	void ExecuteCommandTargets( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4< int, autoptr TIntArray, vector, autoptr TIntArray > funcParam; //Function id, target session ids (last: the main one), admin's cursor, function values
		if ( !ctx.Read( funcParam ) || !funcParam.param2 || funcParam.param2.Count() == 0 ) return;

		ZBerryFunction cmdFunc = GetRunnableFunction(type, sender, funcParam.param1);
		if (!cmdFunc) return;

		TIntArray targetIds = funcParam.param2;
		if (!cmdFunc.IsMultiTarget() || targetIds.Count() == 1) {
			RunOnTarget(cmdFunc, sender, targetIds[targetIds.Count() - 1], funcParam.param3, funcParam.param4);
			return;
		}
		RunOnTargets(cmdFunc, sender, targetIds, funcParam.param3, funcParam.param4);
	}

	//NULL (and logged) when the sender may not run it or it doesn't exist
	protected ZBerryFunction GetRunnableFunction(ZBerryCallType type, PlayerIdentity sender, int funcId) {
		ZBerryFunction cmdFunc = GetZomberryCmdAPI().GetFunc(funcId);
		string funcName = "#" + funcId;
		if (cmdFunc) funcName = cmdFunc.GetName();

		if (!CanRun(type, sender, {ZBerryPermissions.FunctionNode(funcName)}, "execute " + funcName)) return NULL;
		if (!cmdFunc) {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to run unknown function #" + funcId );
			return NULL;
		}
		return cmdFunc;
	}

	//False (logged, and the admin told) when the target's group is protected from the sender's rank
	protected bool MayTarget(PlayerIdentity sender, ZBerryFunction cmdFunc, int targetId, PlayerBase targetPly) {
		if (targetId == GetSenderId(sender) || !targetPly || !ZBGetIdentity(targetPly)) return true;
		if (m_Auth.CanTarget(sender, ZBGetIdentity(targetPly), cmdFunc.IsHarmful())) return true;

		ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, ZBGetIdentity(targetPly), "{actor} tried to run " + cmdFunc.GetDisplayName() + " on {target}, whose group is protected from their rank" );
		Tell(sender, "Can't run " + cmdFunc.GetDisplayName() + " on " + ZBGetIdentity(targetPly).GetName() + ": their group is protected from your rank");
		return false;
	}

	protected void RunOnTarget(ZBerryFunction cmdFunc, PlayerIdentity sender, int targetId, vector cursor, TIntArray values) {
		string funcName = cmdFunc.GetName();
		int adminId = GetSenderId(sender);
		PlayerBase targetPly = ZBGetPlayerById(targetId);
		if (!MayTarget(sender, cmdFunc, targetId, targetPly)) return;

		bool onTarget = targetId != adminId && targetPly && ZBGetIdentity(targetPly);
		PlayerIdentity targetIdentity = NULL;
		if (onTarget) targetIdentity = ZBGetIdentity(targetPly);

		//Functions run again and again are logged as one event per run, with where the moved player started and ended
		string groupKey = "";
		PlayerBase moved;
		vector from;
		if (ZBerryCallGroups.IsGrouped(funcName)) {
			groupKey = adminId.ToString() + ":" + funcName + ":" + targetId;
			moved = ZBGetPlayerById(adminId);
			if (ZBerryCallGroups.MovesTarget(funcName)) moved = targetPly;
			if (moved) from = moved.GetPosition();
		}

		ZomberryCmdAPI api = GetZomberryCmdAPI();
		ZBerryCall cmdCall = api.BeginCall(cmdFunc, sender, targetIdentity);
		RunFunction(cmdFunc, adminId, targetId, cursor, values);
		api.EndCall(cmdCall);

		vector to;
		if (moved) to = moved.GetPosition();
		LogCall(cmdCall, cmdFunc.DescribeCall(values), groupKey, moved != NULL, from, to);
	}

	//Once per selected player, each with its own call; refusals are logged one by one as usual, the runs that did
	//something as one event naming every player
	protected void RunOnTargets(ZBerryFunction cmdFunc, PlayerIdentity sender, TIntArray targetIds, vector cursor, TIntArray values) {
		int adminId = GetSenderId(sender);
		string call = cmdFunc.DescribeCall(values);
		ZomberryCmdAPI api = GetZomberryCmdAPI();
		array<ref ZBerryCall> done = new array<ref ZBerryCall>;
		TIntArray seen = new TIntArray;

		TIntArray targets = new TIntArray; //Each once, still online and not protected: their order places them on a ring
		foreach (int targetId : targetIds) {
			if (seen.Find(targetId) != -1) continue;
			seen.Insert(targetId);

			PlayerBase targetPly = ZBGetPlayerById(targetId);
			if (!targetPly || !MayTarget(sender, cmdFunc, targetId, targetPly)) continue; //Left meanwhile, or protected
			targets.Insert(targetId);
		}

		for (int index = 0; index < targets.Count(); ++index) {
			int runTargetId = targets[index];
			PlayerBase runTarget = ZBGetPlayerById(runTargetId);
			if (!runTarget) continue;

			PlayerIdentity targetIdentity = NULL;
			if (runTargetId != adminId) targetIdentity = ZBGetIdentity(runTarget);

			ZBerryCall cmdCall = api.BeginCall(cmdFunc, sender, targetIdentity);
			cmdCall.SetTargetPosition(index, targets.Count());
			RunFunction(cmdFunc, adminId, runTargetId, cursor, values);
			api.EndCall(cmdCall);

			if (cmdCall.GetOutcome() == ZBerryCallOutcome.DONE) {
				done.Insert(cmdCall);
			} else {
				LogCall(cmdCall, call);
			}
		}
		LogCalls(done, call, sender);
	}

	//The runs of one function on several players as one event. A description every run gave alike (with {target} for
	//the player) is used for all of them, otherwise "ran <function> on" them
	protected void LogCalls(array<ref ZBerryCall> done, string call, PlayerIdentity sender) {
		if (done.Count() == 0) return;
		if (done.Count() == 1) {
			LogCall(done[0], call);
			return;
		}

		string text = done[0].GetDescription();
		foreach (ZBerryCall sameCheck : done) {
			if (sameCheck.GetDescription() != text) text = "";
		}
		if (text == "" || !text.Contains("{target}")) text = "{actor} ran " + call + " on {target}";

		int flags = 0;
		if (done[0].GetFunction().IsHarmful()) flags = ZBerryEventFlags.HARMFUL;
		int type = ZBerryEventType.FUNCTION_PLAYER;
		if (IsTeleport(done[0].GetFunction())) type = ZBerryEventType.TELEPORT;
		ZBerryTeleportRecordArray moves = new ZBerryTeleportRecordArray;
		foreach (ZBerryCall movedCall : done) {
			ZBerryTeleportRecordArray callMoves = movedCall.GetMoves(); //A local: foreach over a method's result repeats the first
			foreach (ZBerryTeleportRecord move : callMoves) moves.Insert(move);
		}
		if (moves.Count() > 0) flags = flags | ZBerryEventFlags.REVERTABLE;

		string actorId, actorName;
		ZomberryBase.DescribeIdentity(sender, actorId, actorName);
		string firstId, firstName;
		TStringArray extraIds = new TStringArray;
		TStringArray extraNames = new TStringArray;
		for (int i = 0; i < done.Count(); ++i) {
			PlayerIdentity targetIdentity = done[i].GetTarget();
			if (!targetIdentity) targetIdentity = sender; //Run on the admin themselves
			string id, name;
			ZomberryBase.DescribeIdentity(targetIdentity, id, name);
			if (i == 0) {
				firstId = id;
				firstName = name;
			} else {
				extraIds.Insert(id);
				extraNames.Insert(name);
			}
		}
		ZBerryLogEvent logged = ZomberryBase.LogEventRaw(type, actorId, actorName, firstId, firstName, text, flags, extraIds, extraNames);
		AttachMoves(logged, moves);
	}

	//The positions a teleport event's players had, kept for "Teleport back" in the Logs tab
	static void AttachMoves(ZBerryLogEvent logged, ZBerryTeleportRecordArray moves) {
		if (!logged || !moves || moves.Count() == 0 || !g_ZBerryTeleportReturns) return;
		g_ZBerryTeleportReturns.Attach(logged.Id, moves);
	}

	//Functions in the Teleport category (stock or from mods) are logged as teleports, like map teleports
	protected static bool IsTeleport(ZBerryFunction cmdFunc) {
		return cmdFunc.GetCategory() == ZomberryStockFunctions.CATEGORY_TELEPORT;
	}

	//The server is stopping: held-back repeated calls are logged now
	void FlushCallGroups() {
		if (m_CallGroups) m_CallGroups.FlushAll();
	}

	//One event per call: refused (with the reason), the command's own description, or "ran <function>".
	//Confirmation prompts aren't logged, the confirmed run is
	//groupKey: a function run repeatedly (ZBerryCallGroups), logged once the run ends
	protected void LogCall(ZBerryCall cmdCall, string call, string groupKey = "", bool hasPositions = false, vector from = "0 0 0", vector to = "0 0 0") {
		PlayerIdentity admin = cmdCall.GetAdmin();
		PlayerIdentity target = cmdCall.GetTarget();
		if (cmdCall.GetOutcome() == ZBerryCallOutcome.CONFIRMING) return;
		if (cmdCall.GetOutcome() == ZBerryCallOutcome.REFUSED) {
			if (target) {
				ZomberryBase.LogEvent( ZBerryEventType.REFUSED, admin, target, "{actor} tried to run " + call + " on {target}: " + cmdCall.GetReason() );
			} else {
				ZomberryBase.LogEvent( ZBerryEventType.REFUSED, admin, NULL, "{actor} tried to run " + call + ": " + cmdCall.GetReason() );
			}
			return;
		}

		int flags = 0;
		if (cmdCall.GetFunction().IsHarmful()) flags = ZBerryEventFlags.HARMFUL;
		int type = ZBerryEventType.FUNCTION_SELF;
		string text = "{actor} ran " + call + " on themselves";
		if (target) {
			type = ZBerryEventType.FUNCTION_PLAYER;
			text = "{actor} ran " + call + " on {target}";
		} else if (cmdCall.GetFunction().IsServerWide()) {
			type = ZBerryEventType.FUNCTION_SERVER;
			text = "{actor} ran " + call;
		}
		if (IsTeleport(cmdCall.GetFunction())) type = ZBerryEventType.TELEPORT;
		if (cmdCall.GetDescription() != "") text = cmdCall.GetDescription();

		ZBerryTeleportRecordArray moves = cmdCall.GetMoves();
		if (groupKey != "") {
			m_CallGroups.Add(groupKey, type, admin, target, text, flags, hasPositions, from, to, moves);
			return;
		}
		if (moves.Count() > 0) flags = flags | ZBerryEventFlags.REVERTABLE;
		ZBerryLogEvent logged = ZomberryBase.LogEvent( type, admin, target, text, flags );
		AttachMoves(logged, moves);
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
		Param4< string, vector, int, int > tgtParam; //Class name, position (AT_POSITION), ZBerrySpawner mode, selected player
		if ( !ctx.Read( tgtParam ) ) return;

		string className = tgtParam.param1;
		int mode = tgtParam.param3;
		bool forPlayer = mode == ZBerrySpawner.PLAYER_FEET || mode == ZBerrySpawner.PLAYER_INVENTORY;
		bool intoInventory = mode == ZBerrySpawner.OWN_INVENTORY || mode == ZBerrySpawner.PLAYER_INVENTORY;

		TStringArray nodes = {ZBerryPermissions.SPAWN_GROUND};
		if (intoInventory) nodes = {ZBerryPermissions.SPAWN_INVENTORY};
		if (!CanRun(type, sender, nodes, "spawn " + className)) return;
		if (forPlayer && !CanRun(type, sender, {ZBerryPermissions.SPAWN_PLAYER}, "spawn " + className + " for another player")) return;

		if (!m_Auth.CanSpawn(sender, className)) {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to spawn " + className + ", which their group's spawn rules don't allow" );
			return;
		}

		PlayerBase admin = ZBGetPlayerById(GetSenderId(sender));
		PlayerBase recipient = admin;
		vector position = tgtParam.param2;
		if (forPlayer) {
			recipient = ZBGetPlayerById(tgtParam.param4);
			if (!recipient || recipient == admin) {
				Tell(sender, "Spawn failed: double-click another player in the list first");
				return;
			}
			if (!m_Auth.CanTarget(sender, ZBGetIdentity(recipient), false)) {
				ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, ZBGetIdentity(recipient), "{actor} tried to spawn " + className + " for {target}, whose group is protected from their rank" );
				Tell(sender, "Can't spawn for " + ZomberryStockFunctions.GetPlayerName(recipient) + ": their group is protected from your rank");
				return;
			}
			position = recipient.GetPosition();
		}

		string result = ZBerrySpawner.Spawn(className, recipient, position, intoInventory, admin);
		PlayerIdentity spawnTarget;
		if (forPlayer) spawnTarget = ZBGetIdentity(recipient);
		ZomberryBase.LogEvent( ZBerryEventType.SPAWN, sender, spawnTarget, "{actor} " + result );
		if (forPlayer && intoInventory && result.IndexOf("couldn't") != 0) ZomberryStockFunctions.MessagePlayer(recipient, "An admin gave you " + ZBerrySpawner.GetDisplayName(className));
	}

	//Map clicks and typed coordinates: the height comes from the surface
	void MapTeleport( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2< vector, int > teleParam; //Map position, selected player (-1: the admin themselves)
		if ( !ctx.Read( teleParam ) ) return;

		float x = teleParam.param1[0];
		float z = teleParam.param1[2];
		float worldSize = GetGame().GetWorld().GetWorldSize();
		if (x < 0 || z < 0 || x > worldSize || z > worldSize) {
			Tell(sender, "Teleport failed: X " + Math.Round(x) + " Y " + Math.Round(z) + " is outside the map (0 to " + Math.Round(worldSize) + ")");
			return;
		}

		vector position = Vector(x, GetGame().SurfaceY(x, z), z);
		Teleport(type, sender, teleParam.param2, position, ZBDescribePosition(position));
	}

	//The admin (targetId -1 or their own) or another player; 'where' names the place in the log
	protected void Teleport(ZBerryCallType type, PlayerIdentity sender, int targetId, vector position, string where) {
		if (targetId >= 0 && targetId != GetSenderId(sender)) {
			TeleportPlayer(type, sender, targetId, position, where);
			return;
		}

		if (!CanRun(type, sender, {ZBerryPermissions.MAP_TELEPORT}, "teleport")) return;

		PlayerBase adminPly = ZBGetPlayerById(GetSenderId(sender));

		if (!adminPly) {
			ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " tried to teleport, but has no character");
		} else if (adminPly.GetCommand_Vehicle()) {
			ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " tried to teleport, but was in vehicle");
		} else {
			vector startPos = adminPly.GetPosition();
			adminPly.SetPosition(position);
			LogMove(sender, NULL, adminPly, startPos, "{actor} teleported from " + ZBDescribePosition(startPos) + " to " + where, 0);
		}
	}

	//A teleport outside a command call (map, locations): logged, with where the player was kept for "Teleport back"
	protected void LogMove(PlayerIdentity sender, PlayerIdentity target, PlayerBase moved, vector startPos, string text, int flags) {
		ZBerryTeleportRecordArray moves = new ZBerryTeleportRecordArray;
		ZBerryTeleportRecord record = ZBerryTeleportReturns.MakeRecord(moved, startPos);
		if (record.SteamId != "") {
			moves.Insert(record);
			flags = flags | ZBerryEventFlags.REVERTABLE;
		}
		ZBerryLogEvent logged = ZomberryBase.LogEvent( ZBerryEventType.TELEPORT, sender, target, text, flags );
		AttachMoves(logged, moves);
	}

	//"Teleport back" in the Logs tab: needs the log and the revert node
	protected bool CanRevert(ZBerryCallType type, PlayerIdentity sender, string action) {
		if (!CanRun(type, sender, {ZBerryPermissions.LOGS_VIEW}, action)) return false;
		return CanRun(type, sender, {ZBerryPermissions.TELEPORT_REVERT}, action);
	}

	void TeleportReturnsRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1< int > data; //Event id
		if ( !ctx.Read( data ) ) return;
		if (!CanRevert(type, sender, "see the players of a teleport")) return;

		SendTeleportReturns(sender, data.param1);
	}

	protected void SendTeleportReturns(PlayerIdentity recipient, int eventId) {
		string reason = "";
		ZBerryTeleportRowArray rows = new ZBerryTeleportRowArray;
		ZBerryTeleportEvent teleport;
		if (g_ZBerryTeleportReturns) teleport = g_ZBerryTeleportReturns.Find(eventId);
		if (teleport) {
			rows = g_ZBerryTeleportReturns.GetRows(teleport);
		} else {
			reason = ZBerryTeleportReturns.NOT_KEPT;
		}
		GetZBerryRPC().SendRPC( ZBerryRPCOp.TeleportReturns, new Param3< int, string, ref ZBerryTeleportRowArray >( eventId, reason, rows ), true, recipient );
	}

	//Moves an event's players back; the revert is a teleport of its own, logged and revertable in turn
	void TeleportRevert( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2< int, ref TStringArray > data; //Event id, Steam64 ids (empty: everyone)
		if ( !ctx.Read( data ) ) return;
		if (!CanRevert(type, sender, "teleport players back")) return;
		if (!g_ZBerryTeleportReturns) return;

		int eventId = data.param1;
		ZBerryTeleportEvent teleport = g_ZBerryTeleportReturns.Find(eventId);
		string summary;
		ZBerryTeleportRecordArray moved = g_ZBerryTeleportReturns.Revert(sender, eventId, data.param2, summary);
		if (teleport && moved.Count() > 0) LogRevert(sender, teleport, moved);
		Tell(sender, summary);
		SendTeleportReturns(sender, eventId);
	}

	protected void LogRevert(PlayerIdentity sender, ZBerryTeleportEvent teleport, ZBerryTeleportRecordArray moved) {
		string when = ZBerryTeleportReturns.DescribeTime(teleport.Time);
		string actorId, actorName;
		ZomberryBase.DescribeIdentity(sender, actorId, actorName);
		int flags = ZBerryEventFlags.REVERTABLE;
		string text = "{actor} teleported back to where they were before the teleport at " + when;
		bool onlyThemselves = moved.Count() == 1 && moved[0].SteamId == actorId;
		if (!onlyThemselves) {
			flags = flags | ZBerryEventFlags.HARMFUL;
			text = "{actor} teleported {target} back to where they were before the teleport at " + when;
		}

		string firstId = "";
		string firstName = "";
		TStringArray extraIds = new TStringArray;
		TStringArray extraNames = new TStringArray;
		if (!onlyThemselves) {
			for (int i = 0; i < moved.Count(); ++i) {
				if (i == 0) {
					firstId = moved[i].SteamId;
					firstName = moved[i].Name;
				} else {
					extraIds.Insert(moved[i].SteamId);
					extraNames.Insert(moved[i].Name);
				}
			}
		}
		ZBerryLogEvent logged = ZomberryBase.LogEventRaw(ZBerryEventType.TELEPORT, actorId, actorName, firstId, firstName, text, flags, extraIds, extraNames);
		AttachMoves(logged, moved);
	}

	//Another player: harmful, so the rank protections apply
	protected void TeleportPlayer(ZBerryCallType type, PlayerIdentity sender, int targetId, vector position, string where) {
		if (!CanRun(type, sender, {ZBerryPermissions.MAP_TELEPORT_PLAYERS}, "teleport another player")) return;

		PlayerBase targetPly = ZBGetPlayerById(targetId);
		if (!targetPly || !ZBGetIdentity(targetPly)) {
			Tell(sender, "Teleport failed: the player has left");
			return;
		}
		if (!m_Auth.CanTarget(sender, ZBGetIdentity(targetPly), true)) {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, ZBGetIdentity(targetPly), "{actor} tried to teleport {target}, whose group is protected from their rank" );
			Tell(sender, "Can't teleport " + ZBGetIdentity(targetPly).GetName() + ": their group is protected from your rank");
			return;
		}
		vector startPos = ZomberryStockFunctions.GetPosSafe(targetPly);
		if (!ZomberryStockFunctions.SetPosSafe(targetPly, position)) {
			Tell(sender, "Teleport failed: " + ZBGetIdentity(targetPly).GetName() + " is in a vehicle");
			return;
		}

		LogMove(sender, ZBGetIdentity(targetPly), targetPly, startPos, "{actor} teleported {target} from " + ZBDescribePosition(startPos) + " to " + where, ZBerryEventFlags.HARMFUL);
		Tell(sender, "Teleported " + ZBGetIdentity(targetPly).GetName());
	}

	//Why the sender may not kick or ban this player, "" if they may: admins only act on lower ranks, superadmins on everyone
	protected string CheckTarget(PlayerIdentity sender, string id, string guid) {
		if ((id != "" && id == sender.GetPlainId()) || (guid != "" && guid == sender.GetId())) return "that's you";

		int targetLevel = Math.Max(m_Auth.GetLevelOfId(id), m_Auth.GetLevelOfId(guid));
		if (!ZBerryAuth.CanActOnLevel(m_Auth.GetLevel(sender), targetLevel)) return "they are an admin of the same or a higher rank";
		return "";
	}

	protected PlayerBase FindOnline(string id, string guid) {
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);

		foreach (Man man : players) {
			PlayerIdentity identity = ZBGetIdentity(man);
			if (!identity) continue;
			if ((id != "" && identity.GetPlainId() == id) || (guid != "" && identity.GetId() == guid)) return PlayerBase.Cast(man);
		}
		return NULL;
	}

	void Kick( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2< int, string > kickParam; //Target session id, reason
		if ( !ctx.Read( kickParam ) ) return;
		if (!CanRunOnServer(type, sender, ZBerryPermissions.PLAYERS_KICK, "kick a player")) return;

		PlayerBase targetPly = ZBGetPlayerById(kickParam.param1);
		if (!targetPly || !ZBGetIdentity(targetPly)) {
			Tell(sender, "Kick failed: the player has left");
			return;
		}

		PlayerIdentity targetIdentity = ZBGetIdentity(targetPly);
		string problem = CheckTarget(sender, targetIdentity.GetPlainId(), targetIdentity.GetId());
		if (problem != "") {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, targetIdentity, "{actor} tried to kick {target}, refused: " + problem );
			Tell(sender, "Can't kick " + targetIdentity.GetName() + ": " + problem);
			return;
		}

		string reason = ZBerryBans.CleanReason(kickParam.param2);
		ZBerryBans.SendKickNotice(targetIdentity, reason, false, 0);
		m_Bans.KickPlayer(targetPly, "Kicked by an admin: " + reason);
		ZomberryBase.LogEvent( ZBerryEventType.KICK, sender, targetIdentity, "{actor} kicked {target}: " + reason );
		Tell(sender, "Kicked " + targetIdentity.GetName());
	}

	void Ban( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4< int, string, string, int > banParam; //Target session id (-1: offline), offline id, reason, duration in seconds (0: permanent)
		if ( !ctx.Read( banParam ) ) return;
		if (!CanRunOnServer(type, sender, ZBerryPermissions.PLAYERS_BAN, "ban a player")) return;

		string id = "";
		string guid = "";
		PlayerBase targetPly;
		if (banParam.param1 >= 0) {
			targetPly = ZBGetPlayerById(banParam.param1);
			if (!targetPly || !ZBGetIdentity(targetPly)) {
				Tell(sender, "Ban failed: the player has left, ban them by id instead");
				return;
			}
		} else {
			string offlineId = banParam.param2.Trim();
			if (ZBerryBans.IsSteamId(offlineId)) {
				id = offlineId;
			} else if (ZBerryBans.IsGuid(offlineId)) {
				guid = offlineId;
			} else {
				Tell(sender, "Ban failed: " + offlineId + " is neither a Steam64 id nor a GUID");
				return;
			}
			targetPly = FindOnline(id, guid); //Online after all: ban both ids and kick
		}

		string name = "";
		if (targetPly) {
			id = ZBGetIdentity(targetPly).GetPlainId();
			guid = ZBGetIdentity(targetPly).GetId();
			name = ZBGetIdentity(targetPly).GetName();
		}

		string bannedId = id; //Steam64 when known; offline bans by GUID only know the GUID
		if (bannedId == "") bannedId = guid;
		string display = bannedId;
		if (name != "") display = name + " (" + bannedId + ")";

		string problem = CheckTarget(sender, id, guid);
		if (problem != "") {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to ban " + display + ", refused: " + problem );
			Tell(sender, "Can't ban " + display + ": " + problem);
			return;
		}

		int duration = Math.Clamp(banParam.param4, 0, ZBerryBans.MAX_DURATION);
		int now = ZBerryTime.Now();
		int until = 0;
		if (duration > 0) until = now + duration;

		string reason = ZBerryBans.CleanReason(banParam.param3);
		ZBerryBanEntry entry = new ZBerryBanEntry(id, guid, name, reason, sender.GetName(), now, until);
		if (!m_Bans.Add(entry)) {
			Tell(sender, "Ban failed: " + ZBerryBans.CONFIG_FILE + " can't be saved, see the server's ZomBerry log");
			return;
		}

		string length = "permanently";
		if (duration > 0) length = "for " + ZBerryTime.FormatDuration(duration);
		if (targetPly) {
			ZBerryBans.SendKickNotice(ZBGetIdentity(targetPly), reason, true, until);
			m_Bans.KickPlayer(targetPly, "Banned by an admin " + length + ": " + reason);
		}

		ZomberryBase.LogEventRaw( ZBerryEventType.BAN, sender.GetPlainId(), sender.GetName(), bannedId, name, "{actor} banned {target} " + length + ": " + reason );
		Tell(sender, "Banned " + display + " " + length);
		if (m_Auth.HasPermission(sender, ZBerryPermissions.PLAYERS_UNBAN)) SendBans(sender);
	}

	void Unban( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1< string > unbanParam; //Steam64 id or GUID
		if ( !ctx.Read( unbanParam ) ) return;
		if (!CanRunOnServer(type, sender, ZBerryPermissions.PLAYERS_UNBAN, "unban a player")) return;

		string id = unbanParam.param1.Trim();
		int removed = m_Bans.Remove(id);
		if (removed == 0) {
			Tell(sender, "Unban failed: " + id + " isn't banned (or " + ZBerryBans.CONFIG_FILE + " is broken)");
		} else {
			ZomberryBase.LogEventRaw( ZBerryEventType.UNBAN, sender.GetPlainId(), sender.GetName(), id, "", "{actor} unbanned {target}" );
			Tell(sender, "Unbanned " + id);
		}

		SendBans(sender);
	}

	void BansRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (!CanRunOnServer(type, sender, ZBerryPermissions.PLAYERS_UNBAN, "see the ban list")) return;

		SendBans(sender);
	}

	//Saved at the admin's own position, height included
	void LocationAdd( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1< string > locParam; //Name
		if ( !ctx.Read( locParam ) ) return;
		if (!CanRun(type, sender, {ZBerryPermissions.MAP_LOCATIONS_MANAGE}, "save a location")) return;

		PlayerBase adminPly = ZBGetPlayerById(GetSenderId(sender));
		if (!adminPly) {
			Tell(sender, "Location not saved: you have no character");
			return;
		}

		string name = ZBerryLocations.CleanName(locParam.param1);
		vector position = adminPly.GetPosition();
		string problem = m_Locations.Add(name, position);
		if (problem != "") {
			Tell(sender, "Location not saved: " + problem);
			return;
		}

		ZomberryBase.LogEvent( ZBerryEventType.LOCATION, sender, NULL, "{actor} saved location " + ZBerryMessages.ForLog(name) + " at " + ZBDescribePosition(position) );
		Tell(sender, "Saved location " + name);
	}

	void LocationDelete( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1< string > locParam; //Name
		if ( !ctx.Read( locParam ) ) return;
		if (!CanRun(type, sender, {ZBerryPermissions.MAP_LOCATIONS_MANAGE}, "delete a location")) return;

		ZBerryLocation location = m_Locations.Find(locParam.param1);
		string name = locParam.param1;
		vector position = "0 0 0";
		if (location) {
			name = location.Name;
			position = location.Position;
		}

		string problem = m_Locations.Remove(name);
		if (problem != "") {
			Tell(sender, "Location not deleted: " + problem);
			return;
		}

		ZomberryBase.LogEvent( ZBerryEventType.LOCATION, sender, NULL, "{actor} deleted location " + ZBerryMessages.ForLog(name) + " at " + ZBDescribePosition(position) );
		Tell(sender, "Deleted location " + name);
	}

	//To the exact saved position, height included
	void LocationTeleport( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2< string, int > locParam; //Name, session id of the player to move (-1: the admin)
		if ( !ctx.Read( locParam ) ) return;

		ZBerryLocation location = m_Locations.Find(locParam.param1);
		if (!location) {
			Tell(sender, "Teleport failed: there's no location called " + locParam.param1);
			return;
		}

		string where = ZBerryMessages.ForLog(location.Name) + " (" + ZBDescribePosition(location.Position) + ")";
		Teleport(type, sender, locParam.param2, location.Position, where);
	}

	//Talking to players isn't a function on them, so the rank protections don't apply: any admin may be messaged
	void Message( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2< int, string > msgParam; //Recipient's session id (-1: everyone), text
		if ( !ctx.Read( msgParam ) ) return;

		string text = ZBerryMessages.Clean(msgParam.param2);
		if (msgParam.param1 < 0) {
			if (!CanRun(type, sender, {ZBerryPermissions.MESSAGES_ANNOUNCE}, "send an announcement")) return;
			if (text == "") return;

			int count = ZBerryMessages.Announce(text);
			ZomberryBase.LogEvent( ZBerryEventType.MESSAGE, sender, NULL, "{actor} announced to everyone: " + ZBerryMessages.ForLog(text) );
			Tell(sender, "Announcement sent to " + count + " players");
			return;
		}

		if (!CanRun(type, sender, {ZBerryPermissions.MESSAGES_PRIVATE}, "send a private message")) return;
		if (text == "") return;

		PlayerBase recipient = ZBGetPlayerById(msgParam.param1);
		if (!recipient || (GetGame().IsMultiplayer() && !ZBGetIdentity(recipient))) {
			Tell(sender, "Message not sent: the player has left");
			return;
		}

		ZBerryMessages.SendPrivate(recipient, text);
		ZomberryBase.LogEvent( ZBerryEventType.MESSAGE, sender, ZBGetIdentity(recipient), "{actor} messaged {target}: " + ZBerryMessages.ForLog(text) );
		if (ZBGetIdentity(recipient)) Tell(sender, "Message sent to " + ZBGetIdentity(recipient).GetName());
	}

	protected void SendBans(PlayerIdentity recipient) {
		int now = ZBerryTime.Now();
		if (m_Bans.Prune(now) > 0) m_Bans.Save();

		GetZBerryRPC().SendRPC( ZBerryRPCOp.BansData, new Param2<ref ZBerryBanEntryArray, int>(m_Bans.GetAll(), now), true, recipient );
	}
};
