# godot-win-toaster

基于 GDExtension 的 Windows toast 通知插件。底层使用
[WinToast 1.3.2](thirdparty/WinToast/)，在 Godot 中提供 `WinToaster` 和
`ToastOptions` 两个类。

## 功能

- 发送两行文本 toast，支持 PNG/JPG/GIF 图标，图标可裁剪为方形或圆形
- 时长可选系统默认 / 短（约 7 秒）/ 长（约 25 秒）
- 系统提示音（默认、IM、邮件、提醒、短信、10 种闹钟、10 种来电）
- 提示音可静音或循环播放
- 最多 5 个操作按钮，点击回调带上按钮索引
- 可选替换模式：新通知立即顶掉当前显示/排队的通知
- 信号：`toast_shown`、`toast_activated`、`toast_action`、`toast_failed`

## 环境要求

- Windows 10 / 11
- Godot 4.3+（扩展按 4.3 API 编译）
- 编译：SCons 4.x + MSVC（Visual Studio 2022）
- 文档翻译插件需要 Godot 4.5+

## 构建

```shell
git submodule update --init --recursive

scons                          # template_debug
scons release                  # template_release（等价于 scons target=template_release）
scons dev_build=yes            # 带调试符号，可设置断点
```

产物输出到 `bin/windows/`，并自动复制到 `project/bin/windows/`：

```
bin/windows/godot_win_toaster.windows.template_debug.x86_64.dll
bin/windows/godot_win_toaster.windows.template_release.x86_64.dll
```

`dev_build=yes` 不会改变文件名，便于配合 `.gdextension` 和 VSCode 调试配置。

## 安装到其他项目

`.gdextension` 里使用相对路径引用库文件，复制时保持目录结构即可：

```
addons/godot_win_toaster/
├── godot_win_toaster.gdextension
└── windows/
    ├── godot_win_toaster.windows.template_debug.x86_64.dll
    └── godot_win_toaster.windows.template_release.x86_64.dll
```

## 使用

```gdscript
var toaster := WinToaster.new()

# create_shortcut 默认 true：创建/更新一个注册 AppUserModelID 的开始菜单快捷方式
if not toaster.initialize("My Game", "MyCompany.MyGame"):
	push_error("WinToaster initialization failed")

toaster.toast_shown.connect(func(id): print("shown: ", id))
toaster.toast_activated.connect(func(id): print("clicked: ", id))
toaster.toast_action.connect(func(id, index): print("button ", index, " on ", id))
toaster.toast_failed.connect(func(id): print("failed: ", id))

var options := ToastOptions.new()
options.icon_path = "C:/absolute/path/icon.png"
options.image_crop = WinToaster.TOAST_IMAGE_CROP_CIRCLE
options.duration = WinToaster.TOAST_DURATION_LONG
options.sound = WinToaster.TOAST_SOUND_IM
options.audio_option = WinToaster.TOAST_AUDIO_DEFAULT
options.actions = ["打开", "忽略"]
options.replace_previous = true

toaster.show_toast("标题", "正文", options)

# 不传 options 时全部使用默认值
toaster.show_toast("标题", "正文")
```

### 接口

| 方法 | 说明 |
| --- | --- |
| `WinToaster.is_available()` | 当前系统是否支持 toast |
| `initialize(app_name, app_id, create_shortcut = true)` | 初始化，每个进程只生效一次 |
| `is_initialized()` | 是否已初始化 |
| `show_toast(title, message, options = null)` | 发送通知，返回 toast id，失败为 `-1` |
| `hide_toast(toast_id)` | 隐藏指定通知 |
| `clear()` | 隐藏所有通知 |

### ToastOptions

| 属性 | 默认值 | 说明 |
| --- | --- | --- |
| `icon_path` | `""` | 图标的绝对文件路径 |
| `image_crop` | `TOAST_IMAGE_CROP_SQUARE` | 方形 / 圆形裁剪 |
| `duration` | `TOAST_DURATION_SYSTEM` | 系统 / 短 / 长（不支持自定义秒数） |
| `sound` | `TOAST_SOUND_DEFAULT` | `TOAST_SOUND_*` 系统提示音 |
| `audio_option` | `TOAST_AUDIO_DEFAULT` | 播放一次 / 静音 / 循环 |
| `actions` | `[]` | 按钮文本，最多 5 个 |
| `replace_previous` | `false` | 先隐藏当前显示和排队的通知 |

