class ZomBerryCamBase extends Camera {

	float sendUpdateAcc = 0.0;
	Object camTarget;

	void ZomBerryCamBase() {
		SetEventMask( EntityEvent.FRAME );

		camTarget = NULL;
	}

	override void EOnFrame(IEntity other, float timeSlice) {
		OnUpdate(timeSlice);

		if (sendUpdateAcc > 0.8) {
			GetGame().UpdateSpectatorPosition(GetPosition()); //Seems clientside, doesn't change bubble pos on both client/server :(

			sendUpdateAcc = 0;
		}

		sendUpdateAcc = sendUpdateAcc + timeSlice;
	}

	void OnUpdate(float timeSlice) {};
}
