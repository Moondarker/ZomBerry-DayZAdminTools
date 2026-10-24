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

	static const float BONE_PADDING = 0.15; //Metres around the joints: they're inside the body, the outline isn't

	//World-space box around a character's joints, padded to the body's outline. Follows the pose (crouched, prone),
	//unlike the model's bounding boxes. False when the model has none of the bones
	static bool GetBoneBounds(Human obj, out vector mins, out vector maxs) {
		TIntArray bones = GetSkeletonBones(obj);
		bool found = false;
		foreach (int bone : bones) {
			vector bonePos = obj.GetBonePositionWS(bone);
			if (bonePos == vector.Zero) continue;

			if (!found) {
				mins = bonePos;
				maxs = bonePos;
				found = true;
				continue;
			}
			for (int axis = 0; axis < 3; ++axis) {
				mins[axis] = Math.Min(mins[axis], bonePos[axis]);
				maxs[axis] = Math.Max(maxs[axis], bonePos[axis]);
			}
		}
		if (!found) return false;

		mins = mins - Vector(BONE_PADDING, BONE_PADDING, BONE_PADDING);
		maxs = maxs + Vector(BONE_PADDING, BONE_PADDING * 1.5, BONE_PADDING); //The head bone sits low in the skull
		return true;
	}

	//Model-space box of the object's physical shape. ClippingInfo (what rendering uses) covers every pose a model can
	//take and was several times wider than a character; it's only the fallback for objects without a collision box
	static void GetModelBounds(Object obj, out vector mins, out vector maxs) {
		vector minMax[2];
		if (!obj.GetCollisionBox(minMax)) obj.ClippingInfo(minMax);
		mins = minMax[0];
		maxs = minMax[1];
	}

	static const float STANDING_HALF_WIDTH = 0.3; //Metres: a standing character, for boxes not taken from the model
	static const float STANDING_HEIGHT = 1.8;

	//Screen rectangle of a standing character's size at a position, whatever the model looks like (invisible players
	//are shrunk on other clients, their bones are all in one spot). False when any corner is behind the camera
	static bool GetStandingScreenBox(vector position, out float left, out float top, out float right, out float bottom) {
		vector boxMin = position - Vector(STANDING_HALF_WIDTH, 0, STANDING_HALF_WIDTH);
		vector boxMax = position + Vector(STANDING_HALF_WIDTH, STANDING_HEIGHT, STANDING_HALF_WIDTH);
		return ProjectBox(NULL, boxMin, boxMax, true, left, top, right, bottom);
	}

	//Screen rectangle around the object (characters: around their joints), false when any corner is behind the camera
	static bool GetScreenBox(Object obj, out float left, out float top, out float right, out float bottom) {
		vector boxMin, boxMax;
		bool worldSpace = false;
		Human human = Human.Cast(obj);
		if (human) worldSpace = GetBoneBounds(human, boxMin, boxMax);
		if (!worldSpace) GetModelBounds(obj, boxMin, boxMax);
		return ProjectBox(obj, boxMin, boxMax, worldSpace, left, top, right, bottom);
	}

	//The screen rectangle around a box's 8 corners: world space, or obj's model space
	protected static bool ProjectBox(Object obj, vector boxMin, vector boxMax, bool worldSpace, out float left, out float top, out float right, out float bottom) {
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

			vector cornerPos = Vector(cornerX, cornerY, cornerZ);
			if (!worldSpace) cornerPos = obj.ModelToWorld(cornerPos);

			float x, y;
			if (!ToScreen(cornerPos, x, y)) return false;
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
