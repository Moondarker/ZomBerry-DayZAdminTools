//ESP overlay (#59), switched on and off by the server's ESP function through the "zomberry_esp" client feature.
//What it draws is up to this client (ZBerryESPSettings, the ESP tab). It draws what this client's network bubble holds
//(ZBerryEntityRegistry): players with box, skeleton up close and a label, vehicles with box and name, infected and
//animals with a box, loot on the ground with a small square and name. Players outside the bubble come from the
//streams as labels
class ZBerryESPOverlay {
	static const string FEATURE = "zomberry_esp";
	static const int LABEL_COUNT = 100; //Loot is drawn last: players and vehicles get labels first
	static const float SKELETON_DISTANCE = 150; //Metres

	static const int COLOR_PLAYER = 0xFF55AAFF;
	static const int COLOR_ADMIN = 0xFF47EB00;
	static const int COLOR_FAR_PLAYER = 0xFFB0B0FF;
	static const int COLOR_VEHICLE = 0xFFFFC34D;
	static const int COLOR_INFECTED = 0xFFFF5A5A;
	static const int COLOR_ANIMAL = 0xFF8FD18F;
	static const int COLOR_LOOT = 0xFFD8D8D8;

	static const int LOOT_SCAN_MS = 1000; //Loot isn't registered anywhere (there's too much of it): looked for this often
	static const int LOOT_LIMIT = 300; //Nearest items kept per look

	protected Widget m_Root;
	protected CanvasWidget m_Canvas;
	protected Man m_OwnBody; //The character this client controls or last controlled. Weak: NULL once deleted
	protected ref array<TextWidget> m_Labels = new array<TextWidget>;
	protected int m_LabelsUsed;

	protected bool m_Enabled;
	protected ref array<ItemBase> m_Loot = new array<ItemBase>; //Nearest first. Weak: NULL once an item is deleted
	protected int m_LootScanTime = -LOOT_SCAN_MS;

	void ZBerryESPOverlay() {
		GetZomberryCmdAPI().RegisterClientFeature(FEATURE, this, "OnFeature");
		GetZBerryESPSettings().OnChanged.Insert(UpdateStreams);
	}

	void ~ZBerryESPOverlay() {
		if (m_Root) m_Root.Unlink();
	}

	protected bool CreateWidgets() {
		if (m_Root) return true;

		m_Root = GetGame().GetWorkspace().CreateWidgets( "ZomBerry/AT/scripts/5_Mission/layouts/ESPOverlay.layout" );
		if (!m_Root) {
			ZomberryBase.Log( "ZomBerryUI", "ERROR: Can't load ESPOverlay.layout" );
			return false;
		}

		m_Canvas = CanvasWidget.Cast( m_Root.FindAnyWidget("ESPCanvas") );
		for (int i = 0; i < LABEL_COUNT; ++i) {
			TextWidget label = TextWidget.Cast( m_Root.FindAnyWidget("ESPLabel" + i.ToString()) );
			if (label) m_Labels.Insert(label);
		}
		return m_Canvas != NULL;
	}

	void OnFeature(bool enabled, TIntArray args) {
		m_Enabled = enabled;
		UpdateStreams();

		if (!m_Enabled) {
			if (m_Root) m_Root.Show(false);
			return;
		}
		if (CreateWidgets()) m_Root.Show(true);
	}

	bool IsEnabled() {
		return m_Enabled;
	}

	//Everyone's position and health come from the server only while they're drawn
	protected void UpdateStreams() {
		ZBerryESPSettingsData settings = GetZBerryESPSettings().Get();
		GetZBerryStreams().SetEspPlayers(m_Enabled && (settings.FarPlayers || settings.PlayerHealth));
	}

