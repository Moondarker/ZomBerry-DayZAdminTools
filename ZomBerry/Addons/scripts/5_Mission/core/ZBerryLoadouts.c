//A round in a weapon itself: in a muzzle's chamber or its internal magazine (shotguns, bolt actions)
class ZBerryLoadoutRound {
	int Muzzle;
	bool Chamber; //False: the internal magazine
	string Ammo; //Bullet type (Bullet_*)
	float Damage;

	void ZBerryLoadoutRound(int muzzle = 0, bool chamber = true, string ammo = "", float damage = 0) {
		Muzzle = muzzle;
		Chamber = chamber;
		Ammo = ammo;
		Damage = damage;
	}
};

//One item of a loadout and what it holds, in the state it was saved in
class ZBerryLoadoutItem {
	static const int PLACE_ATTACHMENT = 0; //In SlotName of whatever holds it (the character: clothing, a weapon: its magazine)
	static const int PLACE_CARGO = 1; //In the holder's cargo, at Row/Col
	static const int PLACE_HANDS = 2; //The character's hands

	string Type;
	int Place;
	string SlotName; //Slot name for PLACE_ATTACHMENT
	int Cargo; //Which cargo of the holder, and where in it
	int Row;
	int Col;
	bool Flip;
	float Health = 1; //0..1 of the item's maximum
	float Wet;
	float Quantity = -1; //-1: the item has none
	int Ammo = -1; //Magazines and ammo piles, -1: not one
	int LiquidType; //Liquid containers, 0: none
	int FoodState = -1; //Food with stages, -1: none
	ref array<ref ZBerryLoadoutRound> Rounds = new array<ref ZBerryLoadoutRound>; //Weapons
	ref array<ref ZBerryLoadoutItem> Items = new array<ref ZBerryLoadoutItem>; //Attachments, then cargo

	//This item and everything in it
	int CountAll() {
		int count = 1;
		foreach (ZBerryLoadoutItem child : Items) count += child.CountAll();
		return count;
	}
};

typedef array<ref ZBerryLoadoutItem> ZBerryLoadoutItemArray;

//A set of gear: what a character wears and holds
class ZBerryLoadout {
	string Name;
	string OwnerId; //Shared loadouts and saved originals: who saved it (Steam64)
	string OwnerName;
	ref ZBerryLoadoutItemArray Items = new ZBerryLoadoutItemArray; //Clothing and the item in hands

	void ZBerryLoadout(string name = "") {
		Name = name;
	}

	int CountAll() {
		int count = 0;
		foreach (ZBerryLoadoutItem item : Items) count += item.CountAll();
		return count;
	}

	//Items the spawn rules don't allow, nested ones too. auth NULL: this client's rules (GetZBerryClientAuth)
	int CountRestricted(ZBerryAuth auth = NULL, PlayerIdentity identity = NULL) {
		int count = 0;
		foreach (ZBerryLoadoutItem item : Items) count += CountRestrictedIn(item, auth, identity);
		return count;
	}

	//Recursive: parameters are copied into locals first, a nested call overwrote the caller's (see AGENTS.md)
	protected static int CountRestrictedIn(ZBerryLoadoutItem itemArg, ZBerryAuth authArg, PlayerIdentity identityArg) {
		ZBerryLoadoutItem item = itemArg;
		ZBerryAuth auth = authArg;
		PlayerIdentity identity = identityArg;
		int count = 0;
		if (!ZBerryLoadoutKit.MaySpawn(item.Type, auth, identity)) ++count;
		foreach (ZBerryLoadoutItem child : item.Items) count += CountRestrictedIn(child, auth, identity);
		return count;
	}
};

typedef array<ref ZBerryLoadout> ZBerryLoadoutArray;

//Root of the loadout files: ZomBerryLoadouts.json (shared, server), ZomBerryLoadoutOriginals.json (gear saved before
//a try, server), ZomBerryMyLoadouts.json (an admin's own, client)
class ZBerryLoadoutsConfig {
	ref ZBerryLoadoutArray Loadouts = new ZBerryLoadoutArray;
};

//What an admin's list shows of a shared loadout; its items come with ZBerryRPCOp.LoadoutDetails when selected
class ZBerryLoadoutHeader {
	string Name;
	string OwnerName;
	int ItemCount;
	int Restricted; //Items this admin's spawn rules leave out
	bool Editable; //This admin may replace or delete it

	void ZBerryLoadoutHeader(string name = "", string ownerName = "", int itemCount = 0, int restricted = 0, bool editable = false) {
		Name = name;
		OwnerName = ownerName;
		ItemCount = itemCount;
		Restricted = restricted;
		Editable = editable;
	}
};

