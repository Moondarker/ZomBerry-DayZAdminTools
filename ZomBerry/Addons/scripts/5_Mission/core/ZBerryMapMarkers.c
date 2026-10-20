//One marker on the Map tab: a single object, or several of one category grouped by area
class ZBerryMapMarker {
	int Category; //ZBerryMapCategory
	vector Position;
	string Label;
	int Count; //Objects this marker stands for

	void ZBerryMapMarker(int category = 0, vector position = "0 0 0", string label = "", int count = 1) {
		Category = category;
		Position = position;
		Label = label;
		Count = count;
	}
};

typedef array<ref ZBerryMapMarker> ZBerryMapMarkerArray;

//Server side: turns the registry into markers for one admin
class ZBerryMapMarkers {
	static const int MAX_MARKERS = 1000; //Per reply, keeps the RPC small
	static const float GROUP_CELL = 50; //Metres: storage and base parts within one cell share a marker

	//Bases are dozens of parts and storage tends to sit in piles: one marker per area for those
	static bool IsGrouped(int category) {
		return category == ZBerryMapCategory.STORAGE || category == ZBerryMapCategory.BASES;
	}

	static string GetCategoryName(int category) {
		TStringArray names = {"Vehicle", "Tent", "Storage", "Base", "Body"};
		if (category < 0 || category >= names.Count()) return "";
		return names[category];
	}

	//Markers of the given categories. dropped: markers left out because of MAX_MARKERS
	static ZBerryMapMarkerArray Build(TIntArray categories, out int dropped) {
		ZBerryMapMarkerArray result = new ZBerryMapMarkerArray;
		dropped = 0;

		foreach (int category : categories) {
			map<string, ref ZBerryMapMarker> cells = new map<string, ref ZBerryMapMarker>;
			TStringArray cellOrder = new TStringArray;

			array<EntityAI> entities = ZBerryMapRegistry.GetEntities(category);
			foreach (EntityAI entity : entities) {
				if (!IsShown(category, entity)) continue;

				vector position = entity.GetPosition();
				if (!IsGrouped(category)) {
					Add(result, new ZBerryMapMarker(category, position, GetLabel(category, entity), 1), dropped);
					continue;
				}

				string cell = GetCellKey(position);
				ZBerryMapMarker grouped = cells.Get(cell);
				if (!grouped) {
					grouped = new ZBerryMapMarker(category, position, "", 0);
					cells.Set(cell, grouped);
					cellOrder.Insert(cell);
				}
				grouped.Count = grouped.Count + 1;
			}

			foreach (string key : cellOrder) {
				ZBerryMapMarker marker = cells.Get(key);
				marker.Label = GetCategoryName(category);
				if (marker.Count > 1) marker.Label += " (" + marker.Count + ")";
				Add(result, marker, dropped);
			}
		}

		return result;
	}

	protected static void Add(ZBerryMapMarkerArray result, ZBerryMapMarker marker, out int dropped) {
		if (result.Count() >= MAX_MARKERS) {
			dropped = dropped + 1;
			return;
		}
		result.Insert(marker);
	}

	//Typed float locals: Enforce doesn't do plain maths on vector components mixed with constants
	static string GetCellKey(vector position) {
		float x = position[0];
		float z = position[2];
		int cellX = Math.Floor(x / GROUP_CELL);
		int cellZ = Math.Floor(z / GROUP_CELL);
		return cellX.ToString() + ":" + cellZ.ToString();
	}

	//Objects in someone's hands or cargo, packed tents and living players aren't on the map
	static bool IsShown(int category, EntityAI entity) {
		if (!entity || entity.GetHierarchyParent()) return false;

		if (category == ZBerryMapCategory.TENTS) {
			TentBase tent = TentBase.Cast(entity);
			return tent && tent.GetState() == TentBase.PITCHED;
		}
		if (category == ZBerryMapCategory.BODIES) return !entity.IsAlive();

		return true;
	}

	static string GetLabel(int category, EntityAI entity) {
		if (category == ZBerryMapCategory.BODIES) {
			PlayerBase body = PlayerBase.Cast(entity);
			if (body && body.GetCachedName() != "") return "Body: " + body.GetCachedName();
			return GetCategoryName(category);
		}

		return entity.GetDisplayName();
	}
};
