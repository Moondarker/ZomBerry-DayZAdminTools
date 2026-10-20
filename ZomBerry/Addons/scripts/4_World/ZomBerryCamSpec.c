//Spectator camera (#60): sees through a player's eyes, or over their shoulder while they're in third person.
//The server creates it (SelectSpectator), the spectate client feature tells it whom to follow (SetTarget, statics,
//as the camera entity may appear before or after the feature arrives)
class ZomBerryCamSpec extends ZomBerryCamBase {
	static const float FIRST_PERSON_FORWARD = 0.12; //Metres in front of the head bone, so the head mesh stays behind
	static const float THIRD_PERSON_DISTANCE = 2.2;
	static const float THIRD_PERSON_SIDE = 0.45;
	static const float THIRD_PERSON_UP = 0.25;
	static const float SMOOTHING = 12; //Higher follows faster

	protected static int s_TargetLow, s_TargetHigh;
	protected static bool s_HasTarget;
	protected static bool s_ThirdPerson;

	protected float m_Yaw, m_Pitch;
	protected bool m_Initialised;

	static void SetTarget(int networkLow, int networkHigh, bool thirdPerson) {
		s_TargetLow = networkLow;
		s_TargetHigh = networkHigh;
		s_ThirdPerson = thirdPerson;
		s_HasTarget = true;
	}

	static void ClearTarget() {
		s_HasTarget = false;
	}

	override void OnUpdate(float timeSlice) {
		if (!s_HasTarget) return;

		Object found = GetGame().GetObjectByNetworkId(s_TargetLow, s_TargetHigh);
		if (found) camTarget = found;

		Human target = Human.Cast(camTarget);
		if (!target) return;

		vector head = target.GetBonePositionWS(target.GetBoneIndexByName("Head"));
		if (head == vector.Zero) head = target.GetPosition() + "0 1.6 0";

		//Body heading plus the replicated aiming angles: where they look, without the head bone's animation wobble
		vector orientation = target.GetOrientation();
		float yaw = orientation[0];
		float pitch = 0;
		HumanCommandWeapons weapons = target.GetCommandModifier_Weapons();
		if (weapons) {
			yaw = yaw + weapons.GetBaseAimingAngleLR();
			pitch = weapons.GetBaseAimingAngleUD();
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
