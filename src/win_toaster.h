#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class ToastOptions;

// Thin GDExtension wrapper around WinToast.
// Windows toast notifications are a process-wide resource, so a single
// WinToast instance is shared by every WinToaster object.
class WinToaster : public RefCounted {
	GDCLASS(WinToaster, RefCounted)

public:
	enum ToastDuration {
		TOAST_DURATION_SYSTEM = 0,
		TOAST_DURATION_SHORT = 1,
		TOAST_DURATION_LONG = 2,
	};

	enum ToastAudioOption {
		TOAST_AUDIO_DEFAULT = 0,
		TOAST_AUDIO_SILENT = 1,
		TOAST_AUDIO_LOOP = 2,
	};

	enum ToastImageCrop {
		TOAST_IMAGE_CROP_SQUARE = 0,
		TOAST_IMAGE_CROP_CIRCLE = 1,
	};

	// System sounds. The order must match WinToastTemplate::AudioSystemFile,
	// which maps these to ms-winsoundevent:Notification.* URIs.
	enum ToastSound {
		TOAST_SOUND_DEFAULT = 0,
		TOAST_SOUND_IM,
		TOAST_SOUND_MAIL,
		TOAST_SOUND_REMINDER,
		TOAST_SOUND_SMS,
		TOAST_SOUND_ALARM,
		TOAST_SOUND_ALARM2,
		TOAST_SOUND_ALARM3,
		TOAST_SOUND_ALARM4,
		TOAST_SOUND_ALARM5,
		TOAST_SOUND_ALARM6,
		TOAST_SOUND_ALARM7,
		TOAST_SOUND_ALARM8,
		TOAST_SOUND_ALARM9,
		TOAST_SOUND_ALARM10,
		TOAST_SOUND_CALL,
		TOAST_SOUND_CALL1,
		TOAST_SOUND_CALL2,
		TOAST_SOUND_CALL3,
		TOAST_SOUND_CALL4,
		TOAST_SOUND_CALL5,
		TOAST_SOUND_CALL6,
		TOAST_SOUND_CALL7,
		TOAST_SOUND_CALL8,
		TOAST_SOUND_CALL9,
		TOAST_SOUND_CALL10,
	};

protected:
	static void _bind_methods();

public:
	WinToaster() = default;
	~WinToaster() override = default;

	// True if the current Windows version supports toast notifications.
	static bool is_available();

	// Must be called once before show_toast(). Calling it again is a no-op.
	// p_app_name is shown in the notification's app name field, and p_app_id is
	// the AppUserModelID (e.g. "MyCompany.MyGame").
	// p_create_shortcut lets WinToast create/update a Start Menu shortcut for the
	// AppUserModelID, which some Windows versions require for toasts to appear.
	bool initialize(const String &p_app_name, const String &p_app_id, bool p_create_shortcut = true);
	bool is_initialized() const;

	// Shows a two-line toast and returns its toast id, or -1 on failure.
	// Optional presentation settings (icon, duration, sound, image crop and
	// replace behaviour) are passed through ToastOptions; null means defaults.
	int64_t show_toast(const String &p_title, const String &p_message, const Ref<ToastOptions> &p_options = Ref<ToastOptions>());

	bool hide_toast(int64_t p_toast_id);
	void clear();
};

} // namespace godot

VARIANT_ENUM_CAST(godot::WinToaster::ToastDuration);
VARIANT_ENUM_CAST(godot::WinToaster::ToastAudioOption);
VARIANT_ENUM_CAST(godot::WinToaster::ToastImageCrop);
VARIANT_ENUM_CAST(godot::WinToaster::ToastSound);
