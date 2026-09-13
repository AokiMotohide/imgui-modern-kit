"""Inventory this independent module without rewriting existing API artifacts."""
import json
import pathlib
import re

root = pathlib.Path(__file__).resolve().parents[1]
header = (root / "include/imkit/node_editor.h").read_text(encoding="utf-8")
records = []
for match in re.finditer(r"^([A-Za-z][\w:<>, &]*?)\s+(\w+)\(([^;]*?)\);", header, re.MULTILINE):
    records.append({"name": "imkit::node_editor::" + match[2],
                    "declaration": " ".join(match[0].split()),
                    "header": "imkit/node_editor.h"})
(root / "docs/node-editor-api.json").write_text(
    json.dumps(records, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(f"{len(records)} node editor functions inventoried")
