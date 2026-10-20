extends Node


func _ready() -> void:
	var toaster := WinToaster.new()

	print("WinToaster.is_available(): ", WinToaster.is_available())
	if not toaster.initialize("Godot Win Toaster", "Godot.WinToaster"):
		push_error("WinToaster initialization failed")
		return

	toaster.toast_shown.connect(_on_toast_shown)
	toaster.toast_activated.connect(_on_toast_activated)
	toaster.toast_failed.connect(_on_toast_failed)

	var options := ToastOptions.new()
	options.icon_path = ProjectSettings.globalize_path("res://ice.png")
	options.image_crop = WinToaster.TOAST_IMAGE_CROP_CIRCLE
	options.duration = WinToaster.TOAST_DURATION_LONG
	options.sound = WinToaster.TOAST_SOUND_IM

	var toast_id := toaster.show_toast("godot-win-toaster", "Hello from GDExtension!", options)
	print("Toast id: ", toast_id)


func _on_toast_shown(toast_id: int) -> void:
	print("toast_shown: ", toast_id)


func _on_toast_activated(toast_id: int) -> void:
	print("toast_activated: ", toast_id)


func _on_toast_failed(toast_id: int) -> void:
	print("toast_failed: ", toast_id)
