static ref ZomberryPermAPI g_ZomberryPermAPI;

/**
\brief Get shared ZomberryPermAPI instance: ZomBerry's admin groups and permissions, for other mods and missions
	\return \p ZomberryPermAPI class
	@code
		//Any side, at startup: the node shows up in the Permissions tab, where groups can be given it
		GetZomberryPermAPI().RegisterNode("mymod.vip.teleport");

		//Server: check a request from a player
		if (!GetZomberryPermAPI().HasPermission(sender, "mymod.vip.teleport")) return;

		//Client: show your own UI only to those who may use it, and update it when their rights change
		myButton.Show(GetZomberryPermAPI().HasLocalPermission("mymod.vip.teleport"));
		GetZomberryPermAPI().OnLocalPermissionsChanged.Insert(MyModUpdateButtons);
	@endcode
*/
static ZomberryPermAPI GetZomberryPermAPI() {
	if ( !g_ZomberryPermAPI ) {
		g_ZomberryPermAPI = new ZomberryPermAPI;
	}

	return g_ZomberryPermAPI;
}

//Nodes are dot-separated and lower case ("mymod.vip.teleport"). Groups grant them with patterns: "mymod.*" grants every
//node of a mod, "*" everything, "-mymod.vip.*" denies. Server checks are the ones that count, client checks only shape UI
class ZomberryPermAPI {
	protected ref TStringArray m_Nodes = new TStringArray;
	protected ZBerryAuth m_Auth; //Server and singleplayer, owned by ZomberryBase

	//Called by ZomberryBase once the server side exists
	void SetAuth(ZBerryAuth auth) {
		m_Auth = auth;
	}

	//Lower case letters, digits, "_" and "-" between dots, at least two parts: "mymod.feature"
	static bool IsValidNode(string node) {
		if (node == "" || node.Get(0) == "." || node.Get(node.Length() - 1) == "." || node.IndexOf(".") == -1) return false;
		if (node.Contains("..") || node.Get(0) == "-") return false;

		string allowed = "abcdefghijklmnopqrstuvwxyz0123456789_-.";
		for (int i = 0; i < node.Length(); ++i) {
			if (allowed.IndexOf(node.Get(i)) == -1) return false;
		}
		return true;
	}

	/**
	\brief Makes a node known, so the Permissions tab lists it and groups can be given it. Call it on the server
		(the tab gets its list from there), at startup. Registering twice is harmless
		\param node \p string like "mymod.vip.teleport", lower case
		\return \p bool false (and a log line) when the node isn't valid or starts with ZomBerry's own "zomberry."
	*/
	bool RegisterNode(string node) {
		node.ToLower();
		if (!IsValidNode(node)) {
			ZomberryBase.Log( "ZomBerryPermAPI", "WARN: Can't register permission node '" + node + "': use lower case letters, digits, _ and - between dots, like mymod.feature" );
			return false;
		}
		if (node.IndexOf("zomberry.") == 0) {
			ZomberryBase.Log( "ZomBerryPermAPI", "WARN: Can't register permission node '" + node + "': zomberry.* is ZomBerry's own, use your mod's name as the first part" );
			return false;
		}

		if (m_Nodes.Find(node) == -1) {
			m_Nodes.Insert(node);
			ZomberryBase.Log( "ZomBerryPermAPI", "INFO: Registered permission node " + node );
		}
		return true;
	}

	//Nodes registered by mods and missions, in registration order
	TStringArray GetRegisteredNodes() {
		TStringArray nodes = new TStringArray;
		nodes.InsertAll(m_Nodes);
		return nodes;
	}

	//Server side (and singleplayer, where the local player may do everything). False on clients: use the Local methods there

	//Is this player in one of ZomBerry's admin groups
	bool IsAdmin(PlayerIdentity identity) {
		return m_Auth && m_Auth.IsAdmin(identity);
	}

	//Does this player's group grant the node
	bool HasPermission(PlayerIdentity identity, string node) {
		node.ToLower();
		return m_Auth && m_Auth.HasPermission(identity, node);
	}

	//Group level, 0-100 (100: superadmin), -1 for players who aren't admins
	int GetLevel(PlayerIdentity identity) {
		if (!m_Auth) return -1;
		if (!GetGame().IsMultiplayer()) return ZBerryPermGroup.SUPERADMIN_LEVEL;

		return m_Auth.GetLevel(identity);
	}

	//Group name, "" for players who aren't admins
	string GetGroup(PlayerIdentity identity) {
		if (!m_Auth) return "";

		return m_Auth.GetGroupName(identity);
	}

	//ZomBerry's rank rule (used for kicks and bans): admins only act on admins ranked below them, superadmins on everyone.
	//Players who aren't admins can always be acted on, and everyone may act on themselves
	bool CanActOn(PlayerIdentity actor, PlayerIdentity target) {
		if (!m_Auth || !actor || !target) return false;
		if (actor.GetPlainId() == target.GetPlainId()) return true;

		return ZBerryAuth.CanActOnLevel(GetLevel(actor), GetLevel(target));
	}

	//ZomBerry's rule for functions: may actor run a (harmful) function on target? Groups shield their members from
	//lower-ranked or same-level admins with zomberry.protect.* nodes, superadmins ignore those. Everyone may target themselves
	bool CanTarget(PlayerIdentity actor, PlayerIdentity target, bool harmful) {
		if (!m_Auth || !actor || !target) return false;

		return m_Auth.CanTarget(actor, target, harmful);
	}

	//Do this player's spawn rules allow the class
	bool CanSpawn(PlayerIdentity identity, string className) {
		return m_Auth && m_Auth.CanSpawn(identity, className);
	}

	//Client side (and singleplayer): what the server told this client. Only for showing or hiding UI, the server checks again

	//Is the local player a ZomBerry admin
	bool IsLocalAdmin() {
		return GetZBerryClientAuth().IsAdmin();
	}

	//Does the local player's group grant the node
	bool HasLocalPermission(string node) {
		node.ToLower();
		return GetZBerryClientAuth().Has(node);
	}

	//Called without arguments whenever the server sends the local player new rights (joining, group or permission changes)
	ref ScriptInvoker OnLocalPermissionsChanged = new ScriptInvoker();
};
