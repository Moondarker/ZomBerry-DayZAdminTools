static string g_zbryVer = "0.8.0";

//Wires ZomBerry together on both sides (server: MissionServer, client and singleplayer: MissionGameplay)
//and keeps the shared config/logger/keybinds. The actual work lives in core/: server auth and permissions, client auth, request handlers
class ZomberryBase {
	ref ZomberryStockFunctions m_ZomberryStockFunctions;
	protected ref ZBerryAuth m_Auth;
	protected ref ZBerryServerRPC m_ServerRPC;
	protected ref ZBerryBans m_Bans;
	protected ref ZBerryStreamServer m_Streams;

	static ref ZomberryConfig m_ZomberryConfig;
	static ref ZomberryLogger m_ZomberryLogger;
	static ref ZomberryKeybinds m_ZomberryKeybinds;

	void ZomberryBase() {
		m_ZomberryStockFunctions = new ZomberryStockFunctions;

		if (!GetConfig().IsDefaultIO()) {
			GetLogger().SwitchToCustomIO();
		}

		//Server side only: in singleplayer the request handlers run locally, on a multiplayer client they'd be dead weight
		if (GetGame().IsServer() || !GetGame().IsMultiplayer()) {
			m_Auth = new ZBerryAuth;
			m_Bans = new ZBerryBans;
			m_ServerRPC = new ZBerryServerRPC(m_Auth, m_Bans);
			m_Streams = new ZBerryStreamServer(m_Auth);
			GetZBerryEventLog().SetAuth(m_Auth);
			GetZomberryPermAPI().SetAuth(m_Auth);
			m_ZomberryStockFunctions.Init();
		}

		if (GetConfig().GetDebugLvl() >= 2) {
			GetZomberryCmdAPI().Debug();
		}
	}

	static ref ZomberryConfig GetConfig() {
		if ( !m_ZomberryConfig ) {
			m_ZomberryConfig = new ZomberryConfig;
		}

		return m_ZomberryConfig;
	}

	static ref ZomberryLogger GetLogger() {
		if ( !m_ZomberryLogger ) {
			m_ZomberryLogger = new ZomberryLogger;
		}

		return m_ZomberryLogger;
	}

	static ref ZomberryKeybinds GetKeyBindsMgr() {
		if ( !m_ZomberryKeybinds ) {
			m_ZomberryKeybinds = new ZomberryKeybinds;
		}

		return m_ZomberryKeybinds;
	}

	//A plain line for ZomBerry's log file. Admin actions, kicks, bans, kills and refusals go through LogEvent instead
	static void Log( string module, string toLog ) {
		GetLogger().Log( module, toLog );
	}

	/**
	\brief Log an event: written to ZomBerry's log file (except logins and logouts) and kept in the in-game log
		\param type \p int ZBerryEventType
		\param actor \p PlayerIdentity who did it (NULL: nobody, or singleplayer)
		\param target \p PlayerIdentity whom it was done to (NULL: nobody)
		\param text \p string with {actor} and {target} where the players go, e.g. "{actor} kicked {target}: spam"
		\param flags \p int ZBerryEventFlags
	*/
	static void LogEvent( int type, PlayerIdentity actor, PlayerIdentity target, string text, int flags = 0 ) {
		string actorId, actorName, targetId, targetName;
		DescribeIdentity(actor, actorId, actorName);
		DescribeIdentity(target, targetId, targetName);
		LogEventRaw(type, actorId, actorName, targetId, targetName, text, flags);
	}

	//LogEvent for players who may be offline or have no identity: Steam64 id ("" for none) and name
	static void LogEventRaw( int type, string actorId, string actorName, string targetId, string targetName, string text, int flags = 0 ) {
		if (!GetGame() || !GetGame().IsServer()) return;

		ZBerryLogEvent logEvent = GetZBerryEventLog().Add(type, actorId, actorName, targetId, targetName, text, flags);
		if (ZBerryEventType.IsInGameOnly(type)) return;

		string line = logEvent.Format();
		if (type == ZBerryEventType.REFUSED) line = "WARN: " + line;
		GetLogger().Log( ZBerryEventType.GetModule(type), line );
	}

	protected static void DescribeIdentity(PlayerIdentity identity, out string id, out string name) {
		id = "";
		name = "";
		if (!identity) {
			if (!GetGame().IsMultiplayer()) name = "Singleplayer";
			return;
		}
		id = identity.GetPlainId();
		name = identity.GetName();
	}

	static void DebugLog( int dLvl, string module, string toLog ) {
		GetLogger().DebugLog( dLvl, module, toLog );
	}

