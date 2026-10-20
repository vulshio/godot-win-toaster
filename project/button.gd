extends Button

var toaster: WinToaster = null

func _ready() -> void:
	toaster = WinToaster.new()
	toaster.initialize("Toaster", "Godot.WinToaster", false)

	toaster.toast_shown.connect(_on_toast_shown)
	toaster.toast_activated.connect(_on_toast_activated)
	toaster.toast_failed.connect(_on_toast_failed)

func _pressed() -> void:
	var options := ToastOptions.new()
	options.icon_path = ProjectSettings.globalize_path("res://ice.png")
	options.image_crop = WinToaster.TOAST_IMAGE_CROP_CIRCLE
	options.replace_previous = true

	toaster.show_toast("hello", "冰冰冰!", options)

func _on_toast_shown(toast_id: int) -> void:
	print("toast_shown: ", toast_id)

func _on_toast_activated(toast_id: int) -> void:
	print("toast_activated: ", toast_id)

func _on_toast_failed(toast_id: int) -> void:
	print("toast_failed: ", toast_id)
