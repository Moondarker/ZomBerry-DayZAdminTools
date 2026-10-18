//Who is an admin, and the version handshake between client and server (AdminAuth RPC).
//Kept on its own so a future permissions system can replace it without touching the request handlers
class ZBerryAuth {
	protected ref TStringArray m_AdminList = new TStringArray; //Server: IDs from admins.cfg
	protected bool m_IsAdmin = false; //Client: what the server answered
	protected string m_RemoteVersion = g_zbryVer; //Client: the server's version

	void ZBerryAuth() {
		GetZBerryRPC().AddRPC( ZBerryRPCOp.AdminAuth, "AdminAuth", this, ZBerrySPExec.Client );
	}

	static bool EveryoneIsAdmin() {
		string value;
		return GetCLIParam("zbryGiveAdminRightsToEveryone", value) && value == "true";
	}

	void LoadAdmins() {
		m_AdminList = ZomberryBase.GetConfig().ConfigureAdmins();
	}

	void RequestAuth() {
		ZomberryBase.Log( "ZomBerryDbg", "Auth Request sent" );

		GetZBerryRPC().SendRPC( ZBerryRPCOp.AdminAuth, new Param2< bool, string >( true, g_zbryVer ), true, NULL );
	}

	string GetRemoteVersion() {
		return m_RemoteVersion;
	}

	//Client and singleplayer: answer from the server. Server: is this identity an admin
	bool IsAdmin(PlayerIdentity plyIdent = NULL) {
		if (!GetGame().IsMultiplayer() || GetGame().IsClient()) return m_IsAdmin;
		if ((m_AdminList.Find(plyIdent.GetId()) != -1) || (m_AdminList.Find(plyIdent.GetPlainId()) != -1)) return true;

		return EveryoneIsAdmin() || ZBerryInstallMode.IsActive();
	}

	void AdminAuth( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2< bool, string > authInfo; //direction (true = to server), version (only in client dir)
		if ( !ctx.Read( authInfo ) ) {
			ZomberryBase.Log( "ZomBerryAT", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") - Auth error, please update client to latest version (v" + g_zbryVer + "+)!");
			return;
		}

		if ( type == ZBerryCallType.Server ) {
			if (IsAdmin(sender)) {
				GetZBerryRPC().SendRPC( ZBerryRPCOp.AdminAuth, new Param2< bool, string >( false, g_zbryVer ), true, sender );
				ZomberryBase.Log( "ZomBerryDbg", "Auth respond to admin " + sender.GetName() + " (" + sender.GetId() + ")");
				if (authInfo.param2 != g_zbryVer) {
					ZomberryBase.Log( "ZomBerryAT", "WARN: Admin " + sender.GetName() + " (" + sender.GetId() + ") ZomBerry version mismatch! S: v" + g_zbryVer + ", C: v" + authInfo.param2 + ", clientside may not start!");
				}
			} else {
				if (m_AdminList.Count() <= 1) GetZBerryRPC().SendRPC( ZBerryRPCOp.AdminAuth, new Param2< bool, string >( false, g_zbryVer + "CFGFailed" ), true, sender );
				ZomberryBase.Log( "ZomBerryDbg", "Auth Request ignored (not an admin) " + sender.GetName() + " (" + sender.GetId() + ")" );
			}
		} else {
			if (!authInfo.param1 || GetGame().IsMultiplayer()) {
				ZomberryBase.Log( "ZomBerryDbg", "Auth Respond received" );
				m_RemoteVersion = authInfo.param2;
				if (!ZBerryVersion.IsCompatible(authInfo.param2, g_zbryVer)) {
					ZomberryBase.Log( "ZomBerryAT", "ERROR: ZomBerry version mismatch! C: v" + g_zbryVer + ", S: v" + authInfo.param2 + ", clientside won't start!");
				} else {
					if (authInfo.param2 != g_zbryVer) ZomberryBase.Log( "ZomBerryAT", "WARN: ZomBerry version mismatch! C: v" + g_zbryVer + ", S: v" + authInfo.param2);
					if (!m_RemoteVersion.Contains("CFGFailed")) m_IsAdmin = true;
				}
			} else {
				ZomberryBase.Log( "ZomBerryDbg", "Auth ignored, singleplayer" );
				m_IsAdmin = true;
			}
			ZomberryBase.GetConfig();
		}
	}
};
