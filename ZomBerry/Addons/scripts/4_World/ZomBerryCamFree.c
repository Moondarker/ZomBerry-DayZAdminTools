class ZomBerryCamFree extends ZomBerryCamBase {

	private float speedMultiplier = 1.0;

	void ZomBerryCamFree() {

	}

	override void OnUpdate(float timeSlice) {

		Input input = GetGame().GetInput();

		if( input.GetActionDown("UAZoomInOptics") )
			speedMultiplier = speedMultiplier + 2;
		if( input.GetActionDown("UAZoomOutOptics") )
			speedMultiplier = speedMultiplier - 2;

		float speed = 5.0 * speedMultiplier;
		if( input.GetAction("UATurbo") )
			speed *= 2;

		if( input.GetAction("UAWalkRunTemp") )
			speed *= 0.5;

		float forward = input.GetAction("UAMoveForward") - input.GetAction("UAMoveBack");
		float strafe = input.GetAction("UAMoveRight") - input.GetAction("UAMoveLeft"); //BI, pls fix
		float vertical = input.GetAction("UALeanLeft") - input.GetAction("UALeanRight");

		vector direction = GetDirection();
		vector directionAside = vector.Up * direction;

		vector oldPos = GetPosition();
		vector forwardChange = forward * timeSlice * direction * speed;
		vector strafeChange = strafe * timeSlice * directionAside * speed;
		vector verticalChange = vertical * timeSlice * vector.Up * speed;
		vector newPos = oldPos + forwardChange + strafeChange + verticalChange;
		float yMin = GetGame().SurfaceY(newPos[0], newPos[2]);
		if( newPos[1] < yMin )
			newPos[1] = yMin;

		SetPosition(newPos);

		float yawDiff = input.GetAction("UAAimLeft") - input.GetAction("UAAimRight");
		float pitchDiff = input.GetAction("UAAimDown") - input.GetAction("UAAimUp");
		vector oldOrient = GetOrientation();
		vector newOrient = oldOrient;
		newOrient[0] = newOrient[0] - Math.RAD2DEG * yawDiff * timeSlice;
		newOrient[1] = newOrient[1] - Math.RAD2DEG * pitchDiff * timeSlice;
		if( newOrient[1] < -89 )
			newOrient[1] = -89;
		if( newOrient[1] > 89 )
			newOrient[1] = 89;

		SetOrientation(newOrient);
	}
}