typedef array<ref ZBerryLoadoutHeader> ZBerryLoadoutHeaderArray;

//How many more items a capture or a client's loadout may have
class ZBerryLoadoutBudget {
	int Left;

	void ZBerryLoadoutBudget(int left) {
		Left = left;
	}
};

//What putting a loadout on did
class ZBerryLoadoutResult {
	int Placed;
	int Restricted; //Left out: the spawn rules don't allow them
	int Failed; //Unknown classes, or no room
};

//Replacing a character's gear with a loadout: strip, wait until the hands are empty (deleting what a character holds
//takes a moment), dress. OnDone gets the job once it's dressed, or gave up (Result says what happened)
class ZBerryLoadoutJob {
	static const int CHECK_INTERVAL = 100; //ms
	static const int MAX_CHECKS = 50;
	protected static ref array<ref ZBerryLoadoutJob> s_Running = new array<ref ZBerryLoadoutJob>; //Kept alive until done

	ref ScriptInvoker OnDone = new ScriptInvoker;
	ref ZBerryLoadoutResult Result = new ZBerryLoadoutResult;
	bool Dressed; //False: the character died or left, or the hands didn't empty
	int SessionId; //Of the admin, for whoever handles OnDone
	ref ZBerryLoadout Loadout;
	string Note; //For whoever handles OnDone

	protected PlayerBase m_Player; //Weak: NULL once the character is gone
	protected ZBerryAuth m_Auth;
	protected int m_Checks;

	//auth NULL: no spawn rules. identity: whose rules, looked up again by session id when dressing
	static ZBerryLoadoutJob Start(PlayerBase player, ZBerryLoadout loadout, ZBerryAuth auth = NULL, PlayerIdentity identity = NULL) {
		ZBerryLoadoutJob job = new ZBerryLoadoutJob;
		job.m_Player = player;
		job.Loadout = loadout;
		job.m_Auth = auth;
		if (identity) job.SessionId = identity.GetPlayerId();
		s_Running.Insert(job);

		ZBerryLoadoutKit.Strip(player);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(job.Check, CHECK_INTERVAL, false);
		return job;
	}

	protected void Check() {
		if (!m_Player || !m_Player.IsAlive()) {
			Finish();
			return;
		}
		if (m_Player.GetHumanInventory().GetEntityInHands() && m_Checks < MAX_CHECKS) {
			++m_Checks;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Check, CHECK_INTERVAL, false);
			return;
		}

		PlayerIdentity identity = NULL;
		if (GetGame().IsMultiplayer()) identity = ZBGetIdentityById(SessionId);
		Result = ZBerryLoadoutKit.Dress(m_Player, Loadout, m_Auth, identity);
		Dressed = true;
		Finish();
	}

	protected void Finish() {
		ZBerryLoadoutJob keep = this; //Alive until this returns, the list held the last ref
		OnDone.Invoke(keep);
		s_Running.RemoveItem(keep);
	}
};

//Saving gear into a loadout and putting a loadout on, server side
class ZBerryLoadoutKit {
	static const int MAX_ITEMS = 400;
	static const int MAX_DEPTH = 8;
	static const int MAX_ROUNDS = 100; //Per weapon
	static const int MAX_NAME_LENGTH = 40;
	static const int MAX_TYPE_LENGTH = 100;

	//auth NULL: this client's rules. Singleplayer: everything
	static bool MaySpawn(string type, ZBerryAuth auth, PlayerIdentity identity) {
		if (auth) return auth.CanSpawn(identity, type);
		return GetZBerryClientAuth().CanSpawn(type);
	}

	static string CleanName(string name) {
		name = name.Trim();
		if (name.Length() > MAX_NAME_LENGTH) name = name.Substring(0, MAX_NAME_LENGTH);
		return name.Trim();
	}

	//Names are compared without case and surrounding spaces
	static string GetKey(string name) {
		string key = name.Trim();
		key.ToLower();
		return key;
	}

	static ZBerryLoadout Find(ZBerryLoadoutArray loadouts, string name) {
		string key = GetKey(name);
		foreach (ZBerryLoadout loadout : loadouts) {
			if (GetKey(loadout.Name) == key) return loadout;
		}
		return NULL;
	}

