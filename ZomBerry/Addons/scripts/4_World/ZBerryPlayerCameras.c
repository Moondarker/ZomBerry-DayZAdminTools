//Back in the body after spectating or FreeCam, the player's camera starts with no previous camera to take its angles
//from: level, while the body still aims where it did before. Vanilla keeps the difference as an extra angle, so the
//view looked ahead while the weapon (and the scope, once aiming) pointed up or down. The camera starts where the body
//aims instead
modded class DayZPlayerCamera1stPerson {
	override void OnActivate(DayZPlayerCamera pPrevCamera, DayZPlayerCameraResult pPrevCameraResult) {
		super.OnActivate(pPrevCamera, pPrevCameraResult);
		if (pPrevCamera || !m_CommandWeapons) return;

		m_fUpDownAngle = m_CommandWeapons.GetBaseAimingAngleUD();
		m_fUpDownAngleAdd = 0;
	}
};

modded class DayZPlayerCamera3rdPerson {
	override void OnActivate(DayZPlayerCamera pPrevCamera, DayZPlayerCameraResult pPrevCameraResult) {
		super.OnActivate(pPrevCamera, pPrevCameraResult);
		if (pPrevCamera || !m_CommandWeapons) return;

		m_fUpDownAngle = m_CommandWeapons.GetBaseAimingAngleUD();
		m_fUpDownAngleAdd = 0;
	}
};
