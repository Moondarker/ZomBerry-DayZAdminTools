//Requests from the admin menu, handled on the server (or locally in singleplayer).
//Each handler reads its request, checks CanRun, then does the work: one code path for multiplayer and singleplayer
class ZBerryServerRPC {
	protected ZBerryAuth m_Auth;
	protected ZBerryBans m_Bans;

	void ZBerryServerRPC(ZBerryAuth auth, ZBerryBans bans) {
		m_Auth = auth;
		m_Bans = bans;

		GetZBerryRPC().AddRPC( ZBerryRPCOp.SyncPlayersRequest, "SyncPlayersRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.SyncFunctionsRequest, "SyncFunctionsRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.ExecuteCommand, "ExecuteCommand", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.SpawnObject, "SpawnObject", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.MapTeleport, "MapTeleport", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Kick, "Kick", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Ban, "Ban", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Unban, "Unban", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.BansRequest, "BansRequest", this, ZBerrySPExec.Client );
	}

	//Singleplayer: always (the local player is the admin). Multiplayer: only on the server, only with one of the permission nodes
	protected bool CanRun(ZBerryCallType type, PlayerIdentity sender, TStringArray nodes, string action) {
		if (!GetGame().IsMultiplayer()) return true;
		if (type != ZBerryCallType.Server || !GetGame().IsServer()) return false;

		foreach (string node : nodes) {
			if (m_Auth.HasPermission(sender, node)) return true;
		}

		ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + Who(sender) + " tried to " + action + ", but IS NOT ALLOWED (" + nodes[0] + ")");
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

		return sender.GetName() + " (" + sender.GetId() + ")";
	}

	//Never trust a player id sent by the client: the sender is the admin
	protected int GetSenderId(PlayerIdentity sender) {
		if (!sender) return 0;

		return sender.GetPlayerId();
	}

	void SyncPlayersRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (!CanRun(type, sender, {ZBerryPermissions.PLAYERS_LIST, ZBerryPermissions.MAP_PLAYER_MARKERS}, "get the player list")) return;

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

	//Any admin gets the list, filtered down to what they may run
	void SyncFunctionsRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (GetGame().IsMultiplayer()) {
			if (type != ZBerryCallType.Server || !GetGame().IsServer()) return;
			if (!m_Auth.IsAdmin(sender)) {
				ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + Who(sender) + " tried to get the function list, but IS NOT AN ADMIN");
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
		Param5< int, int, int, vector, autoptr TIntArray > funcParam; //Function id, Admin Id (ignored), Target Id, Admin cursor, Function params
		if ( !ctx.Read( funcParam ) ) return;

		ZBerryFunction cmdFunc = GetZomberryCmdAPI().GetFunc(funcParam.param1);
		string funcName = "#" + funcParam.param1;
		if (cmdFunc) funcName = cmdFunc.GetName();

		if (!CanRun(type, sender, {ZBerryPermissions.FunctionNode(funcName)}, "execute " + funcName)) return;
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
		if ( !ctx.Read( tgtParam ) ) return;

		string targetNode = ZBerryPermissions.SPAWN_GROUND;
		if (tgtParam.param4) targetNode = ZBerryPermissions.SPAWN_INVENTORY;
		if (!CanRun(type, sender, {targetNode}, "spawn " + tgtParam.param1)) return;

		if (!m_Auth.CanSpawn(sender, tgtParam.param1)) {
			ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + Who(sender) + " tried to spawn " + tgtParam.param1 + ", but the class IS NOT ALLOWED for their group");
			return;
		}

		string result = ZBerrySpawner.Spawn(tgtParam.param1, ZBGetPlayerById(GetSenderId(sender)), tgtParam.param3, tgtParam.param4);
		ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " " + result);
	}

	void MapTeleport( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1< vector > teleParam;
		if ( !ctx.Read( teleParam ) ) return;

		if (!CanRun(type, sender, {ZBerryPermissions.MAP_TELEPORT}, "teleport")) return;

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
			PlayerIdentity identity = man.GetIdentity();
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
		if (!targetPly || !targetPly.GetIdentity()) {
			Tell(sender, "Kick failed: the player has left");
			return;
		}

		PlayerIdentity targetIdentity = targetPly.GetIdentity();
		string problem = CheckTarget(sender, targetIdentity.GetPlainId(), targetIdentity.GetId());
		if (problem != "") {
			ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + Who(sender) + " tried to kick " + Who(targetIdentity) + ", refused: " + problem );
			Tell(sender, "Can't kick " + targetIdentity.GetName() + ": " + problem);
			return;
		}

		string reason = ZBerryBans.CleanReason(kickParam.param2);
		m_Bans.KickPlayer(targetPly, "Kicked by an admin: " + reason);
		ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " kicked " + Who(targetIdentity) + ": " + reason );
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
			if (!targetPly || !targetPly.GetIdentity()) {
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
			id = targetPly.GetIdentity().GetPlainId();
			guid = targetPly.GetIdentity().GetId();
			name = targetPly.GetIdentity().GetName();
		}

		string display = name;
		if (display == "") display = id + guid;

		string problem = CheckTarget(sender, id, guid);
		if (problem != "") {
			ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + Who(sender) + " tried to ban " + display + ", refused: " + problem );
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
		if (targetPly) m_Bans.KickPlayer(targetPly, "Banned by an admin " + length + ": " + reason);

		ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " banned " + display + " (" + entry.GetDisplayId() + ") " + length + ": " + reason );
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
			ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " unbanned " + id );
			Tell(sender, "Unbanned " + id);
		}

		SendBans(sender);
	}

	void BansRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (!CanRunOnServer(type, sender, ZBerryPermissions.PLAYERS_UNBAN, "see the ban list")) return;

		SendBans(sender);
	}

	protected void SendBans(PlayerIdentity recipient) {
		int now = ZBerryTime.Now();
		if (m_Bans.Prune(now) > 0) m_Bans.Save();

		GetZBerryRPC().SendRPC( ZBerryRPCOp.BansData, new Param2<ref ZBerryBanEntryArray, int>(m_Bans.GetAll(), now), true, recipient );
	}
};
