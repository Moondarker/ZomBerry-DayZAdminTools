class ZomBerryCamFree extends ZomBerryCamBase {

	private float speedMultiplier = 1.0;
	static const float PROBE_DEPTH = 10; //Metres below the terrain an underground structure is looked for
	static const float PROBE_MOVE = 1; //Metres moved sideways before looking again
	protected bool m_Probed;
	protected vector m_ProbePos;
	protected bool m_ProbeFound;

	protected static int s_ActiveAtMs = -1; //GetGame().GetTime() of the active FreeCam camera's last update

	//Whether this machine looks through a FreeCam camera now: the admin's own body is hidden meanwhile. Counting the
	//cameras doesn't tell, they aren't deleted when the client gets its body back; Camera.GetCurrentCamera() can
	//return a camera that no longer exists and crashed the game. So the active camera reports itself
	static bool IsUsedHere() {
		if (s_ActiveAtMs < 0) return false;
		int now = GetGame().GetTime();
		return now >= s_ActiveAtMs && now - s_ActiveAtMs < 500; //The game time starts over in a new mission
	}

	//Whether an object lies within PROBE_DEPTH under the terrain here. Only cast when the camera is about to go under
	//the terrain, and again only after moving PROBE_MOVE sideways. Terrain and water hits have no object and don't count
	protected bool HasStructureBelow(vector position, float groundY) {
		vector moved = Vector(position[0] - m_ProbePos[0], 0, position[2] - m_ProbePos[2]);
		if (m_Probed && moved.Length() < PROBE_MOVE) return m_ProbeFound;
		m_Probed = true;
		m_ProbePos = position;
		m_ProbeFound = false;

		vector from = Vector(position[0], groundY - 0.2, position[2]); //Under the terrain's surface, or it's hit first
		vector to = from - Vector(0, PROBE_DEPTH, 0);
		vector contactPos, contactDir;
		int contactComponent;
		set<Object> hits = new set<Object>;
		DayZPhysics.RaycastRV(from, to, contactPos, contactDir, contactComponent, hits);
		for (int i = 0; i < hits.Count(); ++i) {
			if (hits.Get(i)) {
				m_ProbeFound = true;
				break;
			}
		}
		return m_ProbeFound;
	}

	override void OnUpdate(float timeSlice) {
		s_ActiveAtMs = GetGame().GetTime();

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
		//Kept above the terrain while it's above it, unless there's something built under the terrain right there (an
		//underground area's entrance): then it goes through. Underground areas are below the terrain, so a camera already
		//under it moves freely, it was lifted to the surface there
		float yMin = GetGame().SurfaceY(newPos[0], newPos[2]);
		bool aboveGround = oldPos[1] >= GetGame().SurfaceY(oldPos[0], oldPos[2]) - 0.1;
		if( aboveGround && newPos[1] < yMin && !HasStructureBelow(newPos, yMin) )
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
