#include "toast_options.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void ToastOptions::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_icon_path", "path"), &ToastOptions::set_icon_path);
	ClassDB::bind_method(D_METHOD("get_icon_path"), &ToastOptions::get_icon_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "icon_path", PROPERTY_HINT_FILE, "*.png,*.jpg,*.jpeg,*.gif"), "set_icon_path", "get_icon_path");

	ClassDB::bind_method(D_METHOD("set_actions", "actions"), &ToastOptions::set_actions);
	ClassDB::bind_method(D_METHOD("get_actions"), &ToastOptions::get_actions);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "actions"), "set_actions", "get_actions");

	ClassDB::bind_method(D_METHOD("set_image_crop", "crop"), &ToastOptions::set_image_crop);
	ClassDB::bind_method(D_METHOD("get_image_crop"), &ToastOptions::get_image_crop);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "image_crop", PROPERTY_HINT_ENUM, "Square,Circle"), "set_image_crop", "get_image_crop");

	ClassDB::bind_method(D_METHOD("set_duration", "duration"), &ToastOptions::set_duration);
	ClassDB::bind_method(D_METHOD("get_duration"), &ToastOptions::get_duration);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "duration", PROPERTY_HINT_ENUM, "System,Short,Long"), "set_duration", "get_duration");

	ClassDB::bind_method(D_METHOD("set_sound", "sound"), &ToastOptions::set_sound);
	ClassDB::bind_method(D_METHOD("get_sound"), &ToastOptions::get_sound);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sound", PROPERTY_HINT_ENUM,
						 "Default,IM,Mail,Reminder,SMS,Alarm,Alarm2,Alarm3,Alarm4,Alarm5,Alarm6,Alarm7,Alarm8,Alarm9,Alarm10,"
						 "Call,Call1,Call2,Call3,Call4,Call5,Call6,Call7,Call8,Call9,Call10"),
			"set_sound", "get_sound");

	ClassDB::bind_method(D_METHOD("set_audio_option", "option"), &ToastOptions::set_audio_option);
	ClassDB::bind_method(D_METHOD("get_audio_option"), &ToastOptions::get_audio_option);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "audio_option", PROPERTY_HINT_ENUM, "Default,Silent,Loop"), "set_audio_option", "get_audio_option");

	ClassDB::bind_method(D_METHOD("set_replace_previous", "enabled"), &ToastOptions::set_replace_previous);
	ClassDB::bind_method(D_METHOD("is_replace_previous"), &ToastOptions::is_replace_previous);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "replace_previous"), "set_replace_previous", "is_replace_previous");
}