	void OnClientReady() {
		GetZBerryClientAuth().RequestAuth();
	}

	void OnServerReady() {
		m_Auth.LoadAdmins();
		m_Bans.Load(GetConfig().GetConfigDir() + ZBerryBans.CONFIG_FILE);
	}

	//Server only, NULL on clients
	ZBerryBans GetBans() {
		return m_Bans;
	}

	//Server only, NULL on clients
	ZBerryStreamServer GetStreams() {
		return m_Streams;
	}

	string GetRemoteVersion() {
		return GetZBerryClientAuth().GetRemoteVersion();
	}

	//Client and singleplayer: what the server said. Server: is this identity an admin
	bool IsAdmin(PlayerIdentity plyIdent = NULL) {
		if (GetGame().IsClient() || !GetGame().IsMultiplayer()) return GetZBerryClientAuth().IsAdmin();

		return m_Auth && m_Auth.IsAdmin(plyIdent);
	}
};

modded class MissionServer {
	ref ZomberryBase m_ZomberryBase;

	void MissionServer() {
		ZomberryBase.Log( "ZomBerry", "Starting Server side " + ZBerryVersion.GetFullVersion() );
	}

	private ref ZomberryBase GetZomberryBase() {
		if ( !m_ZomberryBase ) {
			m_ZomberryBase = new ZomberryBase;
		}

		return m_ZomberryBase;
	}

	override void OnInit() {
		super.OnInit();

		GetZomberryBase().OnServerReady();
	}

	//Before the character is loaded: banned players never get into the world
	override void OnClientPrepareEvent(PlayerIdentity identity, out bool useDB, out vector pos, out float yaw, out int preloadTimeout) {
		super.OnClientPrepareEvent(identity, useDB, pos, yaw, preloadTimeout);

		ZBerryBanEntry ban = FindZBerryBan(identity);
		if (!ban) return;

		ZomberryBase.LogEvent( ZBerryEventType.JOIN_REFUSED, NULL, identity, "Banned player {target} tried to join, refused: " + ban.Reason + ", " + ban.DescribeUntil() );
		GetGame().DisconnectPlayer(identity, identity.GetId());
	}

	//In case the engine still let the character in
	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity) {
		super.InvokeOnConnect(player, identity);
		GetZBerryEventLog().OnPlayerConnected(identity);

		ZBerryBanEntry ban = FindZBerryBan(identity);
		if (ban) m_ZomberryBase.GetBans().KickPlayer(player, "You are banned: " + ban.Reason + ", " + ban.DescribeUntil());

		//Frozen players stay frozen across relogs; a moment later, so their client is ready for the feature
		if (m_ZomberryBase && m_ZomberryBase.m_ZomberryStockFunctions) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(m_ZomberryBase.m_ZomberryStockFunctions.ReapplyFreeze, 5000, false, player);
	}

	override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed) {
		GetZBerryEventLog().OnPlayerDisconnected(identity); //Before super: the identity may be gone after it
		if (g_ZBerryClientFeatures) g_ZBerryClientFeatures.OnPlayerDisconnected(identity);
		super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
	}

	protected ZBerryBanEntry FindZBerryBan(PlayerIdentity identity) {
		if (!m_ZomberryBase || !m_ZomberryBase.GetBans() || !identity) return NULL;

		return m_ZomberryBase.GetBans().FindActiveFor(identity);
	}
};

