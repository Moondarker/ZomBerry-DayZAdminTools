class ZomBerryCamSpec extends ZomBerryCamBase {

	void ZomBerryCamSpec() {

	}

	override void OnUpdate(float timeSlice) {

		Human targetBody = Human.Cast(camTarget);
		if (!targetBody) return;

		int headIdx = targetBody.GetBoneIndexByName("Head");
		vector headWP = targetBody.GetBonePositionWS(headIdx);

		float pQuat[4]; //w x y z
		targetBody.GetBoneRotationWS(headIdx, pQuat);

		vector headAngle = Math3D.QuatToAngles(pQuat); //ToEulerAngle(pQuat); //roll; pitch <-?-> yaw

		headAngle[0] = headAngle[0] - 90; //+= is not supported for arrays/vectors yet D:
		headAngle[2] = 0;

		SetOrientation(headAngle);
		SetPosition(headWP);
	}
}
