//What one admin's client has subscribed to, and what was last sent to it (for deltas)
class ZBerrySubscriber {
	static const int NOT_SENT = -1;

	PlayerIdentity Identity; //Weak: gone when they disconnect. NULL in singleplayer
	int Streams; //ZBerryStreamFlags
	int MarkerMask; //Bit per ZBerryEntityCategory below MARKER_COUNT
	int SelectedId = -1; //Gets health and blood
	int ViewFlags; //ZBerryStreamFlags.VIEW_*
	bool EspActive; //Set by the ESP function, not by the client. Its data also needs the client's ESP_PLAYERS (WantsEsp)

	int RosterVersionSent = -1;
	float StateTimer = 1000; //Due right away
	float FullTimer = 1000;
	float MarkersTimer = 1000;
	ref map<int, int> SentPositions = new map<int, int>;
	ref map<int, int> SentMisc = new map<int, int>;
	ref TIntArray MarkerSignatures = new TIntArray; //Per marker category, NOT_SENT until sent

	void ZBerrySubscriber() {
		for (int i = 0; i < ZBerryEntityCategory.MARKER_COUNT; ++i) MarkerSignatures.Insert(NOT_SENT);
	}

	bool IsEmpty() {
		return Streams == 0 && MarkerMask == 0 && !EspActive;
	}

	//ESP data needs both: the server switched ESP on, and the client's ESP settings use far players or health
	bool WantsEsp() {
		return EspActive && (Streams & ZBerryStreamFlags.ESP_PLAYERS) != 0;
	}
};

//Server side of the streams: one timer serves every subscriber, data shared between them is built once per tick
class ZBerryStreamServer {
	static const int TICK_MS = 500;
	static const float LIST_INTERVAL = 5; //Seconds between player states for the players panel
	static const float MAP_INTERVAL = 2; //... while the map is shown
	static const float ESP_INTERVAL = 1; //... while ESP is on
	static const float FULL_INTERVAL = 10; //A full state now and then, deltas in between
	static const float MARKERS_INTERVAL = 10;

	protected static ZBerryStreamServer s_Instance; //Weak, owned by ZomberryBase

	protected ZBerryAuth m_Auth;
	protected ref map<int, ref ZBerrySubscriber> m_Subscribers = new map<int, ref ZBerrySubscriber>; //By session id, 0 in singleplayer
	protected ref ZBerryRosterArray m_Roster = new ZBerryRosterArray;
	protected int m_RosterVersion = 0;

