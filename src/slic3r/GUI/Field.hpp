#ifndef SLIC3R_GUI_FIELD_HPP
#define SLIC3R_GUI_FIELD_HPP

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <memory>
#include <cstdint>
#include <functional>
#include <boost/any.hpp>

#include <wx/spinctrl.h>
#include <wx/bmpcbox.h>
#include <wx/clrpicker.h>

#include "libslic3r/libslic3r.h"
#include "libslic3r/Config.hpp"
#include "libslic3r/Utils.hpp"

#include "GUI.hpp"
#include "wxExtensions.hpp"

#ifdef __WXMSW__
#define wxMSW true
#else
#define wxMSW false
#endif

namespace Slic3r { namespace GUI {

class Field;
using t_field = std::unique_ptr<Field>;
using t_kill_focus = std::function<void(const std::string&)>;
using t_change = std::function<void(const t_config_option_key&, const boost::any&)>;
using t_back_to_init = std::function<void(const std::string&)>;

wxString double_to_string(double const value, const int max_precision = 4);

class RevertButton : public ScalableButton
{
    bool hidden = false; // never show button if it's hidden ones
public:
// 	RevertButton() {} 
// 	RevertButton(wxWindow* parent, wxWindowID id, const wxString& label = wxEmptyString,
// 		const wxPoint& pos = wxDefaultPosition,
// 		const wxSize& size = wxDefaultSize, long style = 0,
// 		const wxValidator& validator = wxDefaultValidator,
// 		const wxString& name = wxTextCtrlNameStr)
// 	{
// 		this->Create(parent, id, label, pos, size, style, validator, name);
// 	}
    RevertButton(
        wxWindow *parent,
        const std::string& icon_name = ""
        ) :
        ScalableButton(parent, wxID_ANY, icon_name) {}

	// overridden from wxWindow base class
	virtual bool
		AcceptsFocusFromKeyboard() const { return false; }

    void set_as_hidden() {
        Hide();
        hidden = true;
	}

    virtual bool Show(bool show = true) override {
        return wxButton::Show(hidden ? false : show);
	}
};

class Field {
protected:
    // factory function to defer and enforce creation of derived type. 
	virtual void	PostInitialize();
    
    /// Finish constructing the Field's wxWidget-related properties, including setting its own sizer, etc.
    virtual void	BUILD() = 0;

    /// Call the attached on_kill_focus method. 
	//! It's important to use wxEvent instead of wxFocusEvent,
	//! in another case we can't unfocused control at all
	void			on_kill_focus();
    /// Call the attached on_change method. 
    void			on_set_focus(wxEvent& event);
    /// Call the attached on_change method. 
    void			on_change_field();
    /// Call the attached m_back_to_initial_value method. 
	void			on_back_to_initial_value();
    /// Call the attached m_back_to_sys_value method. 
	void			on_back_to_sys_value();

public:
    /// parent wx item, opportunity to refactor (probably not necessary - data duplication)
    wxWindow*		m_parent {nullptr};

    /// Function object to store callback passed in from owning object.
	t_kill_focus	m_on_kill_focus {nullptr};

    /// Function object to store callback passed in from owning object.
	t_kill_focus	m_on_set_focus {nullptr};

    /// Function object to store callback passed in from owning object.
	t_change		m_on_change {nullptr};

	/// Function object to store callback passed in from owning object.
	t_back_to_init	m_back_to_initial_value{ nullptr };
	t_back_to_init	m_back_to_sys_value{ nullptr };

	// This is used to avoid recursive invocation of the field change/update by wxWidgets.
    bool			m_disable_change_event {false};
    bool			m_is_modified_value {false};
	bool			m_is_nonsys_value {true};

    /// Copy of ConfigOption for deduction purposes
    const ConfigOptionDef			m_opt {ConfigOptionDef()};
	const t_config_option_key		m_opt_id;//! {""};
	int								m_opt_idx = 0;

