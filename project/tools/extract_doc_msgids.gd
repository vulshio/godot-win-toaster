extends SceneTree

# Regenerates the class-reference translation table from doc_classes/Table.xml.
#
#   cd godot-cpp-template
#   "../Godot v4.7.2/Godot_v4.7.2-stable_win64_console.exe" --headless \
#       --path project --script res://tools/extract_doc_msgids.gd
#
# The table is PO, the same format the engine's own class reference uses. The
# engine loads it directly (core/io/translation_loader_po.cpp) - there is no
# import step - and the plugin injects it into the "godot.documentation" domain.
# File layout, escaping and the mandatory header live in tools/doc_po.gd.
#
# Existing translations are kept, new msgids are added empty, and msgids that no
# longer appear in the XML are moved to the `#~` obsolete section at the end of
# the file (their translations are kept - a reworded sentence should not silently
# throw them away, and `#~` is the gettext place to park them).
#
# This runs inside Godot rather than as a Python script on purpose. The key the
# engine looks up is `text.dedent().strip_edges()` - that is literally what `DTR()`
# does to the XML before hitting the translation domain (core/string/ustring.cpp).
# Calling `dedent()` is far safer than reimplementing its indentation rules, and a
# msgid that differs by one space simply never matches and falls back to English
# with no error anywhere.

const DocPO := preload("res://tools/doc_po.gd")

const XML_PATH := "res://../doc_classes/Table.xml"
const TABLE_BASE := "res://addons/table_gdextension/translations/table_docs."
const LOCALE := "zh_CN"


func _initialize() -> void:
	var msgids := DocPO.collect_msgids(XML_PATH)
	if msgids.is_empty():
		push_error("no text nodes found in %s" % XML_PATH)
		quit(1)
		return

	var table_path := TABLE_BASE + LOCALE + ".po"
	var existing := DocPO.read_po(table_path)

	# Live and obsolete are both sources of a translation, so an entry that comes
	# back into the XML is revived with the translation it had before.
	var known: Dictionary = existing["live"]
	for key in existing["obsolete"]:
		if not known.has(key):
			known[key] = existing["obsolete"][key]

	var live := {}
	var missing := 0
	for id in msgids:
		var text := String(known.get(id, ""))
		if text.is_empty():
			missing += 1
		live[id] = text

	# Only orphans that carry a translation are kept - a msgid that was never
	# translated loses nothing by being dropped, and keeping those would let the
	# file fill up with dead entries every time a sentence is reworded.
	var obsolete := {}
	for key in existing["obsolete"]:
		if not msgids.has(key) and not String(existing["obsolete"][key]).is_empty():
			obsolete[key] = existing["obsolete"][key]
	for key in existing["live"]:
		if not msgids.has(key) and not String(existing["live"][key]).is_empty():
			obsolete[key] = existing["live"][key]

	DocPO.write_po(table_path, LOCALE, msgids, live, obsolete)

	print("doc msgids: %d total, %d translated, %d missing (%s)" % [
			msgids.size(), msgids.size() - missing, missing, LOCALE])
	if not obsolete.is_empty():
		print("  %d orphaned entr(ies) kept as #~ obsolete, no longer in the XML:" % obsolete.size())
		for key in obsolete:
			print("    ", key.split("\n")[0])
	if missing == 0 and obsolete.is_empty():
		print("  table is up to date")

	quit(0)
