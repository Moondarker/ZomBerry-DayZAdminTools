//Client side of admin rights: what the server said this player may do (AuthState).
//Only used to shape the UI, the server checks every request again
class ZBerryClientAuth {
	protected int m_State = ZBerryAuthState.NONE;
	protected bool m_StateKnown = false; //First answer since joining
	protected string m_RemoteVersion = g_zbryVer;
	protected ref TStringArray m_Permissions = new TStringArray;
	protected ref TStringArray m_SpawnRules = new TStringArray;
	protected ZomberryMenu m_Menu; //Told about changes while it exists

	void ZBerryClientAuth() {
		GetZBerryRPC().AddRPC( ZBerryRPCOp.AuthState, "AuthState", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.PermsData, "PermsData", this, ZBerrySPExec.Client );
	}

	void SetMenu(ZomberryMenu menu) {
		m_Menu = menu;
	}

	void RequestAuth() {
		if (!GetGame().IsMultiplayer()) { //Singleplayer: the local player may do everything
			m_State = ZBerryAuthState.ADMIN;
			m_Permissions = {"*"};
			m_SpawnRules = {"*"};
			return;
		}

		ZomberryBase.Log( "ZomBerryDbg", "Auth Request sent" );
		GetZBerryRPC().SendRPC( ZBerryRPCOp.AdminAuth, new Param2< bool, string >( true, g_zbryVer ), true, NULL );
	}

	void SubmitToken(string token) {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.SetupToken, new Param1<string>( token ), true, NULL );
	}

	bool IsAdmin() {
		return m_State == ZBerryAuthState.ADMIN;
	}

	bool IsSetupPending() {
		return m_State == ZBerryAuthState.SETUP_PENDING;
	}

	string GetRemoteVersion() {
		return m_RemoteVersion;
	}

	bool Has(string node) {
		return IsAdmin() && ZBerryPermissions.IsGranted(m_Permissions, node);
	}

	bool HasAny(TStringArray nodes) {
		foreach (string node : nodes) {
			if (Has(node)) return true;
		}
		return false;
	}

	bool CanSpawn(string className) {
		return IsAdmin() && ZBerryPermissions.IsSpawnAllowed(m_SpawnRules, className);
	}

	//Managing permissions only makes sense on a server
	bool CanManagePermissions() {
		return GetGame().IsMultiplayer() && Has(ZBerryPermissions.PERMISSIONS_MANAGE);
	}

	void AuthState( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4<string, int, ref TStringArray, ref TStringArray> state; //Server version, state, permissions, spawn rules
		if (type != ZBerryCallType.Client) return;
		if ( !ctx.Read( state ) ) {
			ZomberryBase.Log( "ZomBerryAuth", "ERROR: Can't read the server's answer, version mismatch?" );
			return;
		}

		m_RemoteVersion = state.param1;
		if (!ZBerryVersion.IsCompatible(m_RemoteVersion, g_zbryVer)) {
			ZomberryBase.Log( "ZomBerryAuth", "ERROR: ZomBerry version mismatch! C: v" + g_zbryVer + ", S: v" + m_RemoteVersion + ", clientside won't start!" );
			m_State = ZBerryAuthState.NONE;
			return;
		}
		if (m_RemoteVersion != g_zbryVer) ZomberryBase.Log( "ZomBerryAuth", "WARN: ZomBerry version mismatch! C: v" + g_zbryVer + ", S: v" + m_RemoteVersion );

		bool wasAdmin = IsAdmin();
		bool firstState = !m_StateKnown;
		m_StateKnown = true;
		m_State = state.param2;
		m_Permissions = state.param3;
		m_SpawnRules = state.param4;
		if (!m_Permissions) m_Permissions = new TStringArray;
		if (!m_SpawnRules) m_SpawnRules = new TStringArray;

		ZomberryBase.Log( "ZomBerryDbg", "Auth state " + m_State + " with " + m_Permissions.Count() + " permission patterns" );
		if (IsAdmin() && !wasAdmin && !firstState) {
			ZBerryUIUtils.Message("Admin rights granted, press the admin menu key to open ZomBerry");
		}

		if (m_Menu) m_Menu.OnPermissionsChanged();
	}

	void PermsData( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<ref ZBerryPermsSnapshot> data;
		if (type != ZBerryCallType.Client) return;
		if ( !ctx.Read( data ) || !data.param1 ) return;

		if (m_Menu) m_Menu.OnPermsData(data.param1);
	}
};

static ref ZBerryClientAuth g_ZBerryClientAuth;

//Client and singleplayer only
static ZBerryClientAuth GetZBerryClientAuth() {
	if (!g_ZBerryClientAuth) g_ZBerryClientAuth = new ZBerryClientAuth();

	return g_ZBerryClientAuth;
}