    /// Sets a value for this control.
    /// subclasses should overload with a specific version
    /// Postcondition: Method does not fire the on_change event.
    virtual void		set_value(const boost::any& value, bool change_event) = 0;
    virtual void        set_last_meaningful_value() {}
    virtual void        set_na_value() {}

    /// Gets a boost::any representing this control.
    /// subclasses should overload with a specific version
    virtual boost::any&	get_value() = 0;

    virtual void		enable() = 0;
    virtual void		disable() = 0;

	/// Fires the enable or disable function, based on the input.
    inline void			toggle(bool en) { en ? enable() : disable(); }

	virtual wxString	get_tooltip_text(const wxString& default_string);

    void				field_changed() { on_change_field(); }

    // set icon to "UndoToSystemValue" button according to an inheritance of preset
//	void				set_nonsys_btn_icon(const wxBitmap& icon);

    Field(const ConfigOptionDef& opt, const t_config_option_key& id) : m_opt(opt), m_opt_id(id) {};
    Field(wxWindow* parent, const ConfigOptionDef& opt, const t_config_option_key& id) : m_parent(parent), m_opt(opt), m_opt_id(id) {};
    virtual ~Field() {}

    /// If you don't know what you are getting back, check both methods for nullptr. 
    virtual wxSizer*	getSizer()  { return nullptr; }
    virtual wxWindow*	getWindow() { return nullptr; }

	bool				is_matched(const std::string& string, const std::string& pattern);
	void				get_value_by_opt_type(wxString& str);

    /// Factory method for generating new derived classes.
    template<class T>
    static t_field Create(wxWindow* parent, const ConfigOptionDef& opt, const t_config_option_key& id)// interface for creating shared objects
    {
        auto p = Slic3r::make_unique<T>(parent, opt, id);
        p->PostInitialize();
		return std::move(p); //!p;
    }

    bool 	set_undo_bitmap(const ScalableBitmap *bmp) {
    	if (m_undo_bitmap != bmp) {
    		m_undo_bitmap = bmp;
    		m_Undo_btn->SetBitmap_(*bmp);
    		return true;
    	}
    	return false;
    }

    bool 	set_undo_to_sys_bitmap(const ScalableBitmap *bmp) {
    	if (m_undo_to_sys_bitmap != bmp) {
    		m_undo_to_sys_bitmap = bmp;
    		m_Undo_to_sys_btn->SetBitmap_(*bmp);
    		return true;
    	}
    	return false;
    }

	bool	set_label_colour(const wxColour *clr) {
		if (m_Label == nullptr) return false;
		if (m_label_color != clr) {
			m_label_color = clr;
			m_Label->SetForegroundColour(*clr);
			m_Label->Refresh(true);
		}
		return false;
	}

	bool	set_label_colour_force(const wxColour *clr) {
		if (m_Label == nullptr) return false;
		m_Label->SetForegroundColour(*clr);
		m_Label->Refresh(true);
		return false;
	}

	bool 	set_undo_tooltip(const wxString *tip) {
		if (m_undo_tooltip != tip) {
			m_undo_tooltip = tip;
			m_Undo_btn->SetToolTip(*tip);
			return true;
		}
		return false;
	}

	bool 	set_undo_to_sys_tooltip(const wxString *tip) {
		if (m_undo_to_sys_tooltip != tip) {
			m_undo_to_sys_tooltip = tip;
			m_Undo_to_sys_btn->SetToolTip(*tip);
			return true;
		}
		return false;
	}

	void	set_side_text_ptr(wxStaticText* side_text) {
		m_side_text = side_text;
    }

    virtual void msw_rescale() {
        m_Undo_to_sys_btn->msw_rescale();
        m_Undo_btn->msw_rescale();

        // update em_unit value
        m_em_unit = em_unit(m_parent);
    }

