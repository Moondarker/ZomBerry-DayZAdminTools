//One loadout file on the server: loaded once, never overwritten while broken
class ZBerryLoadoutFile {
	protected string m_Path;
	protected string m_FileName;
	protected bool m_Writable = true;
	protected ref ZBerryLoadoutsConfig m_Config = new ZBerryLoadoutsConfig;

	void Load(string dir, string fileName) {
		m_FileName = fileName;
		m_Path = dir + fileName;
		m_Writable = true;
		m_Config = new ZBerryLoadoutsConfig;
		if (!FileExist(m_Path)) return;

		ZBerryLoadoutsConfig loaded;
		string loadError;
		if (!JsonFileLoader<ZBerryLoadoutsConfig>.LoadFile(m_Path, loaded, loadError) || !loaded) {
			m_Writable = false;
			ZomberryBase.Log( "ZomBerryLoadouts", "ERROR: " + m_FileName + " can't be read, nothing is saved to it until it's fixed: " + loadError );
			return;
		}

		m_Config = loaded;
		if (!m_Config.Loadouts) m_Config.Loadouts = new ZBerryLoadoutArray;
		for (int i = 0; i < m_Config.Loadouts.Count(); ) { //Hand-edited files: no empty or duplicate names
			ZBerryLoadout loadout = m_Config.Loadouts[i];
			if (loadout) ZBerryLoadoutKit.Sanitize(loadout); //Empty is fine: an admin may have worn nothing before a try
			if (!loadout || loadout.Name == "" || Find(loadout.Name) != loadout) {
				m_Config.Loadouts.RemoveOrdered(i);
				continue;
			}
			++i;
		}
		ZomberryBase.Log( "ZomBerryLoadouts", "INFO: Loaded " + m_Config.Loadouts.Count() + " loadouts from " + m_FileName );
	}

	ZBerryLoadoutArray GetAll() {
		return m_Config.Loadouts;
	}

	ZBerryLoadout Find(string name) {
		return ZBerryLoadoutKit.Find(m_Config.Loadouts, name);
	}

	//"" when saved, otherwise why not. Replaces the one of the same name
	string Put(ZBerryLoadout loadout, int maxCount) {
		if (!m_Writable) return m_FileName + " is broken, see the server's ZomBerry log";

		ZBerryLoadout previous = Find(loadout.Name); //Kept alive in case saving fails
		int index = m_Config.Loadouts.Find(previous);
		if (index == -1 && m_Config.Loadouts.Count() >= maxCount) return "there are already " + maxCount;

		if (index == -1) {
			m_Config.Loadouts.Insert(loadout);
		} else {
			m_Config.Loadouts.Set(index, loadout);
		}
		if (Save()) return "";

		if (index == -1) {
			m_Config.Loadouts.Remove(m_Config.Loadouts.Count() - 1);
		} else {
			m_Config.Loadouts.Set(index, previous);
		}
		return m_FileName + " can't be saved, see the server's ZomBerry log";
	}

	//"" when deleted, otherwise why not
	string Remove(string name) {
		ZBerryLoadout loadout = Find(name);
		if (!loadout) return "there's none called " + name;
		if (!m_Writable) return m_FileName + " is broken, see the server's ZomBerry log";

		int index = m_Config.Loadouts.Find(loadout);
		m_Config.Loadouts.RemoveOrdered(index);
		if (Save()) return "";

		m_Config.Loadouts.InsertAt(loadout, index);
		return m_FileName + " can't be saved, see the server's ZomBerry log";
	}

	protected bool Save() {
		if (!m_Writable || m_Path == "") return false;

		string saveError;
		if (JsonFileLoader<ZBerryLoadoutsConfig>.SaveFile(m_Path, m_Config, saveError)) return true;

		ZomberryBase.Log( "ZomBerryLoadouts", "ERROR: Can't save " + m_FileName + ": " + saveError );
		return false;
	}
};