	//Everything the character wears and holds
	static ZBerryLoadout Capture(PlayerBase player, string name) {
		ZBerryLoadout loadout = new ZBerryLoadout(name);
		ZBerryLoadoutBudget budget = new ZBerryLoadoutBudget(MAX_ITEMS);

		GameInventory inventory = player.GetInventory();
		for (int i = 0; i < inventory.AttachmentCount(); ++i) {
			EntityAI attachment = inventory.GetAttachmentFromIndex(i);
			ZBerryLoadoutItem worn = CaptureItem(attachment, 0, budget);
			if (worn) loadout.Items.Insert(worn);
		}

		EntityAI inHands = player.GetHumanInventory().GetEntityInHands();
		ZBerryLoadoutItem held = CaptureItem(inHands, 0, budget);
		if (held) loadout.Items.Insert(held);
		return loadout;
	}

	//Recursive: parameters are copied into locals first, a nested call overwrote the caller's (see AGENTS.md)
	protected static ZBerryLoadoutItem CaptureItem(EntityAI entityArg, int depthArg, ZBerryLoadoutBudget budgetArg) {
		EntityAI entity = entityArg;
		int depth = depthArg;
		ZBerryLoadoutBudget budget = budgetArg;
		if (!entity || depth >= MAX_DEPTH || budget.Left <= 0) return NULL;
		--budget.Left;

		ZBerryLoadoutItem item = new ZBerryLoadoutItem;
		item.Type = entity.GetType();

		InventoryLocation location = new InventoryLocation;
		entity.GetInventory().GetCurrentInventoryLocation(location);
		switch (location.GetType()) {
			case InventoryLocationType.ATTACHMENT:
				item.Place = ZBerryLoadoutItem.PLACE_ATTACHMENT;
				item.SlotName = InventorySlots.GetSlotName(location.GetSlot());
				break;
			case InventoryLocationType.CARGO:
				item.Place = ZBerryLoadoutItem.PLACE_CARGO;
				item.Cargo = location.GetIdx();
				item.Row = location.GetRow();
				item.Col = location.GetCol();
				item.Flip = location.GetFlip();
				break;
			default:
				item.Place = ZBerryLoadoutItem.PLACE_HANDS;
				break;
		}

		item.Health = entity.GetHealth01("", "");
		ItemBase itemBase = ItemBase.Cast(entity);
		Magazine magazine = Magazine.Cast(entity);
		if (magazine) {
			item.Ammo = magazine.GetAmmoCount();
		} else if (itemBase && itemBase.HasQuantity()) {
			item.Quantity = itemBase.GetQuantity();
		}
		if (itemBase) {
			item.Wet = itemBase.GetWet();
			item.LiquidType = itemBase.GetLiquidType();
		}
		Edible_Base edible = Edible_Base.Cast(entity);
		if (edible && edible.HasFoodStage() && edible.GetFoodStage()) item.FoodState = edible.GetFoodStageType();

		Weapon_Base weapon = Weapon_Base.Cast(entity);
		if (weapon) CaptureRounds(weapon, item);

		GameInventory inventory = entity.GetInventory();
		for (int i = 0; i < inventory.AttachmentCount(); ++i) {
			ZBerryLoadoutItem attachment = CaptureItem(inventory.GetAttachmentFromIndex(i), depth + 1, budget);
			if (attachment) item.Items.Insert(attachment);
		}

		CargoBase cargo = inventory.GetCargo();
		if (cargo) {
			for (int j = 0; j < cargo.GetItemCount(); ++j) {
				ZBerryLoadoutItem content = CaptureItem(cargo.GetItem(j), depth + 1, budget);
				if (content) item.Items.Insert(content);
			}
		}
		return item;
	}

	protected static void CaptureRounds(Weapon_Base weapon, ZBerryLoadoutItem item) {
		for (int muzzle = 0; muzzle < weapon.GetMuzzleCount(); ++muzzle) {
			float damage;
			string ammo;
			if (!weapon.IsChamberEmpty(muzzle) && weapon.GetCartridgeInfo(muzzle, damage, ammo)) item.Rounds.Insert(new ZBerryLoadoutRound(muzzle, true, ammo, damage));

			for (int index = 0; index < weapon.GetInternalMagazineCartridgeCount(muzzle) && item.Rounds.Count() < MAX_ROUNDS; ++index) {
				if (weapon.GetInternalMagazineCartridgeInfo(muzzle, index, damage, ammo)) item.Rounds.Insert(new ZBerryLoadoutRound(muzzle, false, ammo, damage));
			}
		}
	}

