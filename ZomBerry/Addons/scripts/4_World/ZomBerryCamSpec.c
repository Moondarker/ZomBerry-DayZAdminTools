//Spectator camera (#60): sees through a player's eyes, or over their shoulder while they're in third person.
//The server creates it (SelectSpectator), the spectate client feature tells it whom to follow (SetTarget, statics,
//as the camera entity may appear before or after the feature arrives). Where they look comes from their own client
//(SetView, relayed by the server): other clients only see the body's aiming angles, which miss looking up and down.
//The server's position for them moves the camera to a target outside this client's network bubble
class ZomBerryCamSpec extends ZomBerryCamBase {
	static const float FIRST_PERSON_FORWARD = 0.12; //Metres in front of the head bone, so the head mesh stays behind
	static const float THIRD_PERSON_DISTANCE = 2.2;
	static const float THIRD_PERSON_SIDE = 0.45;
	static const float THIRD_PERSON_UP = 0.25;
	static const float SMOOTHING = 12; //Higher follows faster
	static const float FAR = 50; //Metres between the server's position and the local copy: the local one is stale

	protected static int s_TargetLow, s_TargetHigh;
	protected static bool s_HasTarget;
	protected static bool s_ThirdPerson;
	protected static bool s_HasPosition, s_HasView;
	protected static float s_ViewYaw, s_ViewPitch;
	protected static vector s_Position; //Where the server last saw the target

	protected float m_Yaw, m_Pitch;
	protected bool m_Initialised;

	static void SetTarget(int networkLow, int networkHigh, bool thirdPerson) {
		if (!s_HasTarget || networkLow != s_TargetLow || networkHigh != s_TargetHigh) {
			s_HasPosition = false;
			s_HasView = false;
		}
		s_TargetLow = networkLow;
		s_TargetHigh = networkHigh;
		s_ThirdPerson = thirdPerson;
		s_HasTarget = true;
	}

	static void ClearTarget() {
		s_HasTarget = false;
		s_HasPosition = false;
		s_HasView = false;
	}

	//hasView false: the target's client hasn't reported its camera yet, only the position is new
	static void SetView(bool hasView, float yaw, float pitch, vector position) {
		s_Position = position;
		s_HasPosition = true;
		if (!hasView) return;

		s_ViewYaw = yaw;
		s_ViewPitch = pitch;
		s_HasView = true;
	}

	override void OnUpdate(float timeSlice) {
		if (!s_HasTarget) return;

		Object found = GetGame().GetObjectByNetworkId(s_TargetLow, s_TargetHigh);
		if (found) camTarget = found;

		Human target = Human.Cast(camTarget);
		if (target && s_HasPosition && vector.Distance(target.GetPosition(), s_Position) > FAR) target = NULL; //Left the bubble
		if (!target && !s_HasPosition) return;

		vector head;
		if (target) head = target.GetBonePositionWS(target.GetBoneIndexByName("Head"));
		if (head == vector.Zero && target) head = target.GetPosition() + "0 1.6 0";
		if (!target) head = s_Position + "0 1.6 0"; //Not here (yet): the camera goes there, and the bubble with it

		//Their own camera once reported; until then body heading plus aiming angles (no head bone animation wobble)
		float yaw = m_Yaw;
		float pitch = m_Pitch;
		if (s_HasView) {
			yaw = s_ViewYaw;
			pitch = s_ViewPitch;
		} else if (target) {
			vector orientation = target.GetOrientation();
			yaw = orientation[0];
			pitch = 0;
			HumanCommandWeapons weapons = target.GetCommandModifier_Weapons();
			if (weapons) {
				yaw = yaw + weapons.GetBaseAimingAngleLR();
				pitch = weapons.GetBaseAimingAngleUD();
			}
		}

		if (!m_Initialised) {
			m_Yaw = yaw;
			m_Pitch = pitch;
			m_Initialised = true;
		}
		float blend = Math.Clamp(timeSlice * SMOOTHING, 0, 1);
		m_Yaw = m_Yaw + AngleDifference(yaw, m_Yaw) * blend;
		m_Pitch = m_Pitch + (pitch - m_Pitch) * blend;

		vector forward = Vector(m_Yaw, m_Pitch, 0).AnglesToVector();
		vector flatForward = Vector(m_Yaw, 0, 0).AnglesToVector();
		vector right = vector.Up * flatForward; //Same cross product FreeCam uses for strafing

		vector position = head + forward * FIRST_PERSON_FORWARD;
		if (s_ThirdPerson) position = head - forward * THIRD_PERSON_DISTANCE + right * THIRD_PERSON_SIDE + vector.Up * THIRD_PERSON_UP;

		SetPosition(position);
		SetOrientation(Vector(m_Yaw, m_Pitch, 0));
	}

	//Shortest signed difference between two angles in degrees
	static float AngleDifference(float to, float from) {
		float difference = to - from;
		while (difference > 180) difference -= 360;
		while (difference < -180) difference += 360;
		return difference;
	}
}
