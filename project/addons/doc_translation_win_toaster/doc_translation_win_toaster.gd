@tool
extends EditorPlugin

## GDExtension class reference docs are always English - there is no way to
## ship a second doc XML. The editor's class reference translates every text
## node through the "godot.documentation" translation domain, though, so this
## plugin loads the PO tables next to it and injects them there.
##
## Regenerate the table after changing doc_classes/*.xml:
##   cd godot-win-toaster
##   "../Godot v4.7.2/Godot_v4.7.2-stable_win64_console.exe" --headless \
##       --path project --script res://tools/extract_doc_msgids.gd

const TRANSLATIONS_DIR := "res://addons/doc_translation_win_toaster/translations"
const DOC_DOMAIN := &"godot.documentation"

var _domain: TranslationDomain = null
var _translations: Array[Translation] = []


func _enter_tree() -> void:
	if not TranslationServer.has_method("get_or_add_domain"):
		push_warning("doc_translation_win_toaster: TranslationServer.get_or_add_domain() is missing (Godot 4.5+ required).")
		return

	_domain = TranslationServer.get_or_add_domain(DOC_DOMAIN)
	for file_name in DirAccess.get_files_at(TRANSLATIONS_DIR):
		if not file_name.ends_with(".po"):
			continue

		var path := TRANSLATIONS_DIR.path_join(file_name)
		var translation := ResourceLoader.load(path, "Translation", ResourceLoader.CACHE_MODE_IGNORE) as Translation
		if translation == null:
			push_warning("doc_translation_win_toaster: could not load %s" % path)
			continue

		_domain.add_translation(translation)
		_translations.append(translation)

	if not _translations.is_empty():
		print("doc_translation_win_toaster: injected %d translation(s) into %s." % [
				_translations.size(), DOC_DOMAIN])


func _exit_tree() -> void:
	if _domain == null:
		return

	for translation in _translations:
		_domain.remove_translation(translation)
	_translations.clear()
	_domain = null
