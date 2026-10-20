//ESP overlay (#59), switched by the server's ESP function through the "zomberry_esp" client feature.
//Draws what this client's network bubble holds (ZBerryEntityRegistry): players with box, skeleton up close and a label,
//vehicles with box and name, infected and animals with a box. Players outside the bubble come from the streams as labels
class ZBerryESPOverlay {
	static const string FEATURE = "zomberry_esp";
	static const int LABEL_COUNT = 40;
	static const float SKELETON_DISTANCE = 150; //Metres
	static const float VEHICLE_DISTANCE = 1500;
	static const float AI_DISTANCE = 300;

	static const int COLOR_PLAYER = 0xFF55AAFF;
	static const int COLOR_ADMIN = 0xFF47EB00;
	static const int COLOR_FAR_PLAYER = 0xFFB0B0FF;
	static const int COLOR_VEHICLE = 0xFFFFC34D;
	static const int COLOR_INFECTED = 0xFFFF5A5A;
	static const int COLOR_ANIMAL = 0xFF8FD18F;

	protected Widget m_Root;
	protected CanvasWidget m_Canvas;
	protected ref array<TextWidget> m_Labels = new array<TextWidget>;
	protected int m_LabelsUsed;

	protected bool m_Enabled;
	protected bool m_ShowPlayers, m_ShowVehicles, m_ShowAI;

	void ZBerryESPOverlay() {
		GetZomberryCmdAPI().RegisterClientFeature(FEATURE, this, "OnFeature");
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

	//args: players, vehicles, infected and animals (0/1)
	void OnFeature(bool enabled, TIntArray args) {
		m_Enabled = enabled;
		m_ShowPlayers = args.Count() > 0 && args[0] != 0;
		m_ShowVehicles = args.Count() > 1 && args[1] != 0;
		m_ShowAI = args.Count() > 2 && args[2] != 0;

		if (!m_Enabled) {
			if (m_Root) m_Root.Show(false);
			return;
		}
		if (CreateWidgets()) m_Root.Show(true);
	}

	//Every frame, from MissionGameplay
	void Update() {
		if (!m_Enabled || !m_Canvas) return;

		m_Canvas.Clear();
		m_LabelsUsed = 0;

		float scale = ZBerryUIUtils.GetUIScale();
		float lineWidth = Math.Max(1, 1.5 * scale);
		vector cameraPos = GetGame().GetCurrentCameraPosition();

		if (m_ShowPlayers) DrawPlayers(cameraPos, lineWidth);
		if (m_ShowVehicles) DrawEntities(ZBerryEntityCategory.VEHICLES, cameraPos, VEHICLE_DISTANCE, COLOR_VEHICLE, lineWidth, true);
		if (m_ShowAI) {
			DrawEntities(ZBerryEntityCategory.INFECTED, cameraPos, AI_DISTANCE, COLOR_INFECTED, lineWidth, false);
			DrawEntities(ZBerryEntityCategory.ANIMALS, cameraPos, AI_DISTANCE, COLOR_ANIMAL, lineWidth, false);
		}

		for (int i = m_LabelsUsed; i < m_Labels.Count(); ++i) m_Labels[i].Show(false);
	}

	protected void DrawPlayers(vector cameraPos, float lineWidth) {
		ZBerryStreamClient streams = GetZBerryStreams();
		Man localPlayer = GetGame().GetPlayer();
		map<int, bool> drawnIds = new map<int, bool>;

		array<EntityAI> players = ZBerryEntityRegistry.GetEntities(ZBerryEntityCategory.PLAYERS);
		foreach (EntityAI entity : players) {
			PlayerBase player = PlayerBase.Cast(entity);
			if (!player || player == localPlayer || !player.IsAlive()) continue;

			int id = -1;
			string name = "Player";
			if (player.GetIdentity()) {
				id = player.GetIdentity().GetPlayerId();
				name = player.GetIdentity().GetName();
			}
			drawnIds.Set(id, true);

			int color = COLOR_PLAYER;
			if (streams.IsAdmin(id)) color = COLOR_ADMIN;

			float distance = vector.Distance(cameraPos, player.GetPosition());
			float left, top, right, bottom;
			if (!ZBerryESPDraw.GetScreenBox(player, left, top, right, bottom)) continue;

			ZBerryESPDraw.DrawBox(m_Canvas, left, top, right, bottom, color, lineWidth);
			if (distance <= SKELETON_DISTANCE) ZBerryESPDraw.DrawSkeleton(m_Canvas, player, color, lineWidth);

			string text = name + " " + Math.Round(distance) + "m";
			int health, blood;
			if (streams.GetHealth(id, health, blood)) text += " | " + health + " HP, " + blood + " blood";
			AddLabel((left + right) / 2, bottom, text, color);
		}

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
