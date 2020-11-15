class ZomberryServer extends ZomberryBase {

	void ZomberryServer() {
		GetRPCManager().AddRPC("ZomBerryAT", "InitRequest", this, SingeplayerExecutionType.Client);
	}

	void InitRequest(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target) {
		Param1<ZBerryInitRequestPacket> initInfo; //direction (true = to server), version (only in client dir)
		if (!ctx.Read(initInfo)) {
			Log( "ZomBerryAT", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") - Auth error, please update client to latest version (v" + g_zbryVer + "+)!");
			return;
		}

		Log( "ZomBerryAT", "INFO: v" + initInfo.param1.version + " client connected.");
	}
}
