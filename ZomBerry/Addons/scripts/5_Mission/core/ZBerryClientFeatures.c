//A feature switched on for one client, as the server remembers it
class ZBerryActiveFeature {
	PlayerIdentity Identity; //Weak: gone when they disconnect. NULL in singleplayer
	string Name;
	string RequiredNode; //Switched off again when the player loses it ("" for none)

	void ZBerryActiveFeature(PlayerIdentity identity, string name, string requiredNode) {
		Identity = identity;
		Name = name;
		RequiredNode = requiredNode;
	}
};

//Client-side functions (#38): the server decides, the client does. Server code (usually a command, after its permission
//and rank checks) switches a named feature on one client; that client runs the handler registered for the name
//(void Handler(bool enabled, TIntArray args)). Use it through GetZomberryCmdAPI().SetClientFeature/RegisterClientFeature
class ZBerryClientFeatures {
	//Server side
	protected ref array<ref ZBerryActiveFeature> m_Active = new array<ref ZBerryActiveFeature>;
	ref ScriptInvoker OnServerFeatureChanged = new ScriptInvoker(); //(PlayerIdentity identity, string name, bool enabled)

	//Client side
	protected ref map<string, ref ZBerryRPCHandler> m_Handlers = new map<string, ref ZBerryRPCHandler>;
	protected ref TStringArray m_Enabled = new TStringArray;

	void ZBerryClientFeatures() {
		GetZBerryRPC().AddRPC( ZBerryRPCOp.ClientFeature, "OnClientFeature", this, ZBerrySPExec.Client );
	}

	protected int FindActive(PlayerIdentity identity, string name) {
		for (int i = 0; i < m_Active.Count(); ++i) {
			if (m_Active[i].Identity == identity && m_Active[i].Name == name) return i;
		}
		return -1;
	}

	//Server: switch a feature on or off for one client. args reach the client's handler (NULL: none)
	void Set(PlayerIdentity identity, string name, bool enabled, TIntArray args = NULL, string requiredNode = "") {
		if (GetGame().IsMultiplayer() && !identity) return;
		ForgetDisconnected();

		int index = FindActive(identity, name);
		if (enabled && index == -1) m_Active.Insert(new ZBerryActiveFeature(identity, name, requiredNode));
		if (!enabled && index != -1) m_Active.Remove(index);

		TIntArray sentArgs = new TIntArray; //Local: a temporary inside the Param would be freed before it's sent
		if (args) sentArgs.InsertAll(args);
		GetZBerryRPC().SendRPC( ZBerryRPCOp.ClientFeature, new Param3<string, bool, ref TIntArray>(name, enabled, sentArgs), true, identity );
		OnServerFeatureChanged.Invoke(identity, name, enabled);
	}

	//Server: players who left have no identity anymore
	protected void ForgetDisconnected() {
		if (!GetGame().IsMultiplayer()) return;

		for (int i = m_Active.Count() - 1; i >= 0; --i) {
			if (!m_Active[i].Identity) m_Active.Remove(i);
		}
	}

	//Server
	bool IsActive(PlayerIdentity identity, string name) {
		return FindActive(identity, name) != -1;
	}

	//Server: after permission changes, switch off what players may no longer use, forget disconnected ones
	void Revalidate(ZBerryAuth auth) {
		for (int i = m_Active.Count() - 1; i >= 0; --i) {
			ZBerryActiveFeature feature = m_Active[i];
			if (GetGame().IsMultiplayer() && !feature.Identity) {
				m_Active.Remove(i);
				continue;
			}
			if (feature.RequiredNode == "" || auth.HasPermission(feature.Identity, feature.RequiredNode)) continue;

			ZomberryBase.Log( "ZomBerryAdmin", "INFO: Client feature " + feature.Name + " switched off for " + ZBDescribePlayer(feature.Identity) + ", its permission " + feature.RequiredNode + " was removed" );
			Set(feature.Identity, feature.Name, false);
		}
	}

	//Client: what runs when the server switches this feature
	void Register(string name, Class instance, string funcName) {
		m_Handlers.Set(name, new ZBerryRPCHandler(instance, funcName, ZBerrySPExec.Client));
	}

	//Client
	bool IsEnabledLocally(string name) {
		return m_Enabled.Find(name) != -1;
	}

	void OnClientFeature( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param3<string, bool, ref TIntArray> data; //Feature, on/off, arguments
		if (type != ZBerryCallType.Client) return;
		if ( !ctx.Read( data ) ) return;

		m_Enabled.RemoveItem(data.param1);
		if (data.param2) m_Enabled.Insert(data.param1);

		ZBerryRPCHandler handler = m_Handlers.Get(data.param1);
		if (!handler || !handler.GetInstance()) {
			ZomberryBase.Log( "ZomBerryClient", "WARN: The server switched client feature " + data.param1 + ", but nothing registered it" );
			return;
		}

		TIntArray args = data.param3;
		if (!args) args = new TIntArray;
		GetGame().GameScript.CallFunctionParams( handler.GetInstance(), handler.GetFuncName(), NULL, new Param2<bool, ref TIntArray>(data.param2, args) );
	}
};

static ref ZBerryClientFeatures g_ZBerryClientFeatures;

static ZBerryClientFeatures GetZBerryClientFeatures() {
	if (!g_ZBerryClientFeatures) g_ZBerryClientFeatures = new ZBerryClientFeatures();

	return g_ZBerryClientFeatures;
}
