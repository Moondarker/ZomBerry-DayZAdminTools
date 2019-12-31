modded class MissionServer {
	protected ref ZomberryBase m_ZomberryBase;

	void MissionServer() {
		m_ZomberryBase = new ref ZomberryBase;
		ZomberryBase.Log( "ZomBerry", "Starting Server side v" + g_zbryVer );
	}

	void ~MissionServer() {
		if (!m_ZomberryBase)
	}

	override void OnInit() {
		super.OnInit();

		m_ZomberryBase.OnServerReady();
	}
};
