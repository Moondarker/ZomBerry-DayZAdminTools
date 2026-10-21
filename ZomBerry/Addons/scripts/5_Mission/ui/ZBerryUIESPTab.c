//ESP settings (#68), under the header's gear: what the overlay draws on this client (ZBerryESPSettings). Changes apply
//at once and are saved when the tab or the menu closes. The header action button runs the ESP function, which is what
//switches ESP on and off: the server decides, as for any function
class ZBerryUIESPTab extends ZBerryUITab {
	static const string FUNCTION = "ESPToggle";

	protected ref array<ButtonWidget> m_Toggles = new array<ButtonWidget>;
	protected ref TStringArray m_ToggleNames = new TStringArray; //Setting of each toggle, see Get/SetToggle
	protected ref ZBerryUIFuncParamRow m_RangeRow, m_AIRangeRow;
	protected ref ZBerryFuncParam m_RangeParam, m_AIRangeParam; //Kept here: the rows only hold weak references
	protected bool m_ShownEnabled;

	protected override void OnInit() {
		AddToggle("ESPBox", "PlayerBox");
		AddToggle("ESPSkeleton", "PlayerSkeleton");
		AddToggle("ESPName", "PlayerName");
		AddToggle("ESPDistance", "PlayerDistance");
		AddToggle("ESPHealth", "PlayerHealth");
		AddToggle("ESPFarPlayers", "FarPlayers");
		AddToggle("ESPVehicles", "Vehicles");
		AddToggle("ESPInfected", "Infected");
		AddToggle("ESPAnimals", "Animals");

		ZBerryESPSettingsData settings = GetZBerryESPSettings().Get();
		TIntArray range = {ZBerryESPSettings.MIN_DISTANCE, ZBerryESPSettings.MAX_DISTANCE}; //min, max, then the current value
		range.Insert(settings.MaxDistance);
		TIntArray aiRange = {ZBerryESPSettings.MIN_DISTANCE, ZBerryESPSettings.MAX_DISTANCE};
		aiRange.Insert(settings.AIDistance);
		m_RangeParam = new ZBerryFuncParam("Players, vehicles (m)", range);
		m_AIRangeParam = new ZBerryFuncParam("Infected, animals (m)", aiRange);
		m_RangeRow = AddRangeRow("ESPRangeSlot", m_RangeParam);
		m_AIRangeRow = AddRangeRow("ESPAIRangeSlot", m_AIRangeParam);

		UpdateToggles();
	}

	protected void AddToggle(string widgetName, string setting) {
		ButtonWidget button = ButtonWidget.Cast( BindWidget(widgetName) );
		if (!button) return;

		m_Toggles.Insert(button);
		m_ToggleNames.Insert(setting);
	}

	protected ZBerryUIFuncParamRow AddRangeRow(string slotName, ZBerryFuncParam param) {
		Widget slot = m_Root.FindAnyWidget(slotName);
		if (!slot) return NULL;

		ZBerryUIFuncParamRow row = new ZBerryUIFuncParamRow();
		row.Init(slot);
		row.Setup(param);
		return row;
	}

	bool GetToggle(string setting) {
		ZBerryESPSettingsData settings = GetZBerryESPSettings().Get();
		switch (setting) {
			case "PlayerBox": return settings.PlayerBox;
			case "PlayerSkeleton": return settings.PlayerSkeleton;
			case "PlayerName": return settings.PlayerName;
			case "PlayerDistance": return settings.PlayerDistance;
			case "PlayerHealth": return settings.PlayerHealth;
			case "FarPlayers": return settings.FarPlayers;
			case "Vehicles": return settings.Vehicles;
			case "Infected": return settings.Infected;
			case "Animals": return settings.Animals;
		}
		return false;
	}

	protected void SetToggle(string setting, bool value) {
		ZBerryESPSettingsData settings = GetZBerryESPSettings().Get();
		switch (setting) {
			case "PlayerBox": settings.PlayerBox = value; break;
			case "PlayerSkeleton": settings.PlayerSkeleton = value; break;
			case "PlayerName": settings.PlayerName = value; break;
			case "PlayerDistance": settings.PlayerDistance = value; break;
			case "PlayerHealth": settings.PlayerHealth = value; break;
			case "FarPlayers": settings.FarPlayers = value; break;
			case "Vehicles": settings.Vehicles = value; break;
			case "Infected": settings.Infected = value; break;
			case "Animals": settings.Animals = value; break;
		}
		GetZBerryESPSettings().Changed();
	}

	//Active ones are green, like the map's filters
	protected void UpdateToggles() {
		for (int i = 0; i < m_Toggles.Count(); ++i) {
			ZBerryUIUtils.SetButtonActive(m_Toggles[i], GetToggle(m_ToggleNames[i]));
		}
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		ButtonWidget clicked = ButtonWidget.Cast(w);
		int index = -1;
		if (clicked) index = m_Toggles.Find(clicked);
		if (index == -1) return false;

		string setting = m_ToggleNames[index];
		SetToggle(setting, !GetToggle(setting));
		UpdateToggles();
		return true;
	}

	//The rows don't report changes: their values are picked up here
	override void Update(float tDelta) {
		ZBerryESPSettingsData settings = GetZBerryESPSettings().Get();
		bool changed = false;
		if (m_RangeRow && m_RangeRow.GetValue() != settings.MaxDistance) {
			settings.MaxDistance = m_RangeRow.GetValue();
			changed = true;
		}
		if (m_AIRangeRow && m_AIRangeRow.GetValue() != settings.AIDistance) {
			settings.AIDistance = m_AIRangeRow.GetValue();
			changed = true;
		}
		if (changed) GetZBerryESPSettings().Changed();

		if (IsEspEnabled() != m_ShownEnabled) UpdateActionButton();
	}

	protected bool IsEspEnabled() {
		return GetZomberryCmdAPI().IsClientFeatureEnabled(ZBerryESPOverlay.FEATURE);
	}

	protected void UpdateActionButton() {
		m_ShownEnabled = IsEspEnabled();
		if (m_ShownEnabled) {
			SetActionButton("ESP: On", COLOR_GREEN);
		} else {
			SetActionButton("ESP: Off", ZBerryUIUtils.TEXT_DEFAULT);
		}
	}

	protected override void OnActivate() {
		UpdateToggles();
		UpdateActionButton();
	}

	protected override void OnDeactivate() {
		GetZBerryESPSettings().Save();
	}

	override void OnMenuHide() {
		GetZBerryESPSettings().Save();
	}

	//Runs the ESP function like the Functions tab would: the server switches the overlay
	override void OnActionButtonClick() {
		int funcId = ZomberryBase.GetKeyBindsMgr().FindFunc(FUNCTION);
		if (funcId == -1) {
			ZBerryUIUtils.Message("The ESP function isn't available (yet): the function list hasn't arrived, or you may not use it");
			return;
		}

		int selfId = ZBerryUIUtils.GetLocalPlayerId();
		TIntArray values = {0, 0, 0}; //Local: a temporary inside the Param would be freed before it's sent
		GetZBerryRPC().SendRPC( ZBerryRPCOp.ExecuteCommand, new Param4< int, int, vector, autoptr TIntArray >( funcId, selfId, ZBerryUIUtils.GetCursorPos(), values ), true );
	}
};
