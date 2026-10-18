//Base for every self-contained piece of the admin menu: owns a sub-layout and handles events of its own widgets only
class ZBerryUIComponent extends ScriptedWidgetEventHandler {
	protected Widget m_Root;
	protected string m_LayoutPath; //As requested, kept for error messages

	Widget GetRoot() {
		return m_Root;
	}

	void Show(bool show) {
		if (m_Root) m_Root.Show(show);
	}

	bool IsVisible() {
		return m_Root && m_Root.IsVisible();
	}

	//Full literal paths on purpose: with the path built at runtime (dir constant + file name) in this call, the engine
	//consistently returned a FunctionsTab.layout tree for SpawnTab.layout in-game, although the right name was requested
	protected Widget LoadLayout(string layoutPath, Widget parent) {
		m_LayoutPath = layoutPath;
		m_Root = GetGame().GetWorkspace().CreateWidgets(layoutPath, parent);
		if (!m_Root) ZomberryBase.Log( "ZomBerryUI", "ERROR: Can't load layout " + layoutPath );

		return m_Root;
	}

	//Finds a widget inside this component and routes its events to this component
	protected Widget BindWidget(string widgetName) {
		if (!m_Root) return NULL;

		Widget w = m_Root.FindAnyWidget(widgetName);
		if (!w) {
			ZomberryBase.Log( "ZomBerryUI", "ERROR: Widget " + widgetName + " not found in " + m_Root.GetName() + " (" + ClassName() + " requested " + m_LayoutPath + ")" );
			return NULL;
		}

		w.SetHandler(this);
		return w;
	}
};

class ZBerryUIUtils {
	static const int TEXT_DEFAULT = 0xFFFFFFFF;

	static void SetButtonActive(ButtonWidget btn, bool active) {
		if (!btn) return;

		if (active) {
			SetButtonTextColor(btn, COLOR_GREEN);
		} else {
			SetButtonTextColor(btn, TEXT_DEFAULT);
		}
	}

	//Button captions are child TextWidgets: a button's own text keeps its pixel size on every resolution,
	//a TextWidget with an exact text size scales with the menu
	static TextWidget GetButtonLabel(ButtonWidget btn) {
		Widget child = btn.GetChildren();
		while (child) {
			TextWidget label = TextWidget.Cast(child);
			if (label) return label;
			child = child.GetSibling();
		}
		return NULL;
	}

	static void SetButtonText(ButtonWidget btn, string text) {
		TextWidget label = GetButtonLabel(btn);
		if (label) {
			label.SetText(text);
		} else {
			btn.SetText(text);
		}
	}

	static void SetButtonTextColor(ButtonWidget btn, int color) {
		TextWidget label = GetButtonLabel(btn);
		if (label) {
			label.SetColor(color);
		} else {
			btn.SetTextColor(color);
		}
	}

	static void Message( string txt ) {
		GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "[ZomBerry]", txt, ""));
	}

	static int GetLocalPlayerId() {
		if (!GetGame().IsMultiplayer()) return 0;

		Man ply = GetGame().GetPlayer();
		if (!ply || !ply.GetIdentity()) return 0;

		return ply.GetIdentity().GetPlayerId();
	}

	static string FormatKeyName(int key) {
		string keyName = typename.EnumToString(KeyCode, key);
		return "[" + keyName.Substring(3, keyName.Length() - 3) + "]"; //Strip "KC_"
	}

	static vector GetCursorPos() { //<3 Arkensor and CO
		if ( !GetGame().GetPlayer() ) { return "0 0 0"; }

		vector rayStart = GetGame().GetCurrentCameraPosition();
		vector rayEnd = rayStart + GetGame().GetCurrentCameraDirection() * 10000;
		vector hitPos; vector hitNormal; int hitComponentIndex;
		DayZPhysics.RaycastRV(rayStart, rayEnd, hitPos, hitNormal, hitComponentIndex, NULL, NULL, GetGame().GetPlayer());

		return hitPos;
	}
};
