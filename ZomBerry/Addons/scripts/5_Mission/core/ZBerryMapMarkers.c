//One marker on the Map tab: a single object, or several of one category grouped by area
class ZBerryMapMarker {
	int Category; //ZBerryEntityCategory, below MARKER_COUNT
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

//The markers of one category at one moment, with a signature to tell whether they changed since the last send
class ZBerryMarkerBatch {
	int Category;
	ref ZBerryMapMarkerArray Markers = new ZBerryMapMarkerArray;
	int Dropped; //Left out because of ZBerryMapMarkers.MAX_MARKERS
	int Signature;
};

//Server side: turns the registry into markers
class ZBerryMapMarkers {
	static const int MAX_MARKERS = 1000; //Per category and message, keeps the RPC small
	static const float GROUP_CELL = 50; //Metres: storage and base parts within one cell share a marker

	//Bases are dozens of parts and storage tends to sit in piles: one marker per area for those
	static bool IsGrouped(int category) {
		return category == ZBerryEntityCategory.STORAGE || category == ZBerryEntityCategory.BASES;
	}

	static string GetCategoryName(int category) {
		TStringArray names = {"Vehicle", "Tent", "Storage", "Base", "Body"};
		if (category < 0 || category >= names.Count()) return "";
		return names[category];
	}

	static ZBerryMarkerBatch Build(int category) {
		ZBerryMarkerBatch batch = new ZBerryMarkerBatch;
		batch.Category = category;

		map<string, ref ZBerryMapMarker> cells = new map<string, ref ZBerryMapMarker>;
		TStringArray cellOrder = new TStringArray;

		array<EntityAI> entities = ZBerryEntityRegistry.GetEntities(category);
		foreach (EntityAI entity : entities) {
			if (!IsShown(category, entity)) continue;

			vector position = entity.GetPosition();
			if (!IsGrouped(category)) {
				Add(batch, new ZBerryMapMarker(category, position, GetLabel(category, entity), 1));
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
			Add(batch, marker);
		}

		batch.Signature = GetSignature(batch);
		return batch;
	}

	protected static void Add(ZBerryMarkerBatch batch, ZBerryMapMarker marker) {
		if (batch.Markers.Count() >= MAX_MARKERS) {
			batch.Dropped = batch.Dropped + 1;
			return;
		}
		batch.Markers.Insert(marker);
	}

	//Changes when a marker appears, disappears, moves a metre or changes its count
	static int GetSignature(ZBerryMarkerBatch batch) {
		int signature = batch.Markers.Count() * 7919 + batch.Dropped;
		foreach (ZBerryMapMarker marker : batch.Markers) {
			int packed = ZBerryStatePacker.PackPosition(marker.Position);
			signature = signature * 31 + packed;
			signature = signature * 31 + marker.Count;
		}
		return signature;
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

		if (category == ZBerryEntityCategory.TENTS) {
			TentBase tent = TentBase.Cast(entity);
			return tent && tent.GetState() == TentBase.PITCHED;
		}
		if (category == ZBerryEntityCategory.BODIES) return !entity.IsAlive();

		return true;
	}

	static string GetLabel(int category, EntityAI entity) {
		if (category == ZBerryEntityCategory.BODIES) {
			PlayerBase body = PlayerBase.Cast(entity);
			if (body && body.GetCachedName() != "") return "Body: " + body.GetCachedName();
			return GetCategoryName(category);
		}

		return entity.GetDisplayName();
	}
};