	void ZBerryStreamServer(ZBerryAuth auth) {
		m_Auth = auth;
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Subscribe, "Subscribe", this, ZBerrySPExec.Client );
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Tick, TICK_MS, true);
		GetZBerryClientFeatures().OnServerFeatureChanged.Insert(OnFeatureChanged);
		s_Instance = this;
	}

	//Server and singleplayer, NULL on clients
	static ZBerryStreamServer Get() {
		return s_Instance;
	}

	//ESP switched off on the server side (permission removed): stop its data too
	protected void OnFeatureChanged(PlayerIdentity identity, string name, bool enabled) {
		if (name == ZBerryESPOverlay.FEATURE && !enabled) SetEspActive(identity, false);
	}

	void ~ZBerryStreamServer() {
		if (GetGame()) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Tick);
		if (g_ZBerryClientFeatures) g_ZBerryClientFeatures.OnServerFeatureChanged.Remove(OnFeatureChanged);
	}

	protected int GetSessionId(PlayerIdentity identity) {
		if (!GetGame().IsMultiplayer() || !identity) return 0;
		return identity.GetPlayerId();
	}

	protected ZBerrySubscriber GetSubscriber(PlayerIdentity identity, bool create) {
		int id = GetSessionId(identity);
		ZBerrySubscriber subscriber = m_Subscribers.Get(id);
		if (!subscriber && create) {
			subscriber = new ZBerrySubscriber;
			subscriber.Identity = identity;
			m_Subscribers.Set(id, subscriber);
		}
		return subscriber;
	}

	protected void DropIfEmpty(PlayerIdentity identity) {
		ZBerrySubscriber subscriber = GetSubscriber(identity, false);
		if (subscriber && subscriber.IsEmpty()) m_Subscribers.Remove(GetSessionId(identity));
	}

	void Subscribe( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4< int, int, int, int > request; //Streams, marker categories, selected player, view flags
		if ( !ctx.Read( request ) ) return;
		if (GetGame().IsMultiplayer() && (type != ZBerryCallType.Server || !GetGame().IsServer() || !sender)) return;
		if (!m_Auth.IsAdmin(sender)) return;

		ZBerrySubscriber subscriber = GetSubscriber(sender, true);
		int addedMarkers = request.param2 & ~subscriber.MarkerMask;
		bool selectionChanged = subscriber.SelectedId != request.param3;
		bool espAdded = (request.param1 & ZBerryStreamFlags.ESP_PLAYERS) != 0 && (subscriber.Streams & ZBerryStreamFlags.ESP_PLAYERS) == 0;
		bool viewChanged = subscriber.ViewFlags != request.param4;

		subscriber.Streams = request.param1;
		subscriber.MarkerMask = request.param2;
		subscriber.SelectedId = request.param3;
		subscriber.ViewFlags = request.param4;

		if (addedMarkers != 0) subscriber.MarkersTimer = MARKERS_INTERVAL; //Newly shown categories right away
		if (selectionChanged || viewChanged || espAdded) subscriber.StateTimer = MAP_INTERVAL; //Sooner, for the new selection or view
		DropIfEmpty(sender);
	}

	//ESP outlives the menu, so it's switched by the server side of the ESP function rather than by Subscribe
	void SetEspActive(PlayerIdentity identity, bool active) {
		ZBerrySubscriber subscriber = GetSubscriber(identity, active);
		if (!subscriber) return;

		subscriber.EspActive = active;
		subscriber.StateTimer = ESP_INTERVAL;
		DropIfEmpty(identity);
	}

	bool IsEspActive(PlayerIdentity identity) {
		ZBerrySubscriber subscriber = GetSubscriber(identity, false);
		return subscriber && subscriber.EspActive;
	}

	protected void Tick() {
		if (m_Subscribers.Count() == 0) return;

		float tickSeconds = TICK_MS / 1000.0;
		map<int, PlayerBase> players = GetPlayers();
		UpdateRoster(players);
		map<int, ref ZBerryMarkerBatch> batches = new map<int, ref ZBerryMarkerBatch>; //Built once per tick, shared

		TIntArray gone = new TIntArray;
		foreach (int id, ZBerrySubscriber subscriber : m_Subscribers) {
			if (GetGame().IsMultiplayer() && !subscriber.Identity) {
				gone.Insert(id);
				continue;
			}

			SendRoster(subscriber);

			subscriber.StateTimer += tickSeconds;
			subscriber.FullTimer += tickSeconds;
			if (subscriber.StateTimer >= GetStateInterval(subscriber)) {
				subscriber.StateTimer = 0;
				SendState(subscriber, players);
			}

			subscriber.MarkersTimer += tickSeconds;
			if (subscriber.MarkersTimer >= MARKERS_INTERVAL) {
				subscriber.MarkersTimer = 0;
				SendMarkers(subscriber, batches);
			}
		}

		foreach (int goneId : gone) m_Subscribers.Remove(goneId);
	}

	protected float GetStateInterval(ZBerrySubscriber subscriber) {
		if (subscriber.WantsEsp()) return ESP_INTERVAL;
		if ((subscriber.ViewFlags & ZBerryStreamFlags.VIEW_MAP) != 0) return MAP_INTERVAL;
		return LIST_INTERVAL;
	}

	//Session id -> player, players with an identity only (connecting and disconnecting ones have none)
	protected map<int, PlayerBase> GetPlayers() {
		map<int, PlayerBase> result = new map<int, PlayerBase>;

		if (!GetGame().IsMultiplayer()) {
			PlayerBase localPlayer = PlayerBase.Cast(GetGame().GetPlayer());
			if (localPlayer) result.Set(0, localPlayer);
			return result;
		}

		array<Man> men = new array<Man>;
		GetGame().GetPlayers(men);
		foreach (Man man : men) {
			PlayerBase player = PlayerBase.Cast(man);
			if (player && player.GetIdentity()) result.Set(player.GetIdentity().GetPlayerId(), player);
		}
		return result;
	}

	//Rebuilt every tick, but only sent when something in it changed
	protected void UpdateRoster(map<int, PlayerBase> players) {
		ZBerryRosterArray roster = new ZBerryRosterArray;
		foreach (int id, PlayerBase player : players) {
			string name = "Player";
			if (player.GetIdentity()) name = player.GetIdentity().GetName();
			roster.Insert(new ZBerryRosterEntry(id, name, m_Auth.IsAdmin(player.GetIdentity()), GetExtra(player)));
		}

		bool changed = roster.Count() != m_Roster.Count();
		for (int i = 0; !changed && i < roster.Count(); ++i) {
			if (!roster[i].IsSameAs(m_Roster[i])) changed = true;
		}
		if (!changed) return;

		m_Roster = roster;
		++m_RosterVersion;
	}

	//What a player holds or drives, for the list
	protected string GetExtra(PlayerBase player) {
		if (player.GetCommand_Vehicle()) return player.GetCommand_Vehicle().GetTransport().GetDisplayName();
		if (player.GetItemInHands()) return player.GetItemInHands().GetInventoryItemType().GetName();
		return "";
	}

	protected bool CanSeePlayers(ZBerrySubscriber subscriber) {
		return m_Auth.HasPermission(subscriber.Identity, ZBerryPermissions.PLAYERS_LIST) || m_Auth.HasPermission(subscriber.Identity, ZBerryPermissions.MAP_PLAYER_MARKERS) || subscriber.WantsEsp();
	}

	protected void SendRoster(ZBerrySubscriber subscriber) {
		if ((subscriber.Streams & ZBerryStreamFlags.ROSTER) == 0 && !subscriber.WantsEsp()) return;
		if (subscriber.RosterVersionSent == m_RosterVersion || !CanSeePlayers(subscriber)) return;

		subscriber.RosterVersionSent = m_RosterVersion;
		ZBerryRosterArray roster = m_Roster; //Locals keep data alive while it's sent
		GetZBerryRPC().SendRPC( ZBerryRPCOp.Roster, new Param3<int, int, ref ZBerryRosterArray>(GetGame().GetTime(), m_RosterVersion, roster), true, subscriber.Identity );
	}

	//Positions only for admins who see map markers (or use ESP), health and blood only for the selected player
	//(with the player list right) and for everyone while ESP is on. Deltas, plus a full state every FULL_INTERVAL
	protected void SendState(ZBerrySubscriber subscriber, map<int, PlayerBase> players) {
		if ((subscriber.Streams & ZBerryStreamFlags.STATE) == 0 && !subscriber.WantsEsp()) return;
		if (!CanSeePlayers(subscriber)) return;

		bool showPositions = subscriber.WantsEsp() || m_Auth.HasPermission(subscriber.Identity, ZBerryPermissions.MAP_PLAYER_MARKERS);
		bool showSelectedHealth = m_Auth.HasPermission(subscriber.Identity, ZBerryPermissions.PLAYERS_LIST);
		bool full = subscriber.FullTimer >= FULL_INTERVAL;
		if (full) {
			subscriber.FullTimer = 0;
			subscriber.SentPositions.Clear();
			subscriber.SentMisc.Clear();
		}

		TIntArray packed = new TIntArray;
		foreach (int id, PlayerBase player : players) {
			int position = 0;
			bool hasHeading = false;
			float heading = 0;
			if (showPositions) {
				position = ZBerryStatePacker.PackPosition(player.GetPosition());
				vector yawPitchRoll = player.GetYawPitchRoll();
				heading = yawPitchRoll[0];
				hasHeading = true;
			}

			bool hasHealth = subscriber.WantsEsp() || (showSelectedHealth && id == subscriber.SelectedId);
			int health = 0;
			int blood = 0;
			if (hasHealth) {
				health = player.GetHealth();
				blood = player.GetHealth("", "Blood");
			}
			int misc = ZBerryStatePacker.PackMisc(heading, hasHeading, health, blood, hasHealth);

			int sentPosition, sentMisc;
			bool known = subscriber.SentPositions.Find(id, sentPosition) && subscriber.SentMisc.Find(id, sentMisc);
			if (!full && known && sentPosition == position && sentMisc == misc) continue;

			subscriber.SentPositions.Set(id, position);
			subscriber.SentMisc.Set(id, misc);
			packed.Insert(id);
			packed.Insert(position);
			packed.Insert(misc);
		}

		if (!full && packed.Count() == 0) return; //Nothing moved
		GetZBerryRPC().SendRPC( ZBerryRPCOp.PlayerState, new Param3<int, bool, ref TIntArray>(GetGame().GetTime(), full, packed), true, subscriber.Identity );
	}

	//Each subscribed category the admin may see, when it changed since the last send
	protected void SendMarkers(ZBerrySubscriber subscriber, map<int, ref ZBerryMarkerBatch> batches) {
		for (int category = 0; category < ZBerryEntityCategory.MARKER_COUNT; ++category) {
			bool wanted = (subscriber.MarkerMask & (1 << category)) != 0;
			if (!wanted || !m_Auth.HasPermission(subscriber.Identity, ZBerryPermissions.MapMarkerNode(category))) {
				subscriber.MarkerSignatures[category] = ZBerrySubscriber.NOT_SENT; //Sent again once shown again
				continue;
			}

			ZBerryMarkerBatch batch = batches.Get(category);
			if (!batch) {
				batch = ZBerryMapMarkers.Build(category);
				batches.Set(category, batch);
			}
			if (subscriber.MarkerSignatures[category] == batch.Signature) continue;

			subscriber.MarkerSignatures[category] = batch.Signature;
			ZBerryMapMarkerArray markers = batch.Markers;
			GetZBerryRPC().SendRPC( ZBerryRPCOp.Markers, new Param3<int, int, ref ZBerryMapMarkerArray>(category, batch.Dropped, markers), true, subscriber.Identity );
		}
	}

	//For tests
	int GetRosterVersion() {
		return m_RosterVersion;
	}
};
