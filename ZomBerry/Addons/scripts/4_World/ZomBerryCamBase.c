class ZomBerryCamBase extends Camera {

	static const float SEND_INTERVAL = 0.5; //Seconds between position reports, as vanilla's spectator camera
	static const float JUMP_DISTANCE = 50; //Metres moved since the last report that are reported at once

	float sendUpdateAcc = 0.0;
	Object camTarget;
	protected vector m_LastSent;

	void ZomBerryCamBase() {
		SetEventMask( EntityEvent.FRAME );

		camTarget = NULL;
	}

	override void EOnFrame(IEntity other, float timeSlice) {
		OnUpdate(timeSlice);

		//While the client controls no character (ZBerryBodies), this position is where the server keeps its network
		//bubble. A big jump (following a target that left the bubble) is reported at once, so the bubble catches up
		vector position = GetPosition();
		if (sendUpdateAcc > SEND_INTERVAL || vector.Distance(position, m_LastSent) > JUMP_DISTANCE) {
			GetGame().UpdateSpectatorPosition(position);
			m_LastSent = position;
			sendUpdateAcc = 0;
		}

		sendUpdateAcc = sendUpdateAcc + timeSlice;
	}

	void OnUpdate(float timeSlice) {};
}
