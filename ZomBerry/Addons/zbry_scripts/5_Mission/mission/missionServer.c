modded class MissionServer {
	protected ref ZomberryServer m_ZomberryServer;

	void MissionServer() {
		m_ZomberryServer = new ref ZomberryServer;
		ZomberryServer.Log( "ZomBerry", "Starting Server side v" + g_zbryVer );
	}

	override void OnInit() {
		super.OnInit();

		//m_ZomberryBase.OnServerReady();
	}
};
