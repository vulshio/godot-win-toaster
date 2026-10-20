#include "win_toaster.h"

#include <wintoastlib.h>

#include <atomic>
#include <memory>

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "toast_options.h"

using namespace WinToastLib;

static_assert(static_cast<int>(godot::WinToaster::TOAST_SOUND_DEFAULT) == static_cast<int>(WinToastTemplate::DefaultSound) &&
			  static_cast<int>(godot::WinToaster::TOAST_SOUND_CALL10) == static_cast<int>(WinToastTemplate::Call10),
		"ToastSound must mirror WinToastTemplate::AudioSystemFile");

namespace {

// Godot strings are UTF-16, which matches wchar_t on Windows, so the buffer
// can be reused without an extra conversion.
std::wstring to_wide(const godot::String &p_string) {
	const godot::Char16String utf16 = p_string.utf16();
	return std::wstring(reinterpret_cast<const wchar_t *>(utf16.get_data()));
}

// WinToast takes ownership of the handler and refuses to show a toast without
// one. The toast id is only known after showToast() returns, so it is shared
// with the handler through an atomic set afterwards.
// Callbacks run on a Windows Runtime thread, therefore the signal is emitted
// through call_deferred() to get back onto the main thread. A weak ObjectID is
// used instead of a Ref so a pending toast never keeps the Godot object alive.
class ToastHandler : public IWinToastHandler {
public:
	ToastHandler(uint64_t p_toaster_id, std::shared_ptr<std::atomic<int64_t>> p_toast_id) :
			_toaster_id(p_toaster_id), _toast_id(std::move(p_toast_id)) {}

	void toastActivated() const override { notify_signal("toast_activated"); }
	void toastActivated(int) const override { notify_signal("toast_activated"); }
	void toastActivated(std::wstring) const override { notify_signal("toast_activated"); }
	void toastDismissed(WinToastDismissalReason) const override {}
	void toastFailed() const override { notify_signal("toast_failed"); }

private:
	void notify_signal(const char *p_signal) const {
		godot::Object *toaster = godot::ObjectDB::get_instance(_toaster_id);
		if (toaster != nullptr) {
			toaster->call_deferred("emit_signal", godot::StringName(p_signal), _toast_id->load());
		}
	}

	uint64_t _toaster_id;
	std::shared_ptr<std::atomic<int64_t>> _toast_id;
};

} // namespace