//Loadouts on the server: shared ones (ZomBerryLoadouts.json) and the gear admins wore before their first try
//(ZomBerryLoadoutOriginals.json, one per Steam64 id, until they restore it)
class ZBerryLoadoutServer {
	static const string SHARED_FILE = "ZomBerryLoadouts.json";
	static const string ORIGINALS_FILE = "ZomBerryLoadoutOriginals.json";
	static const int MAX_SHARED = 100;
	static const int MAX_ORIGINALS = 1000;

	protected ZBerryAuth m_Auth;
	protected ref ZBerryLoadoutFile m_Shared = new ZBerryLoadoutFile;
	protected ref ZBerryLoadoutFile m_Originals = new ZBerryLoadoutFile;

	void ZBerryLoadoutServer(ZBerryAuth auth) {
		m_Auth = auth;

		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutsRequest, "LoadoutsRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutDetailsRequest, "LoadoutDetailsRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutCapture, "LoadoutCapture", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutTry, "LoadoutTry", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutRestore, "LoadoutRestore", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutShare, "LoadoutShare", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutDelete, "LoadoutDelete", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.DeathLoadoutRequest, "DeathLoadoutRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.DeathLoadoutTry, "DeathLoadoutTry", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.DeathLoadoutRestore, "DeathLoadoutRestore", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutPlayerRequest, "LoadoutPlayerRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutGive, "LoadoutGive", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutUndo, "LoadoutUndo", this, ZBerrySPExec.Client );
	}

	void Load(string dir) {
		m_Shared.Load(dir, SHARED_FILE);
		m_Originals.Load(dir, ORIGINALS_FILE);
	}

	//Same as ZBerryServerRPC.CanRun: singleplayer may do everything, multiplayer only on the server with the node
	protected bool CanRun(ZBerryCallType type, PlayerIdentity sender, string node, string action) {
		if (!GetGame().IsMultiplayer()) return true;
		if (type != ZBerryCallType.Server || !GetGame().IsServer()) return false;
		if (m_Auth.HasPermission(sender, node)) return true;

		ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to " + action + " without " + node );
		return false;
	}

	protected void Tell(PlayerIdentity recipient, string message) {
		PlayerBase player;
		if (recipient) player = ZBGetPlayerById(recipient.GetPlayerId());
		if (player || !GetGame().IsMultiplayer()) ZomberryStockFunctions.MessagePlayer(player, message);
	}

	//The admin's character, alive. NULL (and the admin told) without one
	protected PlayerBase GetCharacter(PlayerIdentity sender, string action) {
		int sessionId = 0;
		if (sender) sessionId = sender.GetPlayerId();
		PlayerBase player = ZBGetPlayerById(sessionId);
		if (player && player.IsAlive()) return player;

		Tell(sender, "Can't " + action + ": you have no living character");
		return NULL;
	}

	//Who owns a saved original: the Steam64 id, "singleplayer" offline
	protected static string GetOwnerKey(PlayerIdentity identity) {
		if (!identity) return "singleplayer";
		return identity.GetPlainId();
	}

	protected static string GetName(PlayerIdentity identity) {
		if (!identity) return "Singleplayer";
		return identity.GetName();
	}

	bool HasOriginal(PlayerIdentity identity) {
		return m_Originals.Find(GetOwnerKey(identity)) != NULL;
	}

	//Changing or deleting a shared loadout: their own with the share node, lower ranks' too (the owner's current
	//rank, even offline), anyone's with the manage node
	bool CanEdit(PlayerIdentity identity, ZBerryLoadout loadout) {
		if (!GetGame().IsMultiplayer()) return true;
		if (m_Auth.HasPermission(identity, ZBerryPermissions.LOADOUTS_MANAGE)) return true;
		if (!m_Auth.HasPermission(identity, ZBerryPermissions.LOADOUTS_SHARE)) return false;
		if (!loadout || loadout.OwnerId == identity.GetPlainId()) return true;

		return ZBerryAuth.CanActOnLevel(m_Auth.GetLevel(identity), m_Auth.GetLevelOfId(loadout.OwnerId));
	}

	void LoadoutsRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_USE, "see the shared loadouts")) return;

		SendList(sender);
	}

	//Headers only, each as this admin sees it: their restricted items, whether they may change it
	void SendList(PlayerIdentity recipient) {
		ZBerryLoadoutHeaderArray headers = new ZBerryLoadoutHeaderArray;
		ZBerryLoadoutArray all = m_Shared.GetAll(); //foreach straight over a method's result gives the first element every time
		foreach (ZBerryLoadout loadout : all) {
			headers.Insert(new ZBerryLoadoutHeader(loadout.Name, loadout.OwnerName, loadout.CountAll(), loadout.CountRestricted(m_Auth, recipient), loadout.CountMissing(), CanEdit(recipient, loadout)));
		}

		GetZBerryRPC().SendRPC( ZBerryRPCOp.Loadouts, new Param2<bool, ref ZBerryLoadoutHeaderArray>(HasOriginal(recipient), headers), true, recipient );
	}

	//Every online admin who uses loadouts, after the shared list changed
	protected void SendListToAll() {
		if (!GetGame().IsMultiplayer()) {
			SendList(NULL);
			return;
		}

		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man man : players) {
			PlayerIdentity identity = ZBGetIdentity(man);
			if (identity && m_Auth.HasPermission(identity, ZBerryPermissions.LOADOUTS_USE)) SendList(identity);
		}
	}

	void LoadoutDetailsRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<string> data; //Name
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_USE, "see a shared loadout")) return;

		ZBerryLoadout loadout = m_Shared.Find(data.param1);
		if (!loadout) return;
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutDetails, new Param1<ref ZBerryLoadout>(loadout), true, sender );
	}

	//The admin's gear, sent back for them to keep on their side
	void LoadoutCapture( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<string> data; //Name
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_USE, "save a loadout")) return;

		string name = ZBerryLoadoutKit.CleanName(data.param1);
		if (name == "") {
			Tell(sender, "Loadout not saved: it needs a name");
			return;
		}
		PlayerBase player = GetCharacter(sender, "save a loadout");
		if (!player) return;

		ZBerryLoadout loadout = ZBerryLoadoutKit.Capture(player, name);
		if (loadout.Items.Count() == 0) {
			Tell(sender, "Loadout not saved: you aren't wearing or holding anything");
			return;
		}
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutCaptured, new Param1<ref ZBerryLoadout>(loadout), true, sender );
	}

	//Only on the admin themselves. The gear they wore before the first try is kept until they restore it
	void LoadoutTry( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param3<bool, string, ref ZBerryLoadout> data; //Shared, name, the admin's own loadout (empty for a shared one)
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_USE, "apply a loadout")) return;
		if (!CanRun(type, sender, ZBerryPermissions.SPAWN_INVENTORY, "apply a loadout")) return;

		ZBerryLoadout loadout = data.param3;
		string source = "their loadout ";
		if (data.param1) {
			loadout = m_Shared.Find(data.param2);
			source = "shared loadout ";
			if (!loadout) {
				Tell(sender, "Loadout not applied: there's no shared loadout called " + data.param2);
				return;
			}
		} else if (!ZBerryLoadoutKit.Sanitize(loadout)) {
			Tell(sender, "Loadout not applied: it's empty");
			return;
		} else if (loadout.OwnerId != "" && loadout.OwnerId != GetOwnerKey(sender)) {
			source = ""; //Another player's current gear (LoadoutPlayerRequest): logged with them as the target
		}

		StartTry(sender, loadout, source);
	}

	//The admin's own gear is saved first, unless it already is
	protected void StartTry(PlayerIdentity sender, ZBerryLoadout loadout, string source) {
		PlayerBase player = GetCharacter(sender, "apply a loadout");
		if (!player) return;

		string problem = SaveOriginal(player, sender);
		if (problem != "") { //Their gear would be lost
			Tell(sender, "Loadout not applied: your gear can't be saved, " + problem);
			return;
		}

		ZBerryLoadoutJob job = ZBerryLoadoutJob.Start(player, loadout, m_Auth, sender);
		job.LogNote = source;
		job.OnDone.Insert(OnTried);
		SendList(sender); //Restore is possible from now on
	}

	protected void OnTried(ZBerryLoadoutJob job) {
		string source = job.LogNote;
		PlayerIdentity sender = GetJobIdentity(job);
		if (!job.IsDressed) {
			ZomberryBase.LogEvent( ZBerryEventType.LOADOUT, sender, NULL, "{actor} took off their gear for " + source + ZBerryMessages.ForLog(job.JobLoadout.Name) + ", but couldn't put it on" );
			Tell(sender, "Loadout not put on: your character is gone or busy, Undo gives your gear back");
			return;
		}

		ZBerryLoadoutResult result = job.JobResult;
		string text = "{actor} applied " + source + ZBerryMessages.ForLog(job.JobLoadout.Name);
		if (source == "") text = "{actor} applied {target}'s current gear";
		string told = "Applied " + job.JobLoadout.Name;
		if (result.RestrictedCount > 0) {
			text += ", without " + result.RestrictedCount + " items their spawn rules don't allow";
			told += ", without " + result.RestrictedCount + " items your spawn rules don't allow";
		}
		if (result.MissingCount > 0) text += ", without " + result.MissingCount + " items this server doesn't have";
		told += result.DescribeLosses();
		if (source == "") { //Another player's current gear: they're the target, they may be offline by now
			string actorId, actorName;
			ZomberryBase.DescribeIdentity(sender, actorId, actorName);
			ZomberryBase.LogEventRaw( ZBerryEventType.LOADOUT, actorId, actorName, job.JobLoadout.OwnerId, job.JobLoadout.OwnerName, text );
		} else {
			ZomberryBase.LogEvent( ZBerryEventType.LOADOUT, sender, NULL, text );
		}
		Tell(sender, told);
	}

	//Keeps what a character wears before a loadout replaces it, unless something is kept already (the gear from before
	//the first replacement is what goes back). "" when it's kept, otherwise why not
	protected string SaveOriginal(PlayerBase player, PlayerIdentity owner) {
		if (HasOriginal(owner)) return "";

		ZBerryLoadout original = ZBerryLoadoutKit.Capture(player, GetOwnerKey(owner));
		original.OwnerId = GetOwnerKey(owner);
		original.OwnerName = GetName(owner);
		return m_Originals.Put(original, MAX_ORIGINALS);
	}

	//The admin a job ran for, NULL if they left (and in singleplayer)
	protected PlayerIdentity GetJobIdentity(ZBerryLoadoutJob job) {
		if (!GetGame().IsMultiplayer()) return NULL;
		return ZBGetIdentityById(job.SessionId);
	}

	//The gear from before the first try, as it was: no spawn rules, it was theirs
	void LoadoutRestore( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (GetGame().IsMultiplayer() && (type != ZBerryCallType.Server || !GetGame().IsServer())) return;

		ZBerryLoadout original = m_Originals.Find(GetOwnerKey(sender));
		if (!original) {
			Tell(sender, "Nothing to restore: your gear is your own");
			SendList(sender);
			return;
		}
		PlayerBase player = GetCharacter(sender, "restore your gear");
		if (!player) return;

		ZBerryLoadoutJob job = ZBerryLoadoutJob.Start(player, original, NULL, sender);
		job.OnDone.Insert(OnRestored);
	}

	//Kept saved unless it's back on: a character that died meanwhile gets it with the next restore
	protected void OnRestored(ZBerryLoadoutJob job) {
		PlayerIdentity sender = GetJobIdentity(job);
		if (!job.IsDressed) {
			Tell(sender, "Gear not restored: your character is gone or busy, try again");
			return;
		}

		m_Originals.Remove(job.JobLoadout.Name);
		ZomberryBase.LogEvent( ZBerryEventType.LOADOUT, sender, NULL, "{actor} got their own gear back" );
		string told = "Your own gear is back";
		told += job.JobResult.DescribeLosses();
		Tell(sender, told);
		if (sender || !GetGame().IsMultiplayer()) SendList(sender);
	}

	//---------------------------------------------------------------- Other players

	static const int MAX_TARGETS = 100;

	//A player's current gear, sent back for the admin to look at, save or put on someone
	void LoadoutPlayerRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<int> data; //Session id
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_USE, "copy a player's gear")) return;

		int sessionId = data.param1;
		PlayerBase player = ZBGetPlayerById(sessionId);
		ZBerryLoadout loadout = new ZBerryLoadout("");
		string reason = "";
		if (!player) {
			reason = "They aren't online any more";
		} else if (!player.IsAlive()) {
			reason = "They're dead";
		} else if (!m_Auth.CanTarget(sender, ZBGetIdentity(player), false)) {
			reason = "Their group is protected from your rank";
		} else {
			string name = ZomberryStockFunctions.GetPlayerName(player) + "'s gear";
			loadout = ZBerryLoadoutKit.Capture(player, ZBerryLoadoutKit.CleanName(name));
			loadout.OwnerId = GetOwnerKey(ZBGetIdentity(player)); //Whose it is, for the log when it's applied or given
			loadout.OwnerName = ZomberryStockFunctions.GetPlayerName(player);
		}
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutPlayer, new Param3<int, string, ref ZBerryLoadout>(sessionId, reason, loadout), true, sender );
	}

	//On each player: what they wear is kept first (Undo gives it back), the admin's spawn rules apply. Harmful, so
	//rank protection applies
	void LoadoutGive( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4<bool, string, ref ZBerryLoadout, ref TIntArray> data; //Shared, name, the loadout (empty for a shared one), session ids
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_GIVE, "put a loadout on a player")) return;

		ZBerryLoadout loadout = data.param3;
		string source = "loadout ";
		if (data.param1) {
			loadout = m_Shared.Find(data.param2);
			source = "shared loadout ";
			if (!loadout) {
				Tell(sender, "Loadout not given: there's no shared loadout called " + data.param2);
				return;
			}
		} else if (!ZBerryLoadoutKit.Sanitize(loadout)) {
			Tell(sender, "Loadout not given: it's empty");
			return;
		} else if (loadout.OwnerId != "") {
			source = ""; //A player's current gear, named "<player>'s gear"
		}

		TIntArray targets = data.param4;
		if (!targets || targets.Count() == 0) return;
		for (int i = 0; i < targets.Count() && i < MAX_TARGETS; ++i) {
			PlayerBase player = GetTarget(sender, targets[i], "put " + loadout.Name + " on");
			if (!player) continue;

			PlayerIdentity identity = ZBGetIdentity(player);
			string problem = SaveOriginal(player, identity);
			if (problem != "") { //Their gear would be lost
				Tell(sender, "Loadout not put on " + ZomberryStockFunctions.GetPlayerName(player) + ": their gear can't be saved, " + problem);
				continue;
			}

			ZBerryLoadoutJob job = ZBerryLoadoutJob.Start(player, loadout, m_Auth, sender);
			job.LogNote = source;
			DescribeTarget(job, player);
			job.OnDone.Insert(OnGiven);
		}
	}

	//What they wore before the first loadout someone put on them, as it was: no spawn rules, it was theirs
	void LoadoutUndo( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<ref TIntArray> data; //Session ids
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_GIVE, "give a player their own gear back")) return;

		TIntArray targets = data.param1;
		if (!targets) return;
		for (int i = 0; i < targets.Count() && i < MAX_TARGETS; ++i) {
			PlayerBase player = GetTarget(sender, targets[i], "give their own gear back to");
			if (!player) continue;

			ZBerryLoadout original = m_Originals.Find(GetOwnerKey(ZBGetIdentity(player)));
			if (!original) {
				Tell(sender, "Nothing to undo for " + ZomberryStockFunctions.GetPlayerName(player) + ": their gear is their own");
				continue;
			}

			ZBerryLoadoutJob job = ZBerryLoadoutJob.Start(player, original, NULL, sender);
			DescribeTarget(job, player);
			job.OnDone.Insert(OnUndone);
		}
	}

	//A living player the admin may act on with something harmful. NULL (and the admin told) otherwise
	protected PlayerBase GetTarget(PlayerIdentity sender, int sessionId, string action) {
		PlayerBase player = ZBGetPlayerById(sessionId);
		if (!player) {
			Tell(sender, "Can't " + action + " a player who left");
			return NULL;
		}
		string name = ZomberryStockFunctions.GetPlayerName(player);
		if (!player.IsAlive()) {
			Tell(sender, "Can't " + action + " " + name + ": they're dead");
			return NULL;
		}

		PlayerIdentity identity = ZBGetIdentity(player);
		if (m_Auth.CanTarget(sender, identity, true)) return player;

		ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, identity, "{actor} tried to " + action + " {target}, whose group is protected from their rank" );
		Tell(sender, "Can't " + action + " " + name + ": their group is protected from your rank");
		return NULL;
	}

	protected static void DescribeTarget(ZBerryLoadoutJob job, PlayerBase player) {
		string id, name;
		ZomberryBase.DescribeIdentity(ZBGetIdentity(player), id, name);
		job.TargetId = id;
		job.TargetName = ZomberryStockFunctions.GetPlayerName(player);
	}

	protected void OnGiven(ZBerryLoadoutJob job) {
		PlayerIdentity sender = GetJobIdentity(job);
		string actorId, actorName;
		ZomberryBase.DescribeIdentity(sender, actorId, actorName);
		if (!job.IsDressed) {
			ZomberryBase.LogEventRaw( ZBerryEventType.LOADOUT, actorId, actorName, job.TargetId, job.TargetName, "{actor} took off {target}'s gear for " + job.LogNote + ZBerryMessages.ForLog(job.JobLoadout.Name) + ", but couldn't put it on", ZBerryEventFlags.HARMFUL );
			Tell(sender, "Loadout not put on " + job.TargetName + ": they left, died or are busy. Undo gives their gear back");
			return;
		}

		ZBerryLoadoutResult result = job.JobResult;
		string text = "{actor} put " + job.LogNote + ZBerryMessages.ForLog(job.JobLoadout.Name) + " on {target}";
		string told = "Put " + job.JobLoadout.Name + " on " + job.TargetName;
		if (result.RestrictedCount > 0) {
			text += ", without " + result.RestrictedCount + " items their spawn rules don't allow";
			told += ", without " + result.RestrictedCount + " items your spawn rules don't allow";
		}
		if (result.MissingCount > 0) text += ", without " + result.MissingCount + " items this server doesn't have";
		told += result.DescribeLosses();
		ZomberryBase.LogEventRaw( ZBerryEventType.LOADOUT, actorId, actorName, job.TargetId, job.TargetName, text, ZBerryEventFlags.HARMFUL );
		Tell(sender, told);
		SendListToTarget(job);
	}

	protected void OnUndone(ZBerryLoadoutJob job) {
		PlayerIdentity sender = GetJobIdentity(job);
		if (!job.IsDressed) {
			Tell(sender, job.TargetName + "'s gear not given back: they left, died or are busy, try again");
			return;
		}

		m_Originals.Remove(job.JobLoadout.Name);
		string actorId, actorName;
		ZomberryBase.DescribeIdentity(sender, actorId, actorName);
		ZomberryBase.LogEventRaw( ZBerryEventType.LOADOUT, actorId, actorName, job.TargetId, job.TargetName, "{actor} gave {target} their own gear back", ZBerryEventFlags.HARMFUL );
		string told = job.TargetName + " has their own gear back";
		told += job.JobResult.DescribeLosses();
		Tell(sender, told);
		SendListToTarget(job);
	}

	//A target who uses loadouts themselves: their Restore button follows their saved gear
	protected void SendListToTarget(ZBerryLoadoutJob job) {
		if (!GetGame().IsMultiplayer()) {
			SendList(NULL);
			return;
		}

		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man man : players) {
			PlayerIdentity identity = ZBGetIdentity(man);
			if (identity && identity.GetPlainId() == job.TargetId && m_Auth.HasPermission(identity, ZBerryPermissions.LOADOUTS_USE)) SendList(identity);
		}
	}

	//---------------------------------------------------------------- Deaths (Logs tab)

	//Why a death's gear can't be shown, for the admin
	static string DescribeNotKept() {
		return "This death's gear isn't kept: only the last " + ZBerryDeathLoadouts.MAX_PER_PLAYER + " deaths of each player are, and only until the server crashes or is killed";
	}

	protected ZBerryDeathLoadout FindDeath(int eventId) {
		if (!g_ZBerryDeathLoadouts) return NULL;
		return g_ZBerryDeathLoadouts.Find(eventId);
	}

	void DeathLoadoutRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<int> data; //Kill event id
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOGS_VIEW, "see the gear of a death")) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_USE, "see the gear of a death")) return;

		int eventId = data.param1;
		ZBerryDeathLoadout death = FindDeath(eventId);
		string reason = "";
		ZBerryLoadout loadout = new ZBerryLoadout("");
		int state = ZBerryVictimState.OFFLINE;
		if (death) {
			loadout = death.Loadout;
			state = ZBerryDeathLoadouts.GetVictimState(death);
		} else {
			reason = DescribeNotKept();
		}
		GetZBerryRPC().SendRPC( ZBerryRPCOp.DeathLoadout, new Param4<int, string, ref ZBerryLoadout, int>(eventId, reason, loadout, state), true, sender );
	}

	//On the admin, like any loadout: their own gear is saved first, their spawn rules apply
	void DeathLoadoutTry( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<int> data; //Kill event id
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_USE, "try on the gear of a death")) return;
		if (!CanRun(type, sender, ZBerryPermissions.SPAWN_INVENTORY, "try on the gear of a death")) return;

		ZBerryDeathLoadout death = FindDeath(data.param1);
		if (!death) {
			Tell(sender, "Not applied: " + DescribeNotKept());
			return;
		}
		StartTry(sender, death.Loadout, "the gear of ");
	}

	//Back on the victim, who must be online and alive. Rank protection as for harmful functions, the admin's spawn
	//rules apply
	void DeathLoadoutRestore( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<int> data; //Kill event id
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_RESTORE, "give a player the gear they died with")) return;

		ZBerryDeathLoadout death = FindDeath(data.param1);
		if (!death) {
			Tell(sender, "Not restored: " + DescribeNotKept());
			return;
		}

		ZBerryLoadout loadout = death.Loadout;
		PlayerBase victim = ZBerryDeathLoadouts.FindVictim(death);
		if (!victim) {
			Tell(sender, "Not restored: " + loadout.OwnerName + " isn't online");
			return;
		}
		if (!victim.IsAlive()) {
			Tell(sender, "Not restored: " + loadout.OwnerName + " is dead, try again once they've respawned");
			return;
		}
		PlayerIdentity victimIdentity = ZBGetIdentity(victim);
		if (!m_Auth.CanTarget(sender, victimIdentity, true)) {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, victimIdentity, "{actor} tried to give {target} the gear they died with, but their group is protected from their rank" );
			Tell(sender, "Not restored: " + loadout.OwnerName + "'s group is protected from your rank");
			return;
		}

		ZBerryLoadoutJob job = ZBerryLoadoutJob.Start(victim, loadout, m_Auth, sender);
		job.OnDone.Insert(OnDeathRestored);
	}

	protected void OnDeathRestored(ZBerryLoadoutJob job) {
		PlayerIdentity sender = GetJobIdentity(job);
		ZBerryLoadout loadout = job.JobLoadout;
		if (!job.IsDressed) {
			Tell(sender, "Not restored: " + loadout.OwnerName + " left, died or is busy, their gear was taken off");
			return;
		}

		string actorId, actorName;
		ZomberryBase.DescribeIdentity(sender, actorId, actorName);
		ZBerryLoadoutResult result = job.JobResult;
		string text = "{actor} gave {target} the gear they died with";
		string told = loadout.OwnerName + " got the gear they died with";
		if (result.RestrictedCount > 0) {
			text += ", without " + result.RestrictedCount + " items their spawn rules don't allow";
			told += ", without " + result.RestrictedCount + " items your spawn rules don't allow";
		}
		if (result.MissingCount > 0) text += ", without " + result.MissingCount + " items this server doesn't have";
		told += result.DescribeLosses();
		ZomberryBase.LogEventRaw( ZBerryEventType.LOADOUT, actorId, actorName, loadout.OwnerId, loadout.OwnerName, text, ZBerryEventFlags.HARMFUL );
		Tell(sender, told);
	}

	//Replaces the shared loadout of that name, if the admin may change it
	void LoadoutShare( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<ref ZBerryLoadout> data;
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_SHARE, "share a loadout")) return;

		ZBerryLoadout loadout = data.param1;
		if (!ZBerryLoadoutKit.Sanitize(loadout) || loadout.Name == "") {
			Tell(sender, "Loadout not shared: it's empty or has no name");
			return;
		}

		ZBerryLoadout previous = m_Shared.Find(loadout.Name);
		if (previous && !CanEdit(sender, previous)) {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to replace shared loadout " + ZBerryMessages.ForLog(previous.Name) + " by " + previous.OwnerName + ", whose rank is the same or higher" );
			Tell(sender, "Loadout not shared: " + previous.Name + " is " + previous.OwnerName + "'s, rename yours");
			return;
		}

		if (previous) loadout.Name = previous.Name;
		loadout.OwnerId = GetOwnerKey(sender);
		loadout.OwnerName = GetName(sender);
		string problem = m_Shared.Put(loadout, MAX_SHARED);
		if (problem != "") {
			Tell(sender, "Loadout not shared: " + problem);
			return;
		}

		string text = "{actor} shared loadout " + ZBerryMessages.ForLog(loadout.Name);
		if (previous) text = "{actor} replaced shared loadout " + ZBerryMessages.ForLog(loadout.Name) + " by " + previous.OwnerName;
		ZomberryBase.LogEvent( ZBerryEventType.LOADOUT, sender, NULL, text );
		Tell(sender, "Shared " + loadout.Name);
		SendListToAll();
	}

	void LoadoutDelete( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<string> data; //Name
		if (!ctx.Read(data)) return;
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_USE, "delete a shared loadout")) return;

		ZBerryLoadout loadout = m_Shared.Find(data.param1);
		if (!loadout) {
			Tell(sender, "Loadout not deleted: there's no shared loadout called " + data.param1);
			return;
		}
		if (!CanEdit(sender, loadout)) {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to delete shared loadout " + ZBerryMessages.ForLog(loadout.Name) + " by " + loadout.OwnerName + " without the rights to" );
			Tell(sender, "Loadout not deleted: you may not change " + loadout.OwnerName + "'s loadouts");
			return;
		}

		string name = loadout.Name;
		string owner = loadout.OwnerName;
		string problem = m_Shared.Remove(name);
		if (problem != "") {
			Tell(sender, "Loadout not deleted: " + problem);
			return;
		}

		ZomberryBase.LogEvent( ZBerryEventType.LOADOUT, sender, NULL, "{actor} deleted shared loadout " + ZBerryMessages.ForLog(name) + " by " + owner );
		Tell(sender, "Deleted " + name);
		SendListToAll();
	}
};
