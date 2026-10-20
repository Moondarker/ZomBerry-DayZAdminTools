//Drawing helpers for ESP: world to screen, bounding boxes and skeletons, onto a CanvasWidget (physical pixels).
//Client side only, for what the client's network bubble holds
class ZBerryESPDraw {
	protected static ref map<string, ref TIntArray> s_SkeletonBones; //Per model type: bone index pairs

	//Pairs of bones to connect (the standard DayZ human skeleton)
	static TStringArray GetSkeletonPairs() {
		return {
			"Head", "Neck", "Neck", "Spine3", "Spine3", "Spine1", "Spine1", "Pelvis",
			"Neck", "LeftArm", "LeftArm", "LeftForeArm", "LeftForeArm", "LeftHand",
			"Neck", "RightArm", "RightArm", "RightForeArm", "RightForeArm", "RightHand",
			"Pelvis", "LeftUpLeg", "LeftUpLeg", "LeftLeg", "LeftLeg", "LeftFoot",
			"Pelvis", "RightUpLeg", "RightUpLeg", "RightLeg", "RightLeg", "RightFoot"
		};
	}

	//Bone ids of the pairs above, looked up once per model type. Human.GetBoneIndexByName returns a hash of the name,
	//so any int (also negative ones) is valid
	static TIntArray GetSkeletonBones(Human obj) {
		if (!s_SkeletonBones) s_SkeletonBones = new map<string, ref TIntArray>;

		string type = obj.GetType();
		TIntArray bones = s_SkeletonBones.Get(type);
		if (bones) return bones;

		bones = new TIntArray;
		TStringArray names = GetSkeletonPairs();
		foreach (string boneName : names) {
			bones.Insert(obj.GetBoneIndexByName(boneName));
		}
		s_SkeletonBones.Set(type, bones);
		return bones;
	}

	//Screen pixels of a world position, false when it's behind the camera
	static bool ToScreen(vector world, out float x, out float y) {
		vector toPoint = world - GetGame().GetCurrentCameraPosition();
		if (vector.Dot(toPoint, GetGame().GetCurrentCameraDirection()) <= 0.1) return false;

		vector screen = GetGame().GetScreenPos(world);
		x = screen[0];
		y = screen[1];
		return true;
	}

	//Screen rectangle around the object's bounding box, false when any corner is behind the camera
	static bool GetScreenBox(Object obj, out float left, out float top, out float right, out float bottom) {
		vector minMax[2];
		obj.ClippingInfo(minMax);
		vector boxMin = minMax[0];
		vector boxMax = minMax[1];

		left = 100000;
		top = 100000;
		right = -100000;
		bottom = -100000;
		for (int corner = 0; corner < 8; ++corner) {
			float cornerX = boxMin[0];
			float cornerY = boxMin[1];
			float cornerZ = boxMin[2];
			if ((corner & 1) != 0) cornerX = boxMax[0];
			if ((corner & 2) != 0) cornerY = boxMax[1];
			if ((corner & 4) != 0) cornerZ = boxMax[2];

			float x, y;
			if (!ToScreen(obj.ModelToWorld(Vector(cornerX, cornerY, cornerZ)), x, y)) return false;
			left = Math.Min(left, x);
			top = Math.Min(top, y);
			right = Math.Max(right, x);
			bottom = Math.Max(bottom, y);
		}
		return true;
	}

	static void DrawBox(CanvasWidget canvas, float left, float top, float right, float bottom, int color, float width) {
		canvas.DrawLine(left, top, right, top, width, color);
		canvas.DrawLine(right, top, right, bottom, width, color);
		canvas.DrawLine(right, bottom, left, bottom, width, color);
		canvas.DrawLine(left, bottom, left, top, width, color);
	}

	//Lines between the bones of a human model. Returns how many were drawn
	static int DrawSkeleton(CanvasWidget canvas, Human obj, int color, float width) {
		TIntArray bones = GetSkeletonBones(obj);
		int drawn = 0;
		for (int i = 0; i + 1 < bones.Count(); i += 2) {
			vector fromPos = obj.GetBonePositionWS(bones[i]);
			vector toPos = obj.GetBonePositionWS(bones[i + 1]);
			if (fromPos == vector.Zero || toPos == vector.Zero) continue; //A bone this model doesn't have

			float x1, y1, x2, y2;
			if (!ToScreen(fromPos, x1, y1) || !ToScreen(toPos, x2, y2)) continue;
			canvas.DrawLine(x1, y1, x2, y2, width, color);
			++drawn;
		}
		return drawn;
	}
};