### 信号

- `toast_shown(toast_id)`：通知已交给 Windows
- `toast_activated(toast_id)`：用户点击通知本体
- `toast_action(toast_id, action_index)`：用户点击某个操作按钮
- `toast_failed(toast_id)`：Windows 报告通知无法显示

## 重要限制

### 导出后 `res://` 路径不可用

toast 的图片由 Windows 原生代码读取，**必须是真实存在的绝对文件路径**。

- 编辑器里 `ProjectSettings.globalize_path("res://icon.png")` 能用，因为此时
  `res://` 就是磁盘目录
- 导出后 `res://` 被打包进 PCK，`globalize_path()` 返回的路径并不对应真实文件，
  图片会加载失败。**这个问题在未导出的项目里发现不了**
- 解决办法：见 `globalize_path()` 方法的注释或转移到 `user://`

```gdscript
const TOAST_ICON := "user://toast_icon.png"

func _toast_icon_path() -> String:
	if not FileAccess.file_exists(TOAST_ICON):
		var file := FileAccess.open(TOAST_ICON, FileAccess.WRITE)
		file.store_buffer(FileAccess.get_file_as_bytes("res://icon.png"))
	return ProjectSettings.globalize_path(TOAST_ICON)

options.icon_path = _toast_icon_path()
```

图片格式支持 PNG / JPG / GIF，**不完全支持 SVG（图片尺寸错误）**。
路径请使用正斜杠（`ProjectSettings.globalize_path()` 返回的就是正斜杠），WinToast 会把它拼成 `file:///...` URI。

### 对开始菜单造成的副作用

未打包的桌面程序需要开始菜单中带对应 AUMID 的快捷方式，通知才会弹出横幅。
`initialize(..., create_shortcut = true)` 会让 WinToast 自动创建或更新这个快捷方式。

如果传入 `false` 且系统里没有对应 AUMID 的快捷方式，Windows 可能接收通知
（它会出现在通知中心）但不会显示横幅。

另外 `initialize()` 每个进程只生效一次；在同一个 Godot 编辑器进程里修改参数后，
需要重启编辑器才会应用新的 AUMID。

### 音频

只支持 `TOAST_SOUND_*` 系统提示音。自定义音频文件存在路径限制，因此没有提供文件路径参数。

### 其他

- **支持 Windows10 以上的系统，Windows10 以下部分功能将不可用**
- 操作按钮最多 5 个，超出部分会被忽略
- `initialize()` 会修改进程级状态，多个 `WinToaster` 实例共用同一个 WinToast 单例

## 文档翻译（可选）

GDExtension 的类文档只有英文，引擎没有加载第二份文档 XML 的选项。
`project/addons/doc_translation_win_toaster/` 是一个编辑器插件，它把
`translations/*.po` 注入到 `godot.documentation` 翻译域，从而让类参考显示中文。
插件已在 `project.godot` 中启用。

修改 `doc_classes/*.xml` 后重新生成翻译表：

```shell
cd godot-win-toaster
"../Godot v4.7.2/Godot_v4.7.2-stable_win64_console.exe" --headless \
    --path project --script res://tools/extract_doc_msgids.gd
```

生成后会保留已有翻译，新增条目留空，失效条目转入 `#~` obsolete 区。
文档语言跟随编辑器语言（当前为 `zh_CN`）。

## 目录结构

```
src/                       WinToaster / ToastOptions / 注册代码
thirdparty/WinToast/       第三方库 WinToast 1.3.2
doc_classes/               类参考 XML
project/                   演示项目
  addons/doc_translation_win_toaster/   文档翻译插件与 PO 表
  tools/                   文档 msgid 提取工具
SConstruct                 SCons 构建脚本
```

## 许可

本项目使用 MIT 许可证，见 [LICENSE](LICENSE)。
WinToast 同样使用 MIT 许可证，见
[thirdparty/WinToast/LICENSE.txt](thirdparty/WinToast/LICENSE.txt)。
