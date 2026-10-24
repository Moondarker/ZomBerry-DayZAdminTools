//A loadout's items as listbox rows, nested ones indented, with their hotbar slots. Items the spawn rules don't allow
//and ones this server doesn't have are in red. The Spawn tab's loadouts and the Logs tab's loadout overview use it
class ZBerryLoadoutRows {
	static const int COLOR_RESTRICTED = 0xFFFF6060;

	//Recursive: parameters are copied into locals first, a nested call overwrote the caller's (see AGENTS.md)
	static void Add(TextListboxWidget listArg, ZBerryLoadoutItemArray itemsArg, int depthArg) {
		TextListboxWidget list = listArg;
		ZBerryLoadoutItemArray items = itemsArg;
		int depth = depthArg;
		string indent = "";
		for (int level = 0; level < depth; ++level) indent += "    ";

		foreach (ZBerryLoadoutItem item : items) {
			string text = indent + Describe(item);
			bool exists = ZBerrySpawner.IsSpawnableClass(item.Type);
			bool allowed = exists && GetZBerryClientAuth().CanSpawn(item.Type);
			if (!exists) {
				text += " - not on this server";
			} else if (!allowed) {
				text += " - not allowed";
			}
			int row = list.AddItem(text, NULL, 0);
			if (!allowed) list.SetItemColor(row, 0, COLOR_RESTRICTED);
			Add(list, item.Items, depth + 1);
		}
	}

	static string Describe(ZBerryLoadoutItem item) {
		string text = ZBerrySpawner.GetDisplayName(item.Type);
		if (item.Place == ZBerryLoadoutItem.PLACE_HANDS) text = "In hands: " + text;
		if (item.Ammo >= 0) text += ", " + item.Ammo + " rounds";

		float percent = item.Health;
		percent = percent * 100;
		int health = Math.Round(percent);
		if (health < 100) text += ", " + health + "% health";
		if (item.Wet > 0.05) text += ", wet";
		if (item.Hotbar >= 0) text += ", hotbar " + (item.Hotbar + 1);
		return text;
	}
};
