modded class StaminaHandler
{
	//God mode keeps stamina full. Server only, like the god mode flag: the client's sprint and jump checks use the
	//stamina the server sends. The cap from carried weight still applies
	override protected void ProcessStaminaChanges(float deltaT, bool isServerOrSingleplayer) {
		if (isServerOrSingleplayer && m_Player && m_Player.ZBIsGod()) {
			m_StaminaDepletion = 0;
			if (m_StaminaDelta < 0) m_StaminaDelta = 0;
			m_Stamina = m_StaminaCap;
		}

		super.ProcessStaminaChanges(deltaT, isServerOrSingleplayer);
	}
};
