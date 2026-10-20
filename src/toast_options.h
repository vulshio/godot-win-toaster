#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "win_toaster.h"

namespace godot {

// Optional presentation settings for WinToaster::show_toast().
// Every property has a default, so only the ones you care about need to be set.
class ToastOptions : public RefCounted {
	GDCLASS(ToastOptions, RefCounted)

	String icon_path;
	PackedStringArray actions;
	WinToaster::ToastImageCrop image_crop = WinToaster::TOAST_IMAGE_CROP_SQUARE;
	WinToaster::ToastDuration duration = WinToaster::TOAST_DURATION_SYSTEM;
	WinToaster::ToastSound sound = WinToaster::TOAST_SOUND_DEFAULT;
	WinToaster::ToastAudioOption audio_option = WinToaster::TOAST_AUDIO_DEFAULT;
	bool replace_previous = false;

protected:
	static void _bind_methods();

public:
	// Absolute path to a PNG/JPG/GIF shown in the toast. Empty means no icon.
	void set_icon_path(const String &p_icon_path) { icon_path = p_icon_path; }
	String get_icon_path() const { return icon_path; }

	// Buttons shown on the toast. Windows supports up to 5; extra entries are
	// ignored. Clicking one emits WinToaster.toast_action with its index.
	void set_actions(const PackedStringArray &p_actions) { actions = p_actions; }
	PackedStringArray get_actions() const { return actions; }

	void set_image_crop(WinToaster::ToastImageCrop p_image_crop) { image_crop = p_image_crop; }
	WinToaster::ToastImageCrop get_image_crop() const { return image_crop; }

	void set_duration(WinToaster::ToastDuration p_duration) { duration = p_duration; }
	WinToaster::ToastDuration get_duration() const { return duration; }

	// System sound to play (TOAST_SOUND_*) - custom audio files are not reliably
	// available to unpackaged desktop apps.
	void set_sound(WinToaster::ToastSound p_sound) { sound = p_sound; }
	WinToaster::ToastSound get_sound() const { return sound; }

	void set_audio_option(WinToaster::ToastAudioOption p_audio_option) { audio_option = p_audio_option; }
	WinToaster::ToastAudioOption get_audio_option() const { return audio_option; }

	// Hide toasts that are still displayed or queued before showing this one.
	void set_replace_previous(bool p_replace_previous) { replace_previous = p_replace_previous; }
	bool is_replace_previous() const { return replace_previous; }
};

} // namespace godot
