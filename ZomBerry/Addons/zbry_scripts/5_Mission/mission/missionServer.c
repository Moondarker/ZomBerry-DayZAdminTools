modded class MissionServer {
	ref ZomberryBase m_ZomberryBase;

	void MissionServer() {
		ZomberryBase.Log( "ZomBerry", "Starting Server side v" + g_zbryVer );
	}

	private ref ZomberryBase GetZomberryBase() {
		if ( !m_ZomberryBase ) {
			m_ZomberryBase = new ref ZomberryBase;
		}

		return m_ZomberryBase;
	}

	override void OnInit() {
		super.OnInit();

		GetZomberryBase().OnServerReady();
	}
};
