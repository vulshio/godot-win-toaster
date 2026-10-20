extends Button

@export var with_image: bool = true
@export var with_action: bool = true
@export var other_sound: bool = false

var toaster: WinToaster = null

func _ready() -> void:
	toaster = WinToaster.new()
	toaster.initialize("Toaster", "Godot.Win.Toaster", true)

	toaster.toast_shown.connect(_on_toast_shown)
	toaster.toast_activated.connect(_on_toast_activated)
	toaster.toast_action.connect(_on_toast_action)
	toaster.toast_failed.connect(_on_toast_failed)

func _pressed() -> void:
	var options := ToastOptions.new()
	options.image_crop = WinToaster.TOAST_IMAGE_CROP_CIRCLE
	options.replace_previous = true

	if with_image:
		options.icon_path = ProjectSettings.globalize_path("res://ice.png")

	if with_action:
		options.actions = ["Okay", "Nope"]
	
	if other_sound:
		options.sound = WinToaster.TOAST_SOUND_IM

	toaster.show_toast("hello", "冰冰冰!", options)

func _on_toast_shown(toast_id: int) -> void:
	print("toast_shown: ", toast_id)

func _on_toast_activated(toast_id: int) -> void:
	print("toast_activated: ", toast_id)

func _on_toast_action(toast_id: int, action_index: int) -> void:
	print("toast_action: ", toast_id, " index: ", action_index)

func _on_toast_failed(toast_id: int) -> void:
	print("toast_failed: ", toast_id)
