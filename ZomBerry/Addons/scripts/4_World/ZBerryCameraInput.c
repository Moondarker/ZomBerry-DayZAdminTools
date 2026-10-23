//Vanilla's player cameras keep the input controller they were created with. Spectating and FreeCam detach the admin's
//body from their client (ZBerryBodies) and give it back afterwards; the cameras survive that, but the controller they
//kept doesn't, and the first camera update after taking the body back read through it and crashed the game.
//Every camera that reads the input in its update now takes the character's current controller first
modded class DayZPlayerCameraBase {
	protected void ZBRefreshInput() {
		if (!m_pPlayer) return;

		HumanInputController input = m_pPlayer.GetInputController();
		if (input) m_pInput = input;
	}

	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
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
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera1stPersonUnconscious {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPerson {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonJump {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonErcSpr {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonCrouch {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonProneBase {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCameraIronsights {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera1stPersonVehicle {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonVehicle {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};

modded class DayZPlayerCamera3rdPersonVehicleMotorbike {
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult) {
		ZBRefreshInput();
		super.OnUpdate(pDt, pOutResult);
	}
};
