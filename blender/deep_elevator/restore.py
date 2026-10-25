"""Rebuild the saved Blender scene from the four repository parts."""
import hashlib
import json
from pathlib import Path

folder = Path(__file__).resolve().parent
manifest = json.loads((folder / "checksums.json").read_text(encoding="utf-8"))
output = folder / manifest["file"]
if output.exists():
    raise SystemExit(f"Output already exists: {output}")

try:
    whole = hashlib.sha256()
    with output.open("xb") as dest:
        for part in manifest["parts"]:
            path = folder / part["name"]
            if path.stat().st_size != part["size"]:
                raise ValueError(f"Wrong size: {path.name}")
            digest = hashlib.sha256()
            with path.open("rb") as source:
                while block := source.read(1024 * 1024):
                    digest.update(block)
                    whole.update(block)
                    dest.write(block)
            if digest.hexdigest() != part["sha256"]:
                raise ValueError(f"Checksum mismatch: {path.name}")
    if output.stat().st_size != manifest["size"] or whole.hexdigest() != manifest["sha256"]:
        raise ValueError("Rebuilt scene failed size or checksum verification")
except Exception:
    output.unlink(missing_ok=True)
    raise
print(f"Restored and verified: {output}")
