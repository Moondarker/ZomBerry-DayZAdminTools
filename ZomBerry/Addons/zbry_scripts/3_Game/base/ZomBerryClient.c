class ZomberryClient extends ZomberryBase {

	void ZomberryClient() {

		GetRPCManager().AddRPC("ZomBerryAT", "SyncFunctions", this, SingeplayerExecutionType.Client);
	}

	void SyncFunctions(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target) {

	}

	void OnClientReady() {
		Log("ZomberryClient", "OnClientReady");

		GetRPCManager().SendRPC("ZomBerryAT", "InitRequest", new Param1<ZBerryInitRequestPacket>(new ZBerryInitRequestPacket()));
	}

	string GetRemoteVersion() {
		return "1.0.0";
	}

	bool IsAdmin() {
		return true;
	}
}
