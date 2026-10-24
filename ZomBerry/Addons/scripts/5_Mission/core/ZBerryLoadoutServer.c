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
		foreach (ZBerryLoadout loadout : m_Shared.GetAll()) {
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
		if (!CanRun(type, sender, ZBerryPermissions.LOADOUTS_USE, "try on a loadout")) return;
		if (!CanRun(type, sender, ZBerryPermissions.SPAWN_INVENTORY, "try on a loadout")) return;

		ZBerryLoadout loadout = data.param3;
		string source = "their loadout ";
		if (data.param1) {
			loadout = m_Shared.Find(data.param2);
			source = "shared loadout ";
			if (!loadout) {
				Tell(sender, "Loadout not tried on: there's no shared loadout called " + data.param2);
				return;
			}
		} else if (!ZBerryLoadoutKit.Sanitize(loadout)) {
			Tell(sender, "Loadout not tried on: it's empty");
			return;
		}

		PlayerBase player = GetCharacter(sender, "try on a loadout");
		if (!player) return;

		if (!HasOriginal(sender)) {
			ZBerryLoadout original = ZBerryLoadoutKit.Capture(player, GetOwnerKey(sender));
			original.OwnerId = GetOwnerKey(sender);
			original.OwnerName = GetName(sender);
			string problem = m_Originals.Put(original, MAX_ORIGINALS);
			if (problem != "") { //Their gear would be lost
				Tell(sender, "Loadout not tried on: your gear can't be saved, " + problem);
				return;
			}
		}

		ZBerryLoadoutJob job = ZBerryLoadoutJob.Start(player, loadout, m_Auth, sender);
		job.Note = source;
		job.OnDone.Insert(OnTried);
		SendList(sender); //Restore is possible from now on
	}

	protected void OnTried(ZBerryLoadoutJob job) {
		string source = job.Note;
		PlayerIdentity sender = GetJobIdentity(job);
		if (!job.Dressed) {
			ZomberryBase.LogEvent( ZBerryEventType.LOADOUT, sender, NULL, "{actor} took off their gear for " + source + ZBerryMessages.ForLog(job.Loadout.Name) + ", but couldn't put it on" );
			Tell(sender, "Loadout not put on: your character is gone or busy, restore your gear");
			return;
		}

		ZBerryLoadoutResult result = job.Result;
		string text = "{actor} tried on " + source + ZBerryMessages.ForLog(job.Loadout.Name);
		string told = "Tried on " + job.Loadout.Name;
		if (result.Restricted > 0) {
			text += ", without " + result.Restricted + " items their spawn rules don't allow";
			told += ", without " + result.Restricted + " items your spawn rules don't allow";
		}
		if (result.Missing > 0) text += ", without " + result.Missing + " items this server doesn't have";
		told += result.DescribeLosses();
		ZomberryBase.LogEvent( ZBerryEventType.LOADOUT, sender, NULL, text );
		Tell(sender, told);
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
		if (!job.Dressed) {
			Tell(sender, "Gear not restored: your character is gone or busy, try again");
			return;
		}

		m_Originals.Remove(job.Loadout.Name);
		ZomberryBase.LogEvent( ZBerryEventType.LOADOUT, sender, NULL, "{actor} got their own gear back" );
		string told = "Your own gear is back";
		told += job.Result.DescribeLosses();
		Tell(sender, told);
		if (sender || !GetGame().IsMultiplayer()) SendList(sender);
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
