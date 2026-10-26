//Entity categories tracked by ZBerryEntityRegistry. The first MARKER_COUNT are the Map tab's object markers
class ZBerryEntityCategory {
	static const int VEHICLES = 0;
	static const int TENTS = 1;
	static const int STORAGE = 2; //Barrels, chests, crates, buried stashes
	static const int BASES = 3; //Fences, watchtowers, flags
	static const int BODIES = 4; //Dead players
	static const int MARKER_COUNT = 5;

	static const int PLAYERS = 5; //Living players
	static const int INFECTED = 6;
	static const int ANIMALS = 7;
	static const int COUNT = 8;

	static TStringArray GetKeys() {
		return {"vehicles", "tents", "storage", "bases", "bodies", "players", "infected", "animals"};
	}

	static string GetKey(int category) {
		TStringArray keys = GetKeys();
		if (category < 0 || category >= keys.Count()) return "";
		return keys[category];
	}
};

//The entities of each category this side knows about. Entities add and remove themselves (modded classes below), so nobody
//has to scan the world: on the server that's every object, including those loaded from persistence (they go through
//EEInit too); on a client it's what its network bubble holds. Map markers use the server's, ESP the client's
class ZBerryEntityRegistry {
	protected static ref array<ref array<EntityAI>> s_Entities;

	protected static array<EntityAI> GetList(int category) {
		if (category < 0 || category >= ZBerryEntityCategory.COUNT) return NULL;

		if (!s_Entities) {
			s_Entities = new array<ref array<EntityAI>>;
			for (int i = 0; i < ZBerryEntityCategory.COUNT; ++i) s_Entities.Insert(new array<EntityAI>);
		}
		return s_Entities[category];
	}

	static void Register(int category, EntityAI entity) {
		if (!entity) return;

		array<EntityAI> list = GetList(category);
		if (list && list.Find(entity) == -1) list.Insert(entity);
	}

	static void Unregister(int category, EntityAI entity) {
		if (!entity) return;

		array<EntityAI> list = GetList(category);
		if (list) list.RemoveItem(entity);
	}

	//The live entities of a category (deleted ones are dropped on the way)
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
		ZBerryEntityRegistry.Register(ZBerryEntityCategory.VEHICLES, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.VEHICLES, this);
		super.EEDelete(parent);
	}
};

modded class BoatScript {
	override void EEInit() {
		super.EEInit();
		ZBerryEntityRegistry.Register(ZBerryEntityCategory.VEHICLES, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.VEHICLES, this);
		super.EEDelete(parent);
	}
};

//Packed tents register too: whether a tent is pitched is checked when markers are built
modded class TentBase {
	override void EEInit() {
		super.EEInit();
		ZBerryEntityRegistry.Register(ZBerryEntityCategory.TENTS, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.TENTS, this);
		super.EEDelete(parent);
	}
};

modded class DeployableContainer_Base {
	override void EEInit() {
		super.EEInit();
		ZBerryEntityRegistry.Register(ZBerryEntityCategory.STORAGE, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.STORAGE, this);
		super.EEDelete(parent);
	}
};

modded class UndergroundStash {
	override void EEInit() {
		super.EEInit();
		ZBerryEntityRegistry.Register(ZBerryEntityCategory.STORAGE, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.STORAGE, this);
		super.EEDelete(parent);
	}
};

modded class BaseBuildingBase {
	override void EEInit() {
		super.EEInit();
		ZBerryEntityRegistry.Register(ZBerryEntityCategory.BASES, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.BASES, this);
		super.EEDelete(parent);
	}
};

//Player deaths and hits for whoever listens (the admin log), server side
class ZBerryPlayerFeed {
	static ref ScriptInvoker OnPlayerDying = new ScriptInvoker(); //(PlayerBase victim, Object killer): before vanilla's death handling, gear and hotbar still as they were
	static ref ScriptInvoker OnPlayerKilled = new ScriptInvoker(); //(PlayerBase victim, Object killer)
	static ref ScriptInvoker OnPlayerHit = new ScriptInvoker(); //(PlayerBase victim, PlayerBase attacker, EntityAI source, float damage): player hits only
};

//Living players until they die, then bodies until they're gone
modded class PlayerBase {
	override void EEInit() {
		super.EEInit();
		if (IsAlive()) ZBerryEntityRegistry.Register(ZBerryEntityCategory.PLAYERS, this);
	}

	override void EEKilled(Object killer) {
		if (GetGame().IsServer()) ZBerryPlayerFeed.OnPlayerDying.Invoke(this, killer);
		super.EEKilled(killer);
		if (GetGame().IsServer()) ZBerryPlayerFeed.OnPlayerKilled.Invoke(this, killer);
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.PLAYERS, this);
		ZBerryEntityRegistry.Register(ZBerryEntityCategory.BODIES, this);
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef) {
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		if (!GetGame().IsServer() || !source || !damageResult) return;

		PlayerBase attacker = PlayerBase.Cast(source.GetHierarchyRootPlayer());
		if (attacker && attacker != this) ZBerryPlayerFeed.OnPlayerHit.Invoke(this, attacker, source, damageResult.GetDamage("", "Health"));
	}

	override void EEDelete(EntityAI parent) {
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.PLAYERS, this);
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.BODIES, this);
		super.EEDelete(parent);
	}
};

modded class ZombieBase {
	override void EEInit() {
		super.EEInit();
		ZBerryEntityRegistry.Register(ZBerryEntityCategory.INFECTED, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.INFECTED, this);
		super.EEDelete(parent);
	}
};

modded class AnimalBase {
	override void EEInit() {
		super.EEInit();
		ZBerryEntityRegistry.Register(ZBerryEntityCategory.ANIMALS, this);
	}

	override void EEDelete(EntityAI parent) {
		ZBerryEntityRegistry.Unregister(ZBerryEntityCategory.ANIMALS, this);
		super.EEDelete(parent);
	}
};