	//Every frame, from MissionGameplay
	void Update() {
		if (!m_Enabled || !m_Canvas) return;

		m_Canvas.Clear();
		m_LabelsUsed = 0;

		float scale = ZBerryUIUtils.GetUIScale();
		float lineWidth = Math.Max(1, 1.5 * scale);
		vector cameraPos = GetGame().GetCurrentCameraPosition();

		ZBerryESPSettingsData settings = GetZBerryESPSettings().Get();
		DrawPlayers(settings, cameraPos, lineWidth);
		if (settings.Vehicles) DrawEntities(ZBerryEntityCategory.VEHICLES, cameraPos, settings.MaxDistance, COLOR_VEHICLE, lineWidth, true);
		if (settings.Infected) DrawEntities(ZBerryEntityCategory.INFECTED, cameraPos, settings.AIDistance, COLOR_INFECTED, lineWidth, false);
		if (settings.Animals) DrawEntities(ZBerryEntityCategory.ANIMALS, cameraPos, settings.AIDistance, COLOR_ANIMAL, lineWidth, false);
		if (settings.Loot) DrawLoot(cameraPos, settings.LootDistance, lineWidth);

		for (int i = m_LabelsUsed; i < m_Labels.Count(); ++i) m_Labels[i].Show(false);
	}

	protected void DrawPlayers(ZBerryESPSettingsData settings, vector cameraPos, float lineWidth) {
		bool nearShown = settings.PlayerBox || settings.PlayerSkeleton || settings.PlayerName || settings.PlayerDistance || settings.PlayerHealth;
		if (!nearShown && !settings.FarPlayers) return;

		ZBerryStreamClient streams = GetZBerryStreams();
		Man localPlayer = GetGame().GetPlayer();
		if (localPlayer) m_OwnBody = localPlayer; //Remembered: in FreeCam and while spectating this client has no character
		map<int, bool> drawnIds = new map<int, bool>;

		array<EntityAI> players = ZBerryEntityRegistry.GetEntities(ZBerryEntityCategory.PLAYERS);
		foreach (EntityAI entity : players) {
			PlayerBase player = PlayerBase.Cast(entity);
			if (!player || !player.IsAlive()) continue;

			int id = -1;
			string name = "Player";
			if (player.GetIdentity()) {
				id = player.GetIdentity().GetPlayerId();
				name = player.GetIdentity().GetName();
			}
			drawnIds.Set(id, true);
			//The view's owner isn't marked: this client's own body, and while spectating the player watched
			if (player == m_OwnBody || ZomBerryCamSpec.IsTarget(player)) continue;
			if (!nearShown) continue;

			int color = COLOR_PLAYER;
			if (streams.IsAdmin(id)) color = COLOR_ADMIN;

			float distance = vector.Distance(cameraPos, player.GetPosition());
			if (distance > settings.MaxDistance) continue;
			float left, top, right, bottom;
			if (!ZBerryESPDraw.GetScreenBox(player, left, top, right, bottom)) continue;

			if (settings.PlayerBox) ZBerryESPDraw.DrawBox(m_Canvas, left, top, right, bottom, color, lineWidth);
			if (settings.PlayerSkeleton && distance <= SKELETON_DISTANCE) ZBerryESPDraw.DrawSkeleton(m_Canvas, player, color, lineWidth);

			string text = DescribePlayer(settings, id, name, distance);
			if (text != "") AddLabel((left + right) / 2, bottom, text, color);
		}
		if (!settings.FarPlayers) return;

		//Outside this client's network bubble: only what the server streams (position on the surface)
		TIntArray ids = streams.GetPlayerIds();
		foreach (int streamedId : ids) {
			if (drawnIds.Contains(streamedId) || !streams.HasPosition(streamedId)) continue;
			if (localPlayer && localPlayer.GetIdentity() && localPlayer.GetIdentity().GetPlayerId() == streamedId) continue;

			vector position = streams.GetPosition(streamedId) + "0 1.8 0";
			float x, y;
			if (!ZBerryESPDraw.ToScreen(position, x, y)) continue;

			float farDistance = vector.Distance(cameraPos, position);
			AddLabel(x, y, streams.GetName(streamedId) + " " + Math.Round(farDistance) + "m", COLOR_FAR_PLAYER);
		}
	}