modded class MissionGameplay {
	bool m_plyWarned = false;
	ref ZomberryBase m_ZomberryBase;
	ref ZomberryMenu m_ZomberryMenu;
	ref ZBerryTokenDialog m_ZBerryTokenDialog;
	ref ZBerryESPOverlay m_ZBerryESP;

	void MissionGameplay() {
		m_ZomberryBase = new ZomberryBase;
		GetZBerryClientAuth();
		GetZBerryStreams();
		GetZBerryClientFeatures();
		GetZBerryEventLog();
		m_ZBerryESP = new ZBerryESPOverlay;
		GetZBerrySpectate(); //Registers its client features: spectating, and reporting the camera mode while watched

		ZomberryBase.Log( "ZomBerry", "Starting Client side " + ZBerryVersion.GetFullVersion() );
	}

	void ~MissionGameplay() {

		if (m_ZomberryMenu) delete m_ZomberryMenu;
		if (m_ZomberryBase) delete m_ZomberryBase;
	}

	private ref ZomberryMenu GetZomberryMenu() {
		if ( !m_ZomberryMenu ) {
			m_ZomberryMenu = new ZomberryMenu;
			m_ZomberryMenu.Init();
			GetZBerryRPC().AddRPC( ZBerryRPCOp.SyncFunctions, "SyncFunctions", m_ZomberryMenu, ZBerrySPExec.Client );
			GetZBerryRPC().AddRPC( ZBerryRPCOp.BansData, "BansData", m_ZomberryMenu, ZBerrySPExec.Client );
			GetZBerryClientAuth().SetMenu(m_ZomberryMenu);
		}

		return m_ZomberryMenu;
	}

	//The menu key, Esc and the menu's close button. Callers check GetCloseClearance where a stray key could close it
	void CloseZomberryMenu() {
		if (!m_ZomberryMenu || !m_ZomberryMenu.GetLayoutRoot() || !m_ZomberryMenu.GetLayoutRoot().IsVisible()) return;

		GetUIManager().HideScriptedMenu( m_ZomberryMenu );
		PlayerControlEnable(false);
	}

	private ZBerryTokenDialog GetTokenDialog() {
		if ( !m_ZBerryTokenDialog ) {
			m_ZBerryTokenDialog = new ZBerryTokenDialog;
			m_ZBerryTokenDialog.Init();
		}

		return m_ZBerryTokenDialog;
	}

	private bool IsTokenDialogOpen() {
		return m_ZBerryTokenDialog && m_ZBerryTokenDialog.GetLayoutRoot() && m_ZBerryTokenDialog.GetLayoutRoot().IsVisible();
	}

	void CloseTokenDialog() {
		if (!IsTokenDialogOpen()) return;

		GetUIManager().HideScriptedMenu( m_ZBerryTokenDialog );
		PlayerControlEnable(false);
	}

	override void OnMissionStart() {
		super.OnMissionStart();

		m_ZomberryBase.OnClientReady();
	}

	override void OnKeyPress(int key) {
		super.OnKeyPress(key);

		if (GetZomberryMenu().GetLayoutRoot().IsVisible()) {
			GetZomberryMenu().OnBindKeyPress(key);
		} else if (!GetUIManager().GetMenu() && m_ZomberryBase.IsAdmin()) { //Not while typing in chat or the token dialog, nor in the inventory or pause menu
			ZomberryBase.GetKeyBindsMgr().OnKeyPress(key);
		}
	}

	void DisplayMessage(string msg) {
		GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "", msg, ""));
	}

	override void OnUpdate(float timeslice) {
		super.OnUpdate(timeslice);
		if (m_ZBerryESP) m_ZBerryESP.Update();

		if (GetUApi().GetInputByName("UAZBerryOpenAdminMenu").LocalPress() && !IsTokenDialogOpen()) { //The token may contain the menu key's letter
			ZBerryClientAuth auth = GetZBerryClientAuth();
			string r_zbryVer = auth.GetRemoteVersion();
			UIScriptedMenu menu = GetUIManager().GetMenu();

			if (!menu && !GetZomberryMenu().GetLayoutRoot().IsVisible() && auth.IsAdmin()) {
				GetUIManager().ShowScriptedMenu( GetZomberryMenu(), NULL );
				PlayerControlDisable(INPUT_EXCLUDE_ALL);
			} else if (GetZomberryMenu().GetLayoutRoot().IsVisible() && GetZomberryMenu().GetCloseClearance()) {
				CloseZomberryMenu();
			} else if (!ZBerryVersion.IsCompatible(r_zbryVer, g_zbryVer)) {
				DisplayMessage("[ZomBerry]: Admin auth succeeded, but clientside was disabled due to version mismatch. C: " + g_zbryVer + ", S: " + r_zbryVer);
			} else if (!menu && auth.IsSetupPending()) {
				GetUIManager().ShowScriptedMenu( GetTokenDialog(), NULL );
				PlayerControlDisable(INPUT_EXCLUDE_ALL);
			}
			if (ZBerryVersion.IsNewer(g_zbryVer, r_zbryVer) && !m_plyWarned) {
				DisplayMessage("[ZomBerry] INFO: Don't forget to update your server to v" + g_zbryVer + "+ to get latest features!");
				DisplayMessage("[ZomBerry] INFO: Server is currently running on: v" + r_zbryVer);
				m_plyWarned = true;
			}
		}

		if (GetUApi().GetInputByName("UAUIBack").LocalPress()) {
			if (GetZomberryMenu().GetLayoutRoot().IsVisible() && GetZomberryMenu().CloseDialogs()) {
				//Only the dialog
			} else if (GetZomberryMenu().GetLayoutRoot().IsVisible() && GetZomberryMenu().GetCloseClearance()) {
				CloseZomberryMenu();
			}
			CloseTokenDialog();
		}
	}
};
