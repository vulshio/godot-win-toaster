extends RefCounted

# Shared PO reading and writing for the Table class-reference translations.
#
# The engine's own loader (`core/io/translation_loader_po.cpp`) is the contract:
# it undoes escapes with `String.c_unescape()`, and the mandatory header entry is
# what makes its `config` non-empty - without it loading fails with "No config
# found in file".
#
# Writing does not use `c_escape()` because that also escapes `'`, which gettext
# tools do not expect in a double-quoted string; `_escape()` emits only the
# escapes PO defines. The opening entry is the only thing the loader requires
# beyond that.
#
# Two entry kinds live in the file: live ones the engine loads, and `#~`
# obsolete ones it ignores (a `#`-prefixed line is a comment). Obsolete entries
# are parked at the end of the file so that the word "fuzzy" - which is
# special-cased by the loader for *any* comment line - can never appear before
# a live entry and silently swallow it.


const HEADER_LINES := [
	"Project-Id-Version: Table GDExtension",
	"MIME-Version: 1.0",
	"Content-Type: text/plain; charset=UTF-8",
	"Content-Transfer-Encoding: 8bit",
]


# Every non-empty text node in the doc XML, dedented and stripped the way `DTR()`
# will do it. The engine's own doc parser reads these same nodes through
# `get_node_data()`, so collecting all of them cannot miss a translatable string -
# and an over-collected entry is just an unused one.
static func collect_msgids(xml_path: String) -> Array[String]:
	var msgids: Array[String] = []
	var parser := XMLParser.new()
	if parser.open(xml_path) != OK:
		push_error("cannot open %s" % xml_path)
		return msgids

	while parser.read() == OK:
		if parser.get_node_type() != XMLParser.NODE_TEXT:
			continue
		var text := parser.get_node_data().dedent().strip_edges()
		if not text.is_empty() and not msgids.has(text):
			msgids.append(text)
	return msgids


# Returns `{ "live": {msgid: msgstr}, "obsolete": {msgid: msgstr} }`.
#
# Live entries come from the engine's own loader, so what comes back is exactly
# what the translation domain would resolve. Obsolete entries are invisible to
# it - `#~` lines are comments - so they are scanned out of the raw text instead.
# Both dictionaries keep file order, which is what lets a rewrite stay stable.
# A missing or unreadable file is not an error: it just yields empty tables.
static func read_po(path: String) -> Dictionary:
	var out := {"live": {}, "obsolete": {}}
	if not FileAccess.file_exists(path):
		return out

	var loaded = ResourceLoader.load(path, "Translation", ResourceLoader.CACHE_MODE_IGNORE)
	if loaded is Translation:
		var translation: Translation = loaded
		for msgid in translation.get_message_list():
			var text := String(translation.get_message(msgid))
			if not text.is_empty():
				out["live"][msgid] = text
	else:
		push_warning("doc_po: %s did not load as a Translation" % path)

	var f := FileAccess.open(path, FileAccess.READ)
	if f == null:
		push_warning("doc_po: cannot read %s" % path)
		return out
	out["obsolete"] = _read_obsolete(f.get_as_text())
	return out


# `ordered_msgids` fixes the live order (the XML's order); obsolete entries are
# written after them, in the order the dictionary holds them.
static func write_po(path: String, locale: String, ordered_msgids: Array[String],
		live_table: Dictionary, obsolete_table: Dictionary) -> void:
	var lines: Array[String] = []
	lines.append("msgid \"\"")
	lines.append("msgstr \"\"")
	for header in HEADER_LINES:
		lines.append("\"%s\\n\"" % _escape(header))
	lines.append("\"Language: %s\\n\"" % _escape(locale))
	lines.append("")

	for msgid in ordered_msgids:
		_append_entry(lines, msgid, String(live_table.get(msgid, "")), false)

	if not obsolete_table.is_empty():
		lines.append("# Obsolete entries, kept so a reworded sentence does not lose its translation.")
		lines.append("# The engine ignores what follows: every `#~` line is a comment.")
		lines.append("")
		for msgid in obsolete_table:
			_append_entry(lines, msgid, String(obsolete_table[msgid]), true)

	var f := FileAccess.open(path, FileAccess.WRITE)
	if f == null:
		push_error("doc_po: cannot write %s" % path)
		return
	f.store_string("\n".join(PackedStringArray(lines)) + "\n")


static func _append_entry(lines: Array[String], msgid: String, msgstr: String, obsolete: bool) -> void:
	var prefix := "#~ " if obsolete else ""
	_append_value(lines, msgid, "msgid", prefix)
	_append_value(lines, msgstr, "msgstr", prefix)
	lines.append("")


# A value with no newline goes on one line. Otherwise it is split on the *raw*
# newlines first and each segment escaped on its own, with a literal `\n` escape
# on every line but the last - the layout the engine's own doc translations use.
static func _append_value(lines: Array[String], value: String, keyword: String, prefix: String) -> void:
	var parts := value.split("\n")
	if parts.size() == 1:
		lines.append("%s%s \"%s\"" % [prefix, keyword, _escape(value)])
		return
	lines.append("%s%s \"\"" % [prefix, keyword])
	for i in parts.size():
		var newline := "\\n" if i < parts.size() - 1 else ""
		lines.append("%s\"%s%s\"" % [prefix, _escape(parts[i]), newline])


# Escapes only what PO defines: the backslash, the double quote, and the control
# characters. Deliberately not `c_escape()`, which would turn `'` into `\'` - the
# engine reads that back fine, but gettext tools expect a bare apostrophe.
#
# (A literal backslash followed by n/t/r/v would still come back wrong, because
# `c_unescape()` rewrites those replacements in that order. No doc string holds a
# backslash, and the engine's reader is the fixed contract, so this cannot be
# improved from here.)
static func _escape(text: String) -> String:
	return text \
			.replace("\\", "\\\\") \
			.replace("\a", "\\a") \
			.replace("\b", "\\b") \
			.replace("\f", "\\f") \
			.replace("\n", "\\n") \
			.replace("\r", "\\r") \
			.replace("\t", "\\t") \
			.replace("\v", "\\v") \
			.replace("\"", "\\\"")


# Reads the `#~ msgid` / `#~ msgstr` pairs (and their quoted continuation lines)
# that the engine skips. Escaping is undone with `c_unescape()`, mirroring the
# loader so a value round-trips exactly.
static func _read_obsolete(text: String) -> Dictionary:
	var result := {}
	var msgid := ""
	var msgstr := ""
	var status := 0 # 0 idle, 1 reading msgid, 2 reading msgstr

	for raw in text.split("\n"):
		var line := raw.strip_edges()
		if not line.begins_with("#~"):
			continue
		var body := line.substr(2).strip_edges()

		# Never emitted by this tool, but a translator's editor may add them.
		if body.begins_with("msgid_plural") or body.begins_with("msgctxt") or body.begins_with("msgstr["):
			continue

		if body.begins_with("msgid"):
			if not msgid.is_empty():
				result[msgid] = msgstr
			msgid = _unquote(body.substr(5))
			msgstr = ""
			status = 1
		elif body.begins_with("msgstr"):
			msgstr = _unquote(body.substr(6))
			status = 2
		elif body.begins_with("\""):
			if status == 1:
				msgid += _unquote(body)
			elif status == 2:
				msgstr += _unquote(body)

	if not msgid.is_empty():
		result[msgid] = msgstr
	return result


static func _unquote(line: String) -> String:
	var text := line.strip_edges()
	if text.length() >= 2 and text.begins_with("\"") and text.ends_with("\""):
		text = text.substr(1, text.length() - 2)
	return text.c_unescape()
