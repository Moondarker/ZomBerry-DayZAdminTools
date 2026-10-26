//Loadout overview of a death, from the Logs tab: what the victim wore and held when they died (kept by the server,
//ZBerryDeathLoadouts). Try it on yourself, give it back to the victim, or keep a copy among your own loadouts.
//Part of the menu's own layout like the other dialogs; Esc closes it
class ZBerryUIDeathLoadoutDialog extends ZBerryUIComponent {
	static const int COLOR_UNAVAILABLE = 0xFF808080;

	protected TextWidget m_Status;
	protected MultilineTextWidget m_Info;
	protected TextListboxWidget m_List; //The visible variant of m_Lists
	protected ref ZBerryScaledList m_Lists;
	protected ButtonWidget m_TryButton, m_RestoreButton, m_SaveButton, m_CloseButton;

	protected int m_EventId = -1;
	protected string m_VictimName;
	protected string m_DeathTime; //Local clock, for the name of a saved copy
	protected bool m_Answered;
	protected ref ZBerryLoadout m_Loadout; //NULL until the server answered, or when it isn't kept
	protected int m_VictimState; //ZBerryVictimState

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/DeathLoadoutDialog.layout", parent)) return;

		m_Info = MultilineTextWidget.Cast( m_Root.FindAnyWidget("DeathInfo") );
		m_Status = TextWidget.Cast( m_Root.FindAnyWidget("DeathStatus") );
		m_Lists = new ZBerryScaledList(m_Root, "DeathItems", this);
		m_Lists.Pick();
		m_List = m_Lists.GetActive();
		m_TryButton = ButtonWidget.Cast( BindWidget("DeathTry") );
		m_RestoreButton = ButtonWidget.Cast( BindWidget("DeathRestore") );
		m_SaveButton = ButtonWidget.Cast( BindWidget("DeathSave") );
		m_CloseButton = ButtonWidget.Cast( BindWidget("DeathClose") );
		Show(false);
	}

	void ~ZBerryUIDeathLoadoutDialog() {
		if (g_ZBerryClientLoadouts) g_ZBerryClientLoadouts.OnDeathLoadout.Remove(OnDeathLoadout);
	}

	//The death of a kill event: shown at once, the gear as soon as the server sends it
	void Open(ZBerryLogEvent killEvent, string description, string deathTime) {
		if (!m_Root || !killEvent) return;

		m_EventId = killEvent.Id;
		m_VictimName = killEvent.TargetName;
		m_DeathTime = deathTime;
		m_Answered = false;
		m_Loadout = NULL;
		m_Info.SetText(description);
		if (m_Lists.Pick()) m_List = m_Lists.GetActive();
		Refresh();
		Show(true);

		//Again on every open: the client side is built anew for each server, the menu isn't
		ZBerryClientLoadouts loadouts = GetZBerryClientLoadouts();
		loadouts.OnDeathLoadout.Remove(OnDeathLoadout);
		loadouts.OnDeathLoadout.Insert(OnDeathLoadout);
		loadouts.RequestDeath(m_EventId);
	}

	void Close() {
		Show(false);
	}

	protected void OnDeathLoadout(int eventId) {
		if (eventId != m_EventId) return;

		string reason;
		ZBerryLoadout loadout;
		int state;
		if (!GetZBerryClientLoadouts().GetDeath(eventId, reason, loadout, state)) return;

		m_Answered = true;
		m_Loadout = NULL;
		if (reason == "") m_Loadout = loadout;
		m_VictimState = state;
		Refresh(reason);
	}

	protected void Refresh(string reason = "") {
		m_List.ClearItems();

		if (!m_Answered) {
			SetStatus("Loading...", COLOR_UNAVAILABLE);
		} else if (!m_Loadout) {
			SetStatus(reason, ZBerryUILogsTab.COLOR_REFUSED);
		} else {
			string status = m_Loadout.CountAll().ToString() + " items";
			int restricted = m_Loadout.CountRestricted();
			int missing = m_Loadout.CountMissing();
			if (restricted > 0) status += ", " + restricted + " your spawn rules don't allow";
			if (missing > 0) status += ", " + missing + " not on this server";
			status += ". " + DescribeVictim();
			SetStatus(status, ZBerryUIUtils.TEXT_DEFAULT);

			if (m_Loadout.Items.Count() == 0) {
				int row = m_List.AddItem("Nothing: they had nothing on them", NULL, 0);
				m_List.SetItemColor(row, 0, COLOR_UNAVAILABLE);
			}
			ZBerryLoadoutRows.Add(m_List, m_Loadout.Items, 0);
		}
		UpdateButtons();
	}

	protected void SetStatus(string text, int color) {
		m_Status.SetText(text);
		m_Status.SetColor(color);
	}

	protected string DescribeVictim() {
		if (m_VictimState == ZBerryVictimState.ALIVE) return m_VictimName + " is online";
		if (m_VictimState == ZBerryVictimState.DEAD) return m_VictimName + " is online, but dead";
		return m_VictimName + " is offline";
	}

	//Why a button can't be used, "" if it can
	protected string WhyNotTry() {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		if (!auth.Has(ZBerryPermissions.LOADOUTS_USE) || !auth.Has(ZBerryPermissions.SPAWN_INVENTORY)) return "Trying on needs " + ZBerryPermissions.LOADOUTS_USE + " and " + ZBerryPermissions.SPAWN_INVENTORY;
		return WhyNoGear();
	}

	protected string WhyNotRestore() {
		if (!GetZBerryClientAuth().Has(ZBerryPermissions.LOADOUTS_RESTORE)) return "Restoring needs " + ZBerryPermissions.LOADOUTS_RESTORE;
		string noGear = WhyNoGear();
		if (noGear != "") return noGear;
		if (m_VictimState == ZBerryVictimState.OFFLINE) return m_VictimName + " isn't online";
		if (m_VictimState == ZBerryVictimState.DEAD) return m_VictimName + " is dead, try again once they've respawned";
		return "";
	}

	protected string WhyNoGear() {
		if (!m_Answered) return "Wait until the gear is loaded";
		if (!m_Loadout) return "This death's gear isn't kept";
		if (m_Loadout.Items.Count() == 0) return "They had nothing on them";
		return "";
	}

	protected void UpdateButtons() {
		SetAvailable(m_TryButton, WhyNotTry() == "");
		SetAvailable(m_RestoreButton, WhyNotRestore() == "");
		SetAvailable(m_SaveButton, WhyNoGear() == "");
	}

	protected static void SetAvailable(ButtonWidget btn, bool available) {
		if (available) {
			ZBerryUIUtils.SetButtonTextColor(btn, ZBerryUIUtils.TEXT_DEFAULT);
		} else {
			ZBerryUIUtils.SetButtonTextColor(btn, COLOR_UNAVAILABLE);
		}
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_CloseButton) {
			Close();
			return true;
		}

		string why;
		if (w == m_TryButton) {
			why = WhyNotTry();
			if (why == "") {
				GetZBerryClientLoadouts().TryDeath(m_EventId);
			} else {
				ZBerryUIUtils.Message(why);
			}
			return true;
		}
		if (w == m_RestoreButton) {
			why = WhyNotRestore();
			if (why == "") {
				GetZBerryClientLoadouts().RestoreDeath(m_EventId);
			} else {
				ZBerryUIUtils.Message(why);
			}
			return true;
		}
		if (w == m_SaveButton) {
			why = WhyNoGear();
			if (why == "") {
				GetZBerryClientLoadouts().SaveCopy(m_Loadout, m_VictimName + " died " + m_DeathTime);
			} else {
				ZBerryUIUtils.Message(why);
			}
			return true;
		}
		return false;
	}
};