	//Takes off everything the character wears and holds. The item in hands goes a moment later: Dress once it has
	static void Strip(PlayerBase player) {
		EntityAI inHands = player.GetHumanInventory().GetEntityInHands(); //RemoveAllItems leaves it
		if (inHands) player.GetHumanInventory().LocalDestroyEntity(inHands);
		player.RemoveAllItems();
	}

	//Puts a loadout on a stripped character. auth NULL: no spawn rules (their own saved gear)
	static ZBerryLoadoutResult Dress(PlayerBase player, ZBerryLoadout loadout, ZBerryAuth auth = NULL, PlayerIdentity identity = NULL) {
		ZBerryLoadoutResult result = new ZBerryLoadoutResult;

		//Clothing before the item in hands, so it can't take a slot a piece of clothing needs. Items inside left-out
		//ones go wherever there's room, once everything else is on
		ZBerryLoadoutItemArray orphans = new ZBerryLoadoutItemArray;
		foreach (ZBerryLoadoutItem worn : loadout.Items) {
			if (worn.Place != ZBerryLoadoutItem.PLACE_HANDS) SpawnItem(player, player, worn, auth, identity, result, orphans);
		}
		foreach (ZBerryLoadoutItem held : loadout.Items) {
			if (held.Place == ZBerryLoadoutItem.PLACE_HANDS) SpawnItem(player, player, held, auth, identity, result, orphans);
		}
		for (int i = 0; i < orphans.Count(); ++i) SpawnItem(player, NULL, orphans[i], auth, identity, result, orphans); //Grows while it runs

		return result;
	}

	//holder NULL: anywhere in the character's inventory
	//Recursive: parameters are copied into locals first, a nested call overwrote the caller's (see AGENTS.md)
	protected static void SpawnItem(PlayerBase playerArg, EntityAI holderArg, ZBerryLoadoutItem itemArg, ZBerryAuth authArg, PlayerIdentity identityArg, ZBerryLoadoutResult resultArg, ZBerryLoadoutItemArray orphansArg) {
		PlayerBase player = playerArg;
		EntityAI holder = holderArg;
		ZBerryLoadoutItem item = itemArg;
		ZBerryAuth auth = authArg;
		PlayerIdentity identity = identityArg;
		ZBerryLoadoutResult result = resultArg;
		ZBerryLoadoutItemArray orphans = orphansArg;
		if (!ZBerrySpawner.IsSpawnableClass(item.Type)) {
			result.Failed += item.CountAll();
			return;
		}
		if (auth && !auth.CanSpawn(identity, item.Type)) {
			++result.Restricted;
			foreach (ZBerryLoadoutItem orphan : item.Items) orphans.Insert(orphan);
			return;
		}

		EntityAI entity = Create(player, holder, item);
		if (!entity) {
			result.Failed += item.CountAll();
			return;
		}
		++result.Placed;

		//Attachments first: a backpack's or vest's pouches before what goes in them
		foreach (ZBerryLoadoutItem attachment : item.Items) {
			if (attachment.Place == ZBerryLoadoutItem.PLACE_ATTACHMENT) SpawnItem(player, entity, attachment, auth, identity, result, orphans);
		}
		foreach (ZBerryLoadoutItem content : item.Items) {
			if (content.Place != ZBerryLoadoutItem.PLACE_ATTACHMENT) SpawnItem(player, entity, content, auth, identity, result, orphans);
		}

		SetState(entity, item);
	}

	//Where it was, or anywhere it fits in the holder, or anywhere on the character
	protected static EntityAI Create(PlayerBase player, EntityAI holder, ZBerryLoadoutItem item) {
		EntityAI entity;
		if (holder == player && item.Place == ZBerryLoadoutItem.PLACE_HANDS) {
			entity = player.GetHumanInventory().CreateInHands(item.Type);
		} else if (holder && item.Place == ZBerryLoadoutItem.PLACE_ATTACHMENT) {
			int slotId = InventorySlots.GetSlotIdFromString(item.SlotName);
			if (slotId != InventorySlots.INVALID) entity = holder.GetInventory().CreateAttachmentEx(item.Type, slotId);
		} else if (holder && item.Place == ZBerryLoadoutItem.PLACE_CARGO && holder.GetInventory().GetCargo()) {
			entity = holder.GetInventory().CreateEntityInCargoEx(item.Type, item.Cargo, item.Row, item.Col, item.Flip);
		}

		if (!entity && holder) entity = holder.GetInventory().CreateInInventory(item.Type);
		if (!entity) entity = player.GetInventory().CreateInInventory(item.Type);
		return entity;
	}

