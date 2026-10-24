class ZomBerryCamFree extends ZomBerryCamBase {

	private float speedMultiplier = 1.0;
	protected static int s_Count; //FreeCam cameras on this machine: the admin's own body is hidden while there's one

	void ZomBerryCamFree() {
		++s_Count;
	}

	void ~ZomBerryCamFree() {
		--s_Count;
	}

	static bool IsUsedHere() {
		return s_Count > 0;
	}

	override void OnUpdate(float timeSlice) {

		Input input = GetGame().GetInput();

		if( input.LocalPress("UAZoomInOptics") )
			speedMultiplier = speedMultiplier + 2;
		if( input.LocalPress("UAZoomOutOptics") )
			speedMultiplier = speedMultiplier - 2;

		float speed = 5.0 * speedMultiplier;
		if( input.LocalValue("UATurbo") )
			speed *= 2;

		if( input.LocalValue("UAWalkRunTemp") )
			speed *= 0.5;

		float forward = input.LocalValue("UAMoveForward") - input.LocalValue("UAMoveBack");
		float strafe = input.LocalValue("UAMoveRight") - input.LocalValue("UAMoveLeft"); //BI, pls fix
		float vertical = input.LocalValue("UALeanLeft") - input.LocalValue("UALeanRight");

		vector direction = GetDirection();
		vector directionAside = vector.Up * direction;

		vector oldPos = GetPosition();
		vector forwardChange = forward * timeSlice * direction * speed;
		vector strafeChange = strafe * timeSlice * directionAside * speed;
		vector verticalChange = vertical * timeSlice * vector.Up * speed;
		vector newPos = oldPos + forwardChange + strafeChange + verticalChange;
		//Kept above the terrain while it's above it. Underground areas (bunkers) are below the terrain: a camera already
		//under it moves freely, it was lifted to the surface there
		float yMin = GetGame().SurfaceY(newPos[0], newPos[2]);
		bool aboveGround = oldPos[1] >= GetGame().SurfaceY(oldPos[0], oldPos[2]) - 0.1;
		if( aboveGround && newPos[1] < yMin )
			newPos[1] = yMin;

		SetPosition(newPos);

		float yawDiff = input.LocalValue("UAAimLeft") - input.LocalValue("UAAimRight");
		float pitchDiff = input.LocalValue("UAAimDown") - input.LocalValue("UAAimUp");
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
