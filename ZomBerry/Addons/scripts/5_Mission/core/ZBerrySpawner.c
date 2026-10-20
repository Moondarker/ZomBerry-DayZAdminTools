//Spawning from the Spawn tab: on the ground at a position, or into an inventory (the admin's own or the selected player's)
class ZBerrySpawner {
	//Where the Spawn tab puts things (SpawnObject's mode)
	static const int AT_POSITION = 0; //Cursor or the admin's feet: the client sends the position
	static const int OWN_INVENTORY = 1;
	static const int PLAYER_FEET = 2; //The selected player's
	static const int PLAYER_INVENTORY = 3;

	//Returns a log line describing what happened. recipient: whose inventory, notify: who hears about problems (the admin)
	static string Spawn(string className, PlayerBase recipient, vector position, bool insideInventory, PlayerBase notify = NULL) {
		if (!IsSpawnableClass(className)) return "tried to spawn unknown class " + className;
		if (!notify) notify = recipient;

		if (insideInventory) {
			if (!recipient) return "tried to add " + className + " to an inventory, but there's no character";

			if (TryLoadMagazine(recipient, className)) return "loaded " + className + " into the weapon in " + DescribeOwner(recipient, notify) + " hands";

			EntityAI entity = recipient.GetInventory().CreateInInventory(className);
			if (!entity) {
				string reason = "it can't be carried";
				if (IsCarryableClass(className)) reason = "no room in inventory";

				ZomberryStockFunctions.MessagePlayer(notify, "Can't add " + className + " to inventory: " + reason);
				return "couldn't add " + className + " to " + DescribeOwner(recipient, notify) + " inventory (" + reason + ")";
			}

			ItemBase item = ItemBase.Cast(entity);
			if (item) item.SetQuantity(item.GetQuantityMax());
			return "added " + className + " to " + DescribeOwner(recipient, notify) + " inventory";
		}

		GetGame().CreateObject(className, position, false, true );
		return "spawned " + className + " at " + position;
	}

	//"their" for the admin's own, "Name (Steam64)'s" for someone else's, for log lines
	protected static string DescribeOwner(PlayerBase recipient, PlayerBase admin) {
		if (recipient == admin) return "their";
		return ZBDescribePlayer(recipient.GetIdentity()) + "'s";
	}

	//The name players see, from whichever config has the class; the class name when there's none
	static string GetDisplayName(string className) {
		TStringArray configs = {CFG_VEHICLESPATH, CFG_WEAPONSPATH, CFG_MAGAZINESPATH};
		foreach (string configPath : configs) {
			string displayName;
			if (GetGame().ConfigGetText(configPath + " " + className + " displayName", displayName) && displayName != "") return displayName;
		}
		return className;
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
