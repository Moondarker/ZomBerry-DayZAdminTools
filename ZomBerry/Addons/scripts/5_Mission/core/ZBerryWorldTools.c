//Server-side helpers for functions that act on the world around the cursor: picking the object an admin points at,
//finding things to clean up within a radius, putting a vehicle back on its wheels
class ZBerryWorldTools {
	//What a cleanup removes
	static const int CLEAN_INFECTED = 0;
	static const int CLEAN_BODIES = 1; //Dead players, infected and animals
	static const int CLEAN_LOOSE_ITEMS = 2; //Items on the ground, not built or placed things
	static const int CLEAN_RUINED = 3; //Ruined items on the ground

	static TStringArray GetCleanNames() {
		return {"Infected", "Dead bodies", "Loose items", "Ruined items"};
	}

	//Map objects (buildings, fences of the map, trees) exist on every client by themselves and aren't networked:
	//deleting them on the server only makes them invisible to new players. Only networked entities are fair game
	static bool IsNetworked(Object obj) {
		int low, high;
		obj.GetNetworkID(low, high);
		return low != 0 || high != 0;
	}

	//Metres from a point to the object's bounding box, 0 when the point is inside it. The cursor lands on the
	//surface of whatever the admin points at, so this picks that object rather than whatever is nearest by origin
	static float DistanceToBox(Object obj, vector worldPoint) {
		vector minMax[2];
		obj.ClippingInfo(minMax);
		vector boxMin = minMax[0];
		vector boxMax = minMax[1];
		vector localPoint = obj.WorldToModel(worldPoint);

		float dx = Math.Max(Math.Max(boxMin[0] - localPoint[0], 0), localPoint[0] - boxMax[0]);
		float dy = Math.Max(Math.Max(boxMin[1] - localPoint[1], 0), localPoint[1] - boxMax[1]);
		float dz = Math.Max(Math.Max(boxMin[2] - localPoint[2], 0), localPoint[2] - boxMax[2]);
		return Math.Sqrt(dx * dx + dy * dy + dz * dz);
	}

	//The networked entity closest to point (by bounding box), within maxDistance. Living players and things inside
	//inventories are never picked. vehiclesOnly: only cars and boats
	static EntityAI PickAt(vector point, float maxDistance, bool vehiclesOnly = false) {
		array<Object> objects = new array<Object>;
		array<CargoBase> proxies = new array<CargoBase>;
		GetGame().GetObjectsAtPosition3D(point, maxDistance + 10, objects, proxies); //Origins of big objects can be far from their surface

		EntityAI best;
		float bestDistance = maxDistance;
		foreach (Object obj : objects) {
			EntityAI entity = EntityAI.Cast(obj);
			if (!entity || !IsNetworked(entity) || entity.GetHierarchyParent()) continue;
			if (entity.IsInherited(Man) && entity.IsAlive()) continue;
			if (vehiclesOnly && !entity.IsInherited(Transport)) continue;

			float distance = DistanceToBox(entity, point);
			if (distance > bestDistance) continue;
			bestDistance = distance;
			best = entity;
		}
		return best;
	}

	//What a cleanup of this kind removes within radius of center
	static array<EntityAI> FindForCleanup(vector center, float radius, int kind) {
		array<EntityAI> result = new array<EntityAI>;
		array<Object> objects = new array<Object>;
		array<CargoBase> proxies = new array<CargoBase>;
		GetGame().GetObjectsAtPosition3D(center, radius, objects, proxies);

		foreach (Object obj : objects) {
			EntityAI entity = EntityAI.Cast(obj);
			if (!entity || !IsNetworked(entity) || entity.GetHierarchyParent()) continue;
			if (IsCleanupMatch(entity, kind)) result.Insert(entity);
		}
		return result;
	}

	static bool IsCleanupMatch(EntityAI entity, int kind) {
		if (kind == CLEAN_INFECTED) return entity.IsInherited(ZombieBase) && entity.IsAlive();

		if (kind == CLEAN_BODIES) {
			if (entity.IsAlive()) return false;
			return entity.IsInherited(PlayerBase) || entity.IsInherited(ZombieBase) || entity.IsInherited(AnimalBase);
		}

		if (!entity.IsInherited(ItemBase) || IsPlacedByPlayers(entity)) return false;
		if (kind == CLEAN_LOOSE_ITEMS) return true;
		if (kind == CLEAN_RUINED) return entity.IsRuined();
		return false;
	}

	//Bases, tents, storage, stashes and fireplaces: what players build or set up, never "loose" items
	static bool IsPlacedByPlayers(EntityAI entity) {
		return entity.IsInherited(BaseBuildingBase) || entity.IsInherited(TentBase) || entity.IsInherited(DeployableContainer_Base) || entity.IsInherited(UndergroundStash) || entity.IsInherited(FireplaceBase);
	}

	//Upright, keeping the heading, slightly above the ground and at rest. Returns why it can't, "" when done
	static string FlipVehicle(Transport vehicle) {
		for (int seat = 0; seat < vehicle.CrewSize(); ++seat) {
			if (vehicle.CrewMember(seat)) return "someone is inside";
		}

		vector orientation = vehicle.GetOrientation();
		vector position = vehicle.GetPosition();
		float ground = GetGame().SurfaceY(position[0], position[2]);
		position[1] = Math.Max(position[1], ground) + 0.6;

		SetVelocity(vehicle, vector.Zero);
		dBodySetAngularVelocity(vehicle, vector.Zero);
		vehicle.SetOrientation(Vector(orientation[0], 0, 0));
		vehicle.SetPosition(position);
		dBodyActive(vehicle, ActiveState.ACTIVE);
		return "";
	}
};
