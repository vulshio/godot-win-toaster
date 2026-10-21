extends Control

const TOAST_APP_NAME := "Toaster"
const TOAST_APP_ID := "Godot.Win.Toaster"
const REPEAT_INTERVAL := 5.0
const ChatBubbleScript := preload("res://chat_bubble.gd")

@export var chat_bubble_scene: PackedScene = preload("res://chat_bubble.tscn")

@export var chat_box: VBoxContainer
@export var input_line: LineEdit
@export var repeat_button: Button
@export var send_button: Button
@export var hide_button: Button

var _toaster: WinToaster
var _repeat_timer: Timer
var _window_hidden := false


func _ready() -> void:
	_toaster = WinToaster.new()
	if not _toaster.initialize(TOAST_APP_NAME, TOAST_APP_ID):
		push_error("WinToaster initialization failed")

	_toaster.toast_shown.connect(_on_toast_shown)
	_toaster.toast_activated.connect(_on_toast_activated)
	_toaster.toast_action.connect(_on_toast_action)
	_toaster.toast_failed.connect(_on_toast_failed)

	send_button.pressed.connect(_send_from_input)
	input_line.text_submitted.connect(_on_text_submitted)
	repeat_button.toggled.connect(_on_repeat_toggled)
	hide_button.pressed.connect(_on_hide_pressed)

	_repeat_timer = Timer.new()
	_repeat_timer.wait_time = REPEAT_INTERVAL
	_repeat_timer.timeout.connect(_on_repeat_timeout)
	add_child(_repeat_timer)

	_window_hidden = _is_window_hidden()
	get_window().visibility_changed.connect(_on_window_visibility_changed)


func _send_from_input() -> void:
	var text := input_line.text.strip_edges()
	if text.is_empty():
		return
	input_line.clear()
	_send_message(text)


func _on_text_submitted(_text: String) -> void:
	_send_from_input()


func _send_message(text: String) -> void:
	_add_bubble(text)
	# 窗口不可见时，聊天消息改从 Windows toast 发出。
	_window_hidden = _is_window_hidden()
	if _window_hidden:
		_show_toast(text)


# Godot 4.7 不允许对主窗口调用 hide()（"Can't change visibility of main window"），
# 最小化是引擎内唯一能让主窗口从屏幕上消失的方式。
func _is_window_hidden() -> bool:
	var window := get_window()
	return not window.visible or window.mode == Window.MODE_MINIMIZED


func _add_bubble(text: String) -> void:
	var bubble: ChatBubbleScript = chat_bubble_scene.instantiate()
	bubble.set_message(text)
	chat_box.add_child(bubble)
	await get_tree().process_frame
	var scroll := chat_box.get_parent() as ScrollContainer
	if scroll != null:
		scroll.scroll_vertical = int(scroll.get_v_scroll_bar().max_value)


func _on_repeat_toggled(pressed: bool) -> void:
	if pressed:
		_repeat_timer.start()
	else:
		_repeat_timer.stop()


func _on_repeat_timeout() -> void:
	# 不清空输入框，这样每隔 REPEAT_INTERVAL 秒都能重复同一句话。
	var text := input_line.text.strip_edges()
	if text.is_empty():
		return
	_send_message(text)


func _on_hide_pressed() -> void:
	get_window().mode = Window.MODE_MINIMIZED


func _on_window_visibility_changed() -> void:
	_window_hidden = _is_window_hidden()
	print("window hidden: ", _window_hidden)


func _restore_window() -> void:
	var window := get_window()
	if window.mode == Window.MODE_MINIMIZED:
		window.mode = Window.MODE_WINDOWED
	window.show()
	window.grab_focus()


func _show_toast(text: String) -> void:
	if _toaster == null or not _toaster.is_initialized():
		return

	var options := ToastOptions.new()
	options.icon_path = ProjectSettings.globalize_path("res://ice.png")
	options.image_crop = WinToaster.TOAST_IMAGE_CROP_CIRCLE
	options.sound = WinToaster.TOAST_SOUND_IM
	# 每 5 秒重复一次，顶掉上一条，避免通知堆积。
	options.replace_previous = true
	options.actions = ["显示窗口"]

	var toast_id := _toaster.show_toast("冰冰冰", text, options)
	if toast_id < 0:
		push_warning("show_toast failed")


func _on_toast_shown(toast_id: int) -> void:
	print("toast_shown: ", toast_id)


func _on_toast_activated(_toast_id: int) -> void:
	_restore_window()


func _on_toast_action(_toast_id: int, action_index: int) -> void:
	if action_index == 0:
		_restore_window()


func _on_toast_failed(toast_id: int) -> void:
	print("toast_failed: ", toast_id)