	//The parts of a near player's label the settings ask for, "" for none
	protected string DescribePlayer(ZBerryESPSettingsData settings, int id, string name, float distance) {
		string text = "";
		if (settings.PlayerName) text = name;
		if (settings.PlayerDistance) {
			if (text != "") text += " ";
			text = text + Math.Round(distance) + "m"; //Like the far labels: string + float prints whole metres
		}

		int health, blood;
		if (settings.PlayerHealth && GetZBerryStreams().GetHealth(id, health, blood)) {
			if (text != "") text += " | ";
			text += health.ToString() + " HP, " + blood.ToString() + " blood";
		}
		return text;
	}

	protected void DrawEntities(int category, vector cameraPos, float maxDistance, int color, float lineWidth, bool withName) {
		array<EntityAI> entities = ZBerryEntityRegistry.GetEntities(category);
		foreach (EntityAI entity : entities) {
			if (!entity.IsAlive() && category != ZBerryEntityCategory.VEHICLES) continue;

			float distance = vector.Distance(cameraPos, entity.GetPosition());
			if (distance > maxDistance) continue;

			float left, top, right, bottom;
			if (!ZBerryESPDraw.GetScreenBox(entity, left, top, right, bottom)) continue;

			ZBerryESPDraw.DrawBox(m_Canvas, left, top, right, bottom, color, lineWidth);
			if (withName) AddLabel((left + right) / 2, bottom, entity.GetDisplayName() + " " + Math.Round(distance) + "m", color);
		}
	}

	//A small square on each item, and its name while labels are left
	protected void DrawLoot(vector cameraPos, float maxDistance, float lineWidth) {
		if (GetGame().GetTime() - m_LootScanTime >= LOOT_SCAN_MS) ScanLoot(cameraPos, maxDistance);

		float half = 3 * ZBerryUIUtils.GetUIScale();
		foreach (ItemBase item : m_Loot) {
			if (!item || item.GetHierarchyParent()) continue; //Picked up since the last look

			vector position = item.GetPosition();
			float distance = vector.Distance(cameraPos, position);
			if (distance > maxDistance) continue;

			float x, y;
			if (!ZBerryESPDraw.ToScreen(position, x, y)) continue;
			ZBerryESPDraw.DrawBox(m_Canvas, x - half, y - half, x + half, y + half, COLOR_LOOT, lineWidth);
			AddLabel(x, y + half, item.GetDisplayName() + " " + Math.Round(distance) + "m", COLOR_LOOT);
		}
	}

	//Items on the ground around the camera, nearest first, at most LOOT_LIMIT
	protected void ScanLoot(vector cameraPos, float maxDistance) {
		m_LootScanTime = GetGame().GetTime();
		m_Loot.Clear();
		TFloatArray distances = new TFloatArray;

		array<Object> objects = new array<Object>;
		GetGame().GetObjectsAtPosition3D(cameraPos, maxDistance, objects, NULL);
		foreach (Object obj : objects) {
			ItemBase item = ItemBase.Cast(obj);
			if (!IsGroundLoot(item)) continue;

			float distance = vector.Distance(cameraPos, item.GetPosition());
			int index = 0;
			while (index < distances.Count() && distances[index] <= distance) ++index;
			if (index >= LOOT_LIMIT) continue;

			distances.InsertAt(distance, index);
			m_Loot.InsertAt(item, index);
			if (m_Loot.Count() > LOOT_LIMIT) {
				distances.Remove(LOOT_LIMIT);
				m_Loot.Remove(LOOT_LIMIT);
			}
		}
	}

	//Lying loose on the ground: not in anyone's inventory or a container, and not something built or placed (bases,
	//tents, barrels and crates have their own map markers)
	protected static bool IsGroundLoot(ItemBase item) {
		if (!item || item.GetHierarchyParent()) return false;
		return !item.IsInherited(BaseBuildingBase) && !item.IsInherited(TentBase) && !item.IsInherited(DeployableContainer_Base);
	}

	//Centred under (x, y), from a fixed pool
	protected void AddLabel(float x, float y, string text, int color) {
		if (m_LabelsUsed >= m_Labels.Count()) return;

		TextWidget label = m_Labels[m_LabelsUsed];
		++m_LabelsUsed;

		label.SetText(text);
		label.SetColor(color);
		float width, height;
		label.GetScreenSize(width, height);
		label.SetPos(x - width / 2, y + 2);
		label.Show(true);
	}
};