    bool get_enter_pressed() const { return bEnterPressed; }
    void set_enter_pressed(bool pressed) { bEnterPressed = pressed; }

protected:
	RevertButton*			m_Undo_btn = nullptr;
	// Bitmap and Tooltip text for m_Undo_btn. The wxButton will be updated only if the new wxBitmap pointer differs from the currently rendered one.
	const ScalableBitmap*   m_undo_bitmap = nullptr;
	const wxString*         m_undo_tooltip = nullptr;
	RevertButton*			m_Undo_to_sys_btn = nullptr;
	// Bitmap and Tooltip text for m_Undo_to_sys_btn. The wxButton will be updated only if the new wxBitmap pointer differs from the currently rendered one.
    const ScalableBitmap*   m_undo_to_sys_bitmap = nullptr;
	const wxString*		    m_undo_to_sys_tooltip = nullptr;

	wxStaticText*		m_Label = nullptr;
	// Color for Label. The wxColour will be updated only if the new wxColour pointer differs from the currently rendered one.
	const wxColour*		m_label_color = nullptr;

	wxStaticText*		m_side_text = nullptr;

	// current value
	boost::any			m_value;
    // last maeningful value
	boost::any			m_last_meaningful_value;

    int                 m_em_unit;

    bool    bEnterPressed = false;
    
	friend class OptionsGroup;
};

/// Convenience function, accepts a const reference to t_field and checks to see whether 
/// or not both wx pointers are null.
inline bool is_bad_field(const t_field& obj) { return obj->getSizer() == nullptr && obj->getWindow() == nullptr; }

/// Covenience function to determine whether this field is a valid window field.
inline bool is_window_field(const t_field& obj) { return !is_bad_field(obj) && obj->getWindow() != nullptr && obj->getSizer() == nullptr; }

/// Covenience function to determine whether this field is a valid sizer field.
inline bool is_sizer_field(const t_field& obj) { return !is_bad_field(obj) && obj->getSizer() != nullptr; }

class TextCtrl : public Field {
    using Field::Field;
#ifdef __WXGTK__
	bool	bChangedValueEvent = true;
    void    change_field_value(wxEvent& event);
#endif //__WXGTK__
public:
	TextCtrl(const ConfigOptionDef& opt, const t_config_option_key& id) : Field(opt,  id) {}
	TextCtrl(wxWindow* parent, const ConfigOptionDef& opt, const t_config_option_key& id) : Field(parent, opt, id) {}
	~TextCtrl() {}

    void BUILD();
    bool value_was_changed();
    // Propagate value from field to the OptionGroupe and Config after kill_focus/ENTER
    void propagate_value();
    wxWindow* window {nullptr};

    virtual void	set_value(const std::string& value, bool change_event = false) {
		m_disable_change_event = !change_event;
        dynamic_cast<wxTextCtrl*>(window)->SetValue(wxString(value));
		m_disable_change_event = false;
    }
	virtual void	set_value(const boost::any& value, bool change_event = false) override;
    virtual void    set_last_meaningful_value() override;
    virtual void	set_na_value() override;

	boost::any&		get_value() override;

    void            msw_rescale() override;
    
    virtual void	enable();
    virtual void	disable();
    virtual wxWindow* getWindow() { return window; }
};

class CheckBox : public Field {
	using Field::Field;
    bool            m_is_na_val {false};
public:
	CheckBox(const ConfigOptionDef& opt, const t_config_option_key& id) : Field(opt, id) {}
	CheckBox(wxWindow* parent, const ConfigOptionDef& opt, const t_config_option_key& id) : Field(parent, opt, id) {}
	~CheckBox() {}

	wxWindow*		window{ nullptr };
	void			BUILD() override;

	void			set_value(const bool value, bool change_event = false) {
		m_disable_change_event = !change_event;
		dynamic_cast<wxCheckBox*>(window)->SetValue(value);
		m_disable_change_event = false;
	}
	void			set_value(const boost::any& value, bool change_event = false) override;
    void            set_last_meaningful_value() override;
	void            set_na_value() override;
	boost::any&		get_value() override;