namespace godot {

bool WinToaster::is_available() {
	return WinToast::isCompatible();
}

bool WinToaster::initialize(const String &p_app_name, const String &p_app_id, bool p_create_shortcut) {
	if (WinToast::instance()->isInitialized()) {
		return true;
	}
	if (!WinToast::isCompatible()) {
		UtilityFunctions::push_warning("WinToaster: toast notifications are not supported on this system.");
		return false;
	}

	WinToast::instance()->setAppName(to_wide(p_app_name));
	WinToast::instance()->setAppUserModelId(to_wide(p_app_id));
	WinToast::instance()->setShortcutPolicy(
			p_create_shortcut ? WinToast::SHORTCUT_POLICY_REQUIRE_CREATE : WinToast::SHORTCUT_POLICY_IGNORE);

	WinToast::WinToastError error = WinToast::NoError;
	if (!WinToast::instance()->initialize(&error)) {
		UtilityFunctions::push_error(
				"WinToaster: WinToast initialization failed (error " +
				String::num_int64(static_cast<int64_t>(error)) + ").");
		return false;
	}
	return true;
}

bool WinToaster::is_initialized() const {
	return WinToast::instance()->isInitialized();
}

int64_t WinToaster::show_toast(const String &p_title, const String &p_message, const Ref<ToastOptions> &p_options) {
	if (!WinToast::instance()->isInitialized()) {
		UtilityFunctions::push_error("WinToaster: call initialize() before show_toast().");
		return -1;
	}

	const bool has_options = p_options.is_valid();
	const String icon_path = has_options ? p_options->get_icon_path() : String();
	const ToastImageCrop image_crop = has_options ? p_options->get_image_crop() : TOAST_IMAGE_CROP_SQUARE;
	const ToastDuration duration = has_options ? p_options->get_duration() : TOAST_DURATION_SYSTEM;
	const ToastSound sound = has_options ? p_options->get_sound() : TOAST_SOUND_DEFAULT;
	const ToastAudioOption audio_option = has_options ? p_options->get_audio_option() : TOAST_AUDIO_DEFAULT;
	const bool replace_previous = has_options ? p_options->is_replace_previous() : false;

	const bool has_icon = !icon_path.is_empty();
	WinToastTemplate toast(has_icon ? WinToastTemplate::ImageAndText02 : WinToastTemplate::Text02);
	toast.setTextField(to_wide(p_title), WinToastTemplate::FirstLine);
	toast.setTextField(to_wide(p_message), WinToastTemplate::SecondLine);
	if (has_icon) {
		toast.setImagePath(to_wide(icon_path),
				image_crop == TOAST_IMAGE_CROP_CIRCLE ? WinToastTemplate::Circle : WinToastTemplate::Square);
	}

	// A loop needs a sound source, so explicitly select the default sound then.
	if (sound != TOAST_SOUND_DEFAULT || audio_option == TOAST_AUDIO_LOOP) {
		toast.setAudioPath(static_cast<WinToastTemplate::AudioSystemFile>(sound));
	}
	switch (audio_option) {
		case TOAST_AUDIO_SILENT:
			toast.setAudioOption(WinToastTemplate::Silent);
			break;
		case TOAST_AUDIO_LOOP:
			toast.setAudioOption(WinToastTemplate::Loop);
			break;
		case TOAST_AUDIO_DEFAULT:
		default:
			toast.setAudioOption(WinToastTemplate::Default);
			break;
	}

	switch (duration) {
		case TOAST_DURATION_SHORT:
			toast.setDuration(WinToastTemplate::Short);
			break;
		case TOAST_DURATION_LONG:
			toast.setDuration(WinToastTemplate::Long);
			break;
		case TOAST_DURATION_SYSTEM:
		default:
			toast.setDuration(WinToastTemplate::System);
			break;
	}

	if (replace_previous) {
		// Windows displays toasts from the same app one at a time and queues the
		// rest. Hiding the buffered ones makes room so this toast is shown right
		// away instead of after the pending notifications.
		WinToast::instance()->clear();
	}

	WinToast::WinToastError error = WinToast::NoError;
	auto toast_id_state = std::make_shared<std::atomic<int64_t>>(-1);
	auto *handler = new ToastHandler(get_instance_id(), toast_id_state);
	const INT64 toast_id = WinToast::instance()->showToast(toast, handler, &error);
	if (toast_id < 0) {
		UtilityFunctions::push_error(
				"WinToaster: failed to show toast (error " +
				String::num_int64(static_cast<int64_t>(error)) + ").");
		return toast_id;
	}

	toast_id_state->store(toast_id);
	emit_signal("toast_shown", toast_id);
	return toast_id;
}

bool WinToaster::hide_toast(int64_t p_toast_id) {
	return WinToast::instance()->hideToast(p_toast_id);
}

void WinToaster::clear() {
	WinToast::instance()->clear();
}

void WinToaster::_bind_methods() {
	ADD_SIGNAL(MethodInfo("toast_shown", PropertyInfo(Variant::INT, "toast_id")));
	ADD_SIGNAL(MethodInfo("toast_activated", PropertyInfo(Variant::INT, "toast_id")));
	ADD_SIGNAL(MethodInfo("toast_failed", PropertyInfo(Variant::INT, "toast_id")));

	BIND_ENUM_CONSTANT(TOAST_DURATION_SYSTEM);
	BIND_ENUM_CONSTANT(TOAST_DURATION_SHORT);
	BIND_ENUM_CONSTANT(TOAST_DURATION_LONG);
	BIND_ENUM_CONSTANT(TOAST_AUDIO_DEFAULT);
	BIND_ENUM_CONSTANT(TOAST_AUDIO_SILENT);
	BIND_ENUM_CONSTANT(TOAST_AUDIO_LOOP);
	BIND_ENUM_CONSTANT(TOAST_IMAGE_CROP_SQUARE);
	BIND_ENUM_CONSTANT(TOAST_IMAGE_CROP_CIRCLE);
	BIND_ENUM_CONSTANT(TOAST_SOUND_DEFAULT);
	BIND_ENUM_CONSTANT(TOAST_SOUND_IM);
	BIND_ENUM_CONSTANT(TOAST_SOUND_MAIL);
	BIND_ENUM_CONSTANT(TOAST_SOUND_REMINDER);
	BIND_ENUM_CONSTANT(TOAST_SOUND_SMS);
	BIND_ENUM_CONSTANT(TOAST_SOUND_ALARM);
	BIND_ENUM_CONSTANT(TOAST_SOUND_ALARM2);
	BIND_ENUM_CONSTANT(TOAST_SOUND_ALARM3);
	BIND_ENUM_CONSTANT(TOAST_SOUND_ALARM4);
	BIND_ENUM_CONSTANT(TOAST_SOUND_ALARM5);
	BIND_ENUM_CONSTANT(TOAST_SOUND_ALARM6);
	BIND_ENUM_CONSTANT(TOAST_SOUND_ALARM7);
	BIND_ENUM_CONSTANT(TOAST_SOUND_ALARM8);
	BIND_ENUM_CONSTANT(TOAST_SOUND_ALARM9);
	BIND_ENUM_CONSTANT(TOAST_SOUND_ALARM10);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL1);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL2);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL3);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL4);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL5);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL6);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL7);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL8);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL9);
	BIND_ENUM_CONSTANT(TOAST_SOUND_CALL10);

	ClassDB::bind_static_method("WinToaster", D_METHOD("is_available"), &WinToaster::is_available);

	ClassDB::bind_method(D_METHOD("initialize", "app_name", "app_id", "create_shortcut"), &WinToaster::initialize, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("is_initialized"), &WinToaster::is_initialized);
	ClassDB::bind_method(D_METHOD("show_toast", "title", "message", "options"), &WinToaster::show_toast, DEFVAL(Ref<ToastOptions>()));
	ClassDB::bind_method(D_METHOD("hide_toast", "toast_id"), &WinToaster::hide_toast);
	ClassDB::bind_method(D_METHOD("clear"), &WinToaster::clear);
}

} // namespace godot
