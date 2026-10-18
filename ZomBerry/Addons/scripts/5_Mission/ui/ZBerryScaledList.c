//Listbox text keeps its pixel size on every resolution, and scripts can't change a listbox's font. So each layout
//has the same listbox in several fonts (Name, Name_F12, Name_F16...) and this shows the one closest to the menu's scale.
//Owners keep using a plain TextListboxWidget: the one Pick() returns, refilled whenever Pick() switched
class ZBerryScaledList {
	static const int BASE_FONT_SIZE = 14; //The base listbox (no suffix), at 1080p

	static TIntArray GetVariantSizes() {
		return {12, 16, 22, 28};
	}

	protected ref array<TextListboxWidget> m_Lists = new array<TextListboxWidget>;
	protected ref TIntArray m_Sizes = new TIntArray;
	protected TextListboxWidget m_Active;

	void ZBerryScaledList(Widget root, string name, ScriptedWidgetEventHandler handler) {
		AddVariant(root.FindAnyWidget(name), BASE_FONT_SIZE, handler);
		TIntArray sizes = GetVariantSizes();
		foreach (int size : sizes) {
			AddVariant(root.FindAnyWidget(name + "_F" + size), size, handler);
		}

		if (m_Lists.Count() == 0) ZomberryBase.Log( "ZomBerryUI", "ERROR: Listbox " + name + " not found" );
	}

	protected void AddVariant(Widget w, int size, ScriptedWidgetEventHandler handler) {
		TextListboxWidget list = TextListboxWidget.Cast(w);
		if (!list) return;

		list.SetHandler(handler);
		m_Lists.Insert(list);
		m_Sizes.Insert(size);
	}

	TextListboxWidget GetActive() {
		return m_Active;
	}

	//Shows the variant for the current resolution. Returns true when it changed (the owner must refill it)
	bool Pick() {
		float wanted = BASE_FONT_SIZE * ZBerryUIUtils.GetUIScale();

		TextListboxWidget best;
		float bestDistance = 1000;
		for (int i = 0; i < m_Lists.Count(); ++i) {
			float distance = Math.AbsFloat(m_Sizes[i] - wanted);
			if (distance < bestDistance) {
				bestDistance = distance;
				best = m_Lists[i];
			}
		}

		foreach (TextListboxWidget list : m_Lists) list.Show(list == best);

		bool changed = best != m_Active;
		m_Active = best;
		return changed;
	}

	//Same position or size change for every variant (used by scripted layouts)
	array<TextListboxWidget> GetAll() {
		return m_Lists;
	}
};
