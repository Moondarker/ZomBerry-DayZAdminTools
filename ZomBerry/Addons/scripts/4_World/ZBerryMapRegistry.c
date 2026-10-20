//Map marker categories, shared by the server registry and the Map tab
class ZBerryMapCategory {
	static const int VEHICLES = 0;
	static const int TENTS = 1;
	static const int STORAGE = 2; //Barrels, chests, crates, buried stashes
	static const int BASES = 3; //Fences, watchtowers, flags
	static const int BODIES = 4; //Dead players
	static const int COUNT = 5;

	static TStringArray GetKeys() {
		return {"vehicles", "tents", "storage", "bases", "bodies"};
	}

	static string GetKey(int category) {
		TStringArray keys = GetKeys();
		if (category < 0 || category >= keys.Count()) return "";
		return keys[category];
	}
};

//Server side: the objects each map marker category shows. Objects add and remove themselves (modded classes below),
//so nobody has to scan the whole map. Objects loaded from persistence register too, as they go through EEInit
class ZBerryMapRegistry {
	protected static ref array<ref array<EntityAI>> s_Entities;

	protected static array<EntityAI> GetList(int category) {
		if (category < 0 || category >= ZBerryMapCategory.COUNT) return NULL;

		if (!s_Entities) {
			s_Entities = new array<ref array<EntityAI>>;
			for (int i = 0; i < ZBerryMapCategory.COUNT; ++i) s_Entities.Insert(new array<EntityAI>);
		}
		return s_Entities[category];
	}

	static void Register(int category, EntityAI entity) {
		if (!entity || !GetGame().IsServer()) return;

		array<EntityAI> list = GetList(category);
		if (list && list.Find(entity) == -1) list.Insert(entity);
	}

	static void Unregister(int category, EntityAI entity) {
		if (!entity || !GetGame().IsServer()) return;

		array<EntityAI> list = GetList(category);
		if (list) list.RemoveItem(entity);
	}

	//The live objects of a category (deleted ones are dropped on the way)
	static array<EntityAI> GetEntities(int category) {
		array<EntityAI> result = new array<EntityAI>;
		array<EntityAI> list = GetList(category);
		if (!list) return result;

		for (int i = list.Count() - 1; i >= 0; --i) {
			if (!list[i]) {
				list.Remove(i);
				continue;
			}
			result.Insert(list[i]);
		}
		return result;
	}

	static int GetCount(int category) {
		array<EntityAI> list = GetList(category);
		if (!list) return 0;
		return list.Count();
	}

	static void Clear() {
		s_Entities = NULL;
	}
};

modded class CarScript {
	override void EEInit() {
		super.EEInit();
		ZBerryMapRegistry.Register(ZBerryMapCategory.VEHICLES, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryMapRegistry.Unregister(ZBerryMapCategory.VEHICLES, this);
		super.EEDelete(parent);
	}
};

modded class BoatScript {
	override void EEInit() {
		super.EEInit();
		ZBerryMapRegistry.Register(ZBerryMapCategory.VEHICLES, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryMapRegistry.Unregister(ZBerryMapCategory.VEHICLES, this);
		super.EEDelete(parent);
	}
};

//Packed tents register too: whether a tent is pitched is checked when markers are sent
modded class TentBase {
	override void EEInit() {
		super.EEInit();
		ZBerryMapRegistry.Register(ZBerryMapCategory.TENTS, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryMapRegistry.Unregister(ZBerryMapCategory.TENTS, this);
		super.EEDelete(parent);
	}
};

modded class DeployableContainer_Base {
	override void EEInit() {
		super.EEInit();
		ZBerryMapRegistry.Register(ZBerryMapCategory.STORAGE, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryMapRegistry.Unregister(ZBerryMapCategory.STORAGE, this);
		super.EEDelete(parent);
	}
};

modded class UndergroundStash {
	override void EEInit() {
		super.EEInit();
		ZBerryMapRegistry.Register(ZBerryMapCategory.STORAGE, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryMapRegistry.Unregister(ZBerryMapCategory.STORAGE, this);
		super.EEDelete(parent);
	}
};

modded class BaseBuildingBase {
	override void EEInit() {
		super.EEInit();
		ZBerryMapRegistry.Register(ZBerryMapCategory.BASES, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryMapRegistry.Unregister(ZBerryMapCategory.BASES, this);
		super.EEDelete(parent);
	}
};

//Bodies: from the moment a player dies until the body is gone
modded class PlayerBase {
	override void EEKilled(Object killer) {
		super.EEKilled(killer);
		ZBerryMapRegistry.Register(ZBerryMapCategory.BODIES, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryMapRegistry.Unregister(ZBerryMapCategory.BODIES, this);
		super.EEDelete(parent);
	}
};
