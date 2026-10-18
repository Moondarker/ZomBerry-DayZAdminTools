//Spawning from the Spawn tab: on the ground at a position, or into the admin's own inventory
class ZBerrySpawner {
	//Returns a log line describing what happened
	static string Spawn(string className, PlayerBase admin, vector position, bool insideInventory) {
		if (!IsSpawnableClass(className)) return "tried to spawn unknown class " + className;

		if (insideInventory) {
			if (!admin) return "tried to add " + className + " to inventory, but has no character";

			if (TryLoadMagazine(admin, className)) return "loaded " + className + " into the weapon in their hands";

			EntityAI entity = admin.GetInventory().CreateInInventory(className);
			if (!entity) {
				string reason = "it can't be carried";
				if (IsCarryableClass(className)) reason = "no room in inventory";

				ZomberryStockFunctions.MessagePlayer(admin, "Can't add " + className + " to inventory: " + reason);
				return "couldn't add " + className + " to their inventory (" + reason + ")";
			}

			ItemBase item = ItemBase.Cast(entity);
			if (item) item.SetQuantity(item.GetQuantityMax());
			return "added " + className + " to their inventory";
		}

		GetGame().CreateObject(className, position, false, true );
		return "spawned " + className + " at " + position;
	}

	//Same configs the Spawn tab lists
	static bool IsSpawnableClass(string className) {
		if (className == "") return false;

		return GetGame().ConfigIsExisting(CFG_VEHICLESPATH + " " + className) || GetGame().ConfigIsExisting(CFG_WEAPONSPATH + " " + className) || GetGame().ConfigIsExisting(CFG_MAGAZINESPATH + " " + className);
	}

	//Checked in config: many items (SaltyMix, helmets...) have no script class of their own to check instead
	static bool IsCarryableClass(string className) {
		if (GetGame().ConfigIsExisting(CFG_WEAPONSPATH + " " + className) || GetGame().ConfigIsExisting(CFG_MAGAZINESPATH + " " + className)) return true;

		return GetGame().IsKindOf(className, "Inventory_Base");
	}

	//Attachments already land on the weapon by themselves (CreateInInventory picks free attachment slots),
	//magazines need the weapon to load them: do it when the weapon in hands takes this magazine and has none
	static Magazine TryLoadMagazine(PlayerBase admin, string className) {
		Weapon_Base weapon = Weapon_Base.Cast(admin.GetHumanInventory().GetEntityInHands());
		if (!weapon || weapon.GetMagazine(weapon.GetCurrentMuzzle())) return NULL;

		TStringArray magazines = new TStringArray;
		GetGame().ConfigGetTextArray(CFG_WEAPONSPATH + " " + weapon.GetType() + " magazines", magazines);

		string classNameLower = className;
		classNameLower.ToLower();
		foreach (string magazine : magazines) {
			magazine.ToLower();
			if (magazine == classNameLower) return weapon.SpawnAttachedMagazine(className, WeaponWithAmmoFlags.NONE);
		}

		return NULL;
	}
};
