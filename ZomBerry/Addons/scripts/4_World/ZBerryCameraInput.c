//Vanilla's player cameras keep the input controller and the weapons command modifier they were created with.
//Spectating and FreeCam hand the admin's client a spectator camera and give the body back afterwards (ZBerryBodies);
//the cameras survive that, but what they kept doesn't, and the first camera update after taking the body back read
//through it and crashed the game (UpdateUDAngleUnlocked reads both). Every camera that reads them in its update now
//takes the character's current ones first, and skips the update while the character has no input controller (in the
//middle of the handover), rather than keep the old one
modded class DayZPlayerCameraBase {
	protected bool ZBRefreshInput() {
		if (!m_pPlayer) return false;

		HumanCommandWeapons weapons = m_pPlayer.GetCommandModifier_Weapons();
		if (weapons) m_CommandWeapons = weapons;
		HumanInputController input = m_pPlayer.GetInputController();
		if (!input) return false;
		m_pInput = input;
		return true;
	}

	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}

	override void OnActivate(DayZPlayerCamera pPrevCamera, DayZPlayerCameraResult pPrevCameraResult) {
		ZBRefreshInput();
		super.OnActivate(pPrevCamera, pPrevCameraResult);
	}
};

//Each class below overrides OnUpdate and may read the input before calling up the chain
modded class DayZPlayerCamera1stPerson {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera1stPersonUnconscious {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPerson {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonJump {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonErcSpr {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonCrouch {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonProneBase {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCameraIronsights {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera1stPersonVehicle {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonVehicle {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonVehicleMotorbike {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		if (!ZBRefreshInput()) return;
		super.OnUpdate(pDt, pOutResult);
	}
};