    void            msw_rescale() override;

	void			enable() override { dynamic_cast<wxCheckBox*>(window)->Enable(); }
	void			disable() override { dynamic_cast<wxCheckBox*>(window)->Disable(); }
	wxWindow*		getWindow() override { return window; }
};

class SpinCtrl : public Field {
	using Field::Field;
private:
	static const int UNDEF_VALUE = INT_MIN;
public:
	SpinCtrl(const ConfigOptionDef& opt, const t_config_option_key& id) : Field(opt, id), tmp_value(UNDEF_VALUE) {}
	SpinCtrl(wxWindow* parent, const ConfigOptionDef& opt, const t_config_option_key& id) : Field(parent, opt, id), tmp_value(UNDEF_VALUE) {}
	~SpinCtrl() {}

	int				tmp_value;

	wxWindow*		window{ nullptr };
	void			BUILD() override;
    /// Propagate value from field to the OptionGroupe and Config after kill_focus/ENTER
    void	        propagate_value() ;

	void			set_value(const std::string& value, bool change_event = false) {
		m_disable_change_event = !change_event;
		dynamic_cast<wxSpinCtrl*>(window)->SetValue(value);
		m_disable_change_event = false;
	}
	void			set_value(const boost::any& value, bool change_event = false) {
		m_disable_change_event = !change_event;
		tmp_value = boost::any_cast<int>(value);
        m_value = value;
		dynamic_cast<wxSpinCtrl*>(window)->SetValue(tmp_value);
		m_disable_change_event = false;
	}

	boost::any&		get_value() override {
		int value = static_cast<wxSpinCtrl*>(window)->GetValue();
		return m_value = value;
	}

    void            msw_rescale() override;

	void			enable() override { dynamic_cast<wxSpinCtrl*>(window)->Enable(); }
	void			disable() override { dynamic_cast<wxSpinCtrl*>(window)->Disable(); }
	wxWindow*		getWindow() override { return window; }
};

class Choice : public Field {
	using Field::Field;
    int             m_width{ 15 };
public:
	Choice(const ConfigOptionDef& opt, const t_config_option_key& id) : Field(opt, id) {}
	Choice(wxWindow* parent, const ConfigOptionDef& opt, const t_config_option_key& id) : Field(parent, opt, id) {}
	~Choice() {}

	wxWindow*		window{ nullptr };
	void			BUILD() override;

    /* Under OSX: wxBitmapComboBox->GetWindowStyle() returns some weard value, 
     * so let use a flag, which has TRUE value for a control without wxCB_READONLY style
     */
    bool            m_is_editable { false };

	void			set_selection();
	void			set_value(const std::string& value, bool change_event = false);
	void			set_value(const boost::any& value, bool change_event = false);
	void			set_values(const std::vector<std::string> &values);
	boost::any&		get_value() override;

    void            msw_rescale() override;

	void			enable() override { dynamic_cast<wxBitmapComboBox*>(window)->Enable(); };
	void			disable() override{ dynamic_cast<wxBitmapComboBox*>(window)->Disable(); };
	wxWindow*		getWindow() override { return window; }
};

class ColourPicker : public Field {
	using Field::Field;

    void            set_undef_value(wxColourPickerCtrl* field);
public:
	ColourPicker(const ConfigOptionDef& opt, const t_config_option_key& id) : Field(opt, id) {}
	ColourPicker(wxWindow* parent, const ConfigOptionDef& opt, const t_config_option_key& id) : Field(parent, opt, id) {}
	~ColourPicker() {}

	wxWindow*		window{ nullptr };
	void			BUILD()  override;

	void			set_value(const std::string& value, bool change_event = false) {
		m_disable_change_event = !change_event;
		dynamic_cast<wxColourPickerCtrl*>(window)->SetColour(value);
		m_disable_change_event = false;
	 	}
	void			set_value(const boost::any& value, bool change_event = false) override;
	boost::any&		get_value() override;
    void            msw_rescale() override;

