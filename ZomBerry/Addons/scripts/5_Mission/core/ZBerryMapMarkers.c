//One marker on the Map tab: a single object, or several of one category grouped by area
class ZBerryMapMarker {
	int MarkerCategory; //ZBerryEntityCategory, below MARKER_COUNT
	vector MarkerPos;
	string MarkerLabel;
	int ObjectCount; //Objects this marker stands for

	void ZBerryMapMarker(int category = 0, vector position = "0 0 0", string label = "", int count = 1) {
		MarkerCategory = category;
		MarkerPos = position;
		MarkerLabel = label;
		ObjectCount = count;
	}
};

typedef array<ref ZBerryMapMarker> ZBerryMapMarkerArray;

//The markers of one category at one moment, with a signature to tell whether they changed since the last send
class ZBerryMarkerBatch {
	int BatchCategory;
	ref ZBerryMapMarkerArray BatchMarkers = new ZBerryMapMarkerArray;
	int DroppedCount; //Left out because of ZBerryMapMarkers.MAX_MARKERS
	int BatchSignature;
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
		batch.BatchCategory = category;

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
			grouped.ObjectCount = grouped.ObjectCount + 1;
		}

		foreach (string key : cellOrder) {
			ZBerryMapMarker marker = cells.Get(key);
			marker.MarkerLabel = GetCategoryName(category);
			if (marker.ObjectCount > 1) marker.MarkerLabel += " (" + marker.ObjectCount + ")";
			Add(batch, marker);
		}

		batch.BatchSignature = GetSignature(batch);
		return batch;
	}

	protected static void Add(ZBerryMarkerBatch batch, ZBerryMapMarker marker) {
		if (batch.BatchMarkers.Count() >= MAX_MARKERS) {
			batch.DroppedCount = batch.DroppedCount + 1;
			return;
		}
		batch.BatchMarkers.Insert(marker);
	}

	//Changes when a marker appears, disappears, moves a metre or changes its count
	static int GetSignature(ZBerryMarkerBatch batch) {
		int signature = batch.BatchMarkers.Count() * 7919 + batch.DroppedCount;
		foreach (ZBerryMapMarker marker : batch.BatchMarkers) {
			int packed = ZBerryStatePacker.PackPosition(marker.MarkerPos);
			signature = signature * 31 + packed;
			signature = signature * 31 + marker.ObjectCount;
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
