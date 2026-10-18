//One function parameter: label + slider + edit box, kept in sync and clamped to [min, max].
//With value names the box shows the name, typing either a name or a number works
class ZBerryUIFuncParamRow extends ZBerryUIComponent {
	protected TextWidget m_Name;
	protected SliderWidget m_Slider;
	protected EditBoxWidget m_ValueBox;
	protected ZBerryFuncParam m_Param;

	protected int m_Min = 0;
	protected int m_Max = 1000;
	protected int m_Value = 0;

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/FuncParamRow.layout", parent)) return;

		m_Name = TextWidget.Cast( m_Root.FindAnyWidget("ParamName") );
		m_Slider = SliderWidget.Cast( BindWidget("ParamSlider") );
		m_ValueBox = EditBoxWidget.Cast( BindWidget("ParamValue") );
	}

	void Setup(ZBerryFuncParam param) {
		m_Param = param;
		m_Name.SetText(param.name);
		m_Min = param.values.Get(0);
		m_Max = param.values.Get(1);
		SetValue(param.values.Get(2));
	}

	int GetValue() {
		return m_Value;
	}

	void SetValue(int value) {
		m_Value = Clamp(value);
		UpdateSlider();
		UpdateBox();
	}

	override bool OnChange(Widget w, int x, int y, bool finished) {
		if (w == m_Slider) {
			m_Value = Clamp((m_Slider.GetCurrent() / 100 * (m_Max - m_Min)) + m_Min);
			UpdateBox();
			return true;
		}
		if (w == m_ValueBox) {
			int namedValue;
			if (m_Param && m_Param.FindNameValue(m_ValueBox.GetText(), namedValue)) {
				m_Value = Clamp(namedValue);
			} else {
				m_Value = Clamp(m_ValueBox.GetText().ToInt());
			}
			UpdateSlider(); //Box itself is corrected on mouse leave, to not fight with typing
			return true;
		}
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y) {
		if (w == m_ValueBox) {
			UpdateBox();
			return true;
		}
		return false;
	}

	protected int Clamp(float value) {
		return Math.Clamp(Math.Round(value), m_Min, m_Max);
	}

	protected void UpdateSlider() {
		float range = m_Max - m_Min;
		if (range <= 0) {
			m_Slider.SetCurrent(0);
			return;
		}

		m_Slider.SetCurrent((m_Value - m_Min) / range * 100);
	}

	protected void UpdateBox() {
		string valueName;
		if (m_Param) valueName = m_Param.GetValueName(m_Value);

		if (valueName != "") {
			m_ValueBox.SetText(valueName);
		} else {
			m_ValueBox.SetText(m_Value.ToString());
		}
	}
};