	void			enable() override { dynamic_cast<wxColourPickerCtrl*>(window)->Enable(); };
	void			disable() override{ dynamic_cast<wxColourPickerCtrl*>(window)->Disable(); };
	wxWindow*		getWindow() override { return window; }
};

class PointCtrl : public Field {
	using Field::Field;
public:
	PointCtrl(const ConfigOptionDef& opt, const t_config_option_key& id) : Field(opt, id) {}
	PointCtrl(wxWindow* parent, const ConfigOptionDef& opt, const t_config_option_key& id) : Field(parent, opt, id) {}
	~PointCtrl() {}

	wxSizer*		sizer{ nullptr };
	wxTextCtrl*		x_textctrl{ nullptr };
	wxTextCtrl*		y_textctrl{ nullptr };

	void			BUILD()  override;
    // Propagate value from field to the OptionGroupe and Config after kill_focus/ENTER
    void            propagate_value(wxTextCtrl* win);
	void			set_value(const Vec2d& value, bool change_event = false);
	void			set_value(const boost::any& value, bool change_event = false);
	boost::any&		get_value() override;

    void            msw_rescale() override;

	void			enable() override {
		x_textctrl->Enable();
		y_textctrl->Enable(); }
	void			disable() override{
		x_textctrl->Disable();
		y_textctrl->Disable(); }
	wxSizer*		getSizer() override { return sizer; }
};

class StaticText : public Field {
	using Field::Field;
public:
	StaticText(const ConfigOptionDef& opt, const t_config_option_key& id) : Field(opt, id) {}
	StaticText(wxWindow* parent, const ConfigOptionDef& opt, const t_config_option_key& id) : Field(parent, opt, id) {}
	~StaticText() {}

	wxWindow*		window{ nullptr };
	void			BUILD()  override;

	void			set_value(const std::string& value, bool change_event = false) {
		m_disable_change_event = !change_event;
		dynamic_cast<wxStaticText*>(window)->SetLabel(wxString::FromUTF8(value.data()));
		m_disable_change_event = false;
	}
	void			set_value(const boost::any& value, bool change_event = false) {
		m_disable_change_event = !change_event;
		dynamic_cast<wxStaticText*>(window)->SetLabel(boost::any_cast<wxString>(value));
		m_disable_change_event = false;
	}

	boost::any&		get_value()override { return m_value; }

    void            msw_rescale() override;

	void			enable() override { dynamic_cast<wxStaticText*>(window)->Enable(); };
	void			disable() override{ dynamic_cast<wxStaticText*>(window)->Disable(); };
	wxWindow*		getWindow() override { return window; }
};

class SliderCtrl : public Field {
	using Field::Field;
public:
	SliderCtrl(const ConfigOptionDef& opt, const t_config_option_key& id) : Field(opt, id) {}
	SliderCtrl(wxWindow* parent, const ConfigOptionDef& opt, const t_config_option_key& id) : Field(parent, opt, id) {}
	~SliderCtrl() {}

	wxSizer*		m_sizer{ nullptr };
	wxTextCtrl*		m_textctrl{ nullptr };
	wxSlider*		m_slider{ nullptr };

	int				m_scale = 10;

	void			BUILD()  override;

	void			set_value(const int value, bool change_event = false);
	void			set_value(const boost::any& value, bool change_event = false);
	boost::any&		get_value() override;

	void			enable() override {
		m_slider->Enable();
		m_textctrl->Enable();
		m_textctrl->SetEditable(true);
	}
	void			disable() override{
		m_slider->Disable();
		m_textctrl->Disable();
		m_textctrl->SetEditable(false);
	}
	wxSizer*		getSizer() override { return m_sizer; }
	wxWindow*		getWindow() override { return dynamic_cast<wxWindow*>(m_slider); }
};

} // GUI
} // Slic3r

#endif /* SLIC3R_GUI_FIELD_HPP */
