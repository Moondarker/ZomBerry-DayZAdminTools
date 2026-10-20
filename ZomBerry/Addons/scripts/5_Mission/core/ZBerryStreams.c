//World state the server streams to admins (players, map markers, ESP, spectating). Clients say what they show
//(Subscribe), the server pushes only that, only to those allowed to see it, and only what changed.
//Shared by ZBerryStreamServer and ZBerryStreamClient
class ZBerryStreamFlags {
	//What a client wants (Subscribe)
	static const int ROSTER = 1; //Who is online: names, admin flag, item in hands
	static const int STATE = 2; //Positions, headings, the selected player's health

	//What a client shows, decides the update rate
	static const int VIEW_MAP = 1;
};

//One online player as the roster lists them. Changes rarely, so it's only sent when something in it changes
class ZBerryRosterEntry {
	int Id; //Session id
	string Name;
	bool IsAdmin;
	string Extra; //Item in hands or vehicle, shown after the name

	void ZBerryRosterEntry(int id = 0, string name = "", bool isAdmin = false, string extra = "") {
		Id = id;
		Name = name;
		IsAdmin = isAdmin;
		Extra = extra;
	}

	bool IsSameAs(ZBerryRosterEntry other) {
		return other && Id == other.Id && Name == other.Name && IsAdmin == other.IsAdmin && Extra == other.Extra;
	}
};

typedef array<ref ZBerryRosterEntry> ZBerryRosterArray;

//Player state travels as 3 ints per player: session id, position, misc. That's 12 bytes instead of a full
//ZBerryPlayer with name and vector. Typed locals throughout: Enforce mixes up int maths on array/method operands
class ZBerryStatePacker {
	static const int INTS_PER_PLAYER = 3;
	static const int HEADING_STEP = 5; //Degrees: smaller turns aren't worth an update
	static const int BLOOD_STEP = 20; //Blood 0-5000 fits 8 bits in steps of 20
	static const int HAS_HEALTH = 16777216; //1 << 24: health and blood are filled in

	//x and z in whole metres, 15 + 16 bits: maps up to 32 km
	static int PackPosition(vector position) {
		float x = position[0];
		float z = position[2];
		int ix = Math.Clamp(Math.Round(x), 0, 32767);
		int iz = Math.Clamp(Math.Round(z), 0, 65535);
		return (ix << 16) | iz;
	}

	static void UnpackPosition(int packed, out float x, out float z) {
		int ix = (packed >> 16) & 32767;
		int iz = packed & 65535;
		x = ix;
		z = iz;
	}

	//Heading (yaw in degrees, compass-style) in 9 bits, health 0-100 in 7 bits, blood in 8 bits, then the HAS_HEALTH flag
	static int PackMisc(float heading, bool hasHeading, int health, int blood, bool hasHealth) {
		int misc = 0;
		if (hasHeading) {
			float step = HEADING_STEP;
			int rounded = Math.Round(heading / step);
			int degrees = rounded * HEADING_STEP;
			while (degrees < 0) degrees += 360;
			misc = degrees % 360;
		}

		if (hasHealth) {
			float bloodValue = blood;
			float bloodStep = BLOOD_STEP;
			int healthBits = Math.Clamp(health, 0, 100);
			int bloodBits = Math.Clamp(Math.Round(bloodValue / bloodStep), 0, 255);
			misc = misc | (healthBits << 9) | (bloodBits << 16) | HAS_HEALTH;
		}
		return misc;
	}

	static int GetHeading(int misc) {
		return misc & 511;
	}

	static bool HasHealth(int misc) {
		return (misc & HAS_HEALTH) != 0;
	}

	static int GetHealth(int misc) {
		return (misc >> 9) & 127;
	}

	static int GetBlood(int misc) {
		int bloodBits = (misc >> 16) & 255;
		return bloodBits * BLOOD_STEP;
	}
};
