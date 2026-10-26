//God mode and invisibility hold the player's body temperature at a comfortable level: vanilla works out the heat
//comfort anew on every update, so it's set back to neutral after each one. Server: the stat reaches the client by sync
modded class Environment {
	override void Update(float pDelta) {
		super.Update(pDelta);
		if (!m_Player || !m_Player.ZBProtected()) return;

		m_HeatComfort = 0;
		m_Player.GetStatHeatComfort().Set(0);
	}
};