	protected static void SetState(EntityAI entity, ZBerryLoadoutItem item) {
		ItemBase itemBase = ItemBase.Cast(entity);
		Magazine magazine = Magazine.Cast(entity);
		if (magazine && item.Ammo >= 0) {
			magazine.ServerSetAmmoCount(Math.Min(item.Ammo, magazine.GetAmmoMax()));
		} else if (itemBase && itemBase.HasQuantity() && item.Quantity >= 0) {
			if (item.LiquidType != 0) itemBase.SetLiquidType(item.LiquidType);
			itemBase.SetQuantity(item.Quantity);
		}
		if (itemBase) itemBase.SetWet(item.Wet);

		Edible_Base edible = Edible_Base.Cast(entity);
		if (edible && item.FoodState >= 0 && edible.HasFoodStage() && edible.GetFoodStage() && edible.GetFoodStageType() != item.FoodState) edible.ChangeFoodStage(item.FoodState);

		Weapon_Base weapon = Weapon_Base.Cast(entity);
		if (weapon) SetRounds(weapon, item);

		entity.SetHealth01("", "", item.Health); //Last: a ruined container would refuse its contents
	}

	//The chambers and internal magazines as saved. The weapon's state machine is told what it holds now
	protected static void SetRounds(Weapon_Base weapon, ZBerryLoadoutItem item) {
		float damage;
		string ammo;
		for (int muzzle = 0; muzzle < weapon.GetMuzzleCount(); ++muzzle) {
			if (!weapon.IsChamberEmpty(muzzle)) weapon.PopCartridgeFromChamber(muzzle, damage, ammo);
			while (weapon.GetInternalMagazineCartridgeCount(muzzle) > 0) {
				if (!weapon.PopCartridgeFromInternalMagazine(muzzle, damage, ammo)) break;
			}
		}

		foreach (ZBerryLoadoutRound round : item.Rounds) {
			if (round.Muzzle < 0 || round.Muzzle >= weapon.GetMuzzleCount() || !ZBerrySpawner.IsSpawnableAmmo(round.Ammo)) continue;
			if (round.Chamber) {
				weapon.PushCartridgeToChamber(round.Muzzle, round.Damage, round.Ammo);
			} else {
				weapon.PushCartridgeToInternalMagazine(round.Muzzle, round.Damage, round.Ammo);
			}
		}

		weapon.RandomizeFSMState();
		weapon.Synchronize();
	}

	//A loadout sent by a client: trimmed to the limits, the name cleaned. False when nothing is left of it
	static bool Sanitize(ZBerryLoadout loadout) {
		if (!loadout) return false;
		loadout.Name = CleanName(loadout.Name);
		if (!loadout.Items) loadout.Items = new ZBerryLoadoutItemArray;

		ZBerryLoadoutBudget budget = new ZBerryLoadoutBudget(MAX_ITEMS);
		SanitizeItems(loadout.Items, 0, budget);
		return loadout.Items.Count() > 0;
	}

	//Recursive: parameters are copied into locals first, a nested call overwrote the caller's (see AGENTS.md)
	protected static void SanitizeItems(ZBerryLoadoutItemArray itemsArg, int depthArg, ZBerryLoadoutBudget budgetArg) {
		ZBerryLoadoutItemArray items = itemsArg;
		int depth = depthArg;
		ZBerryLoadoutBudget budget = budgetArg;
		for (int i = 0; i < items.Count(); ) {
			ZBerryLoadoutItem item = items[i];
			if (!item || depth >= MAX_DEPTH || budget.Left <= 0 || item.Type == "" || item.Type.Length() > MAX_TYPE_LENGTH) {
				items.RemoveOrdered(i);
				continue;
			}
			--budget.Left;

			item.Place = Math.Clamp(item.Place, ZBerryLoadoutItem.PLACE_ATTACHMENT, ZBerryLoadoutItem.PLACE_HANDS);
			if (depth > 0 && item.Place == ZBerryLoadoutItem.PLACE_HANDS) item.Place = ZBerryLoadoutItem.PLACE_CARGO;
			item.Health = Math.Clamp(item.Health, 0, 1);
			item.Wet = Math.Clamp(item.Wet, 0, 1);
			if (!item.Rounds) item.Rounds = new array<ref ZBerryLoadoutRound>;
			while (item.Rounds.Count() > MAX_ROUNDS) item.Rounds.Remove(item.Rounds.Count() - 1);
			if (!item.Items) item.Items = new ZBerryLoadoutItemArray;

			SanitizeItems(item.Items, depth + 1, budget);
			++i;
		}
	}
};
