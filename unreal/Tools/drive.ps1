param($json)
$sp = "C:\Users\nickf\AppData\Local\Temp\claude\C--Users-nickf\0df46d9a-2366-4026-9062-dc693361cd0b\scratchpad"
$json | Set-Content "$sp\drive_cmd.json" -Encoding ascii
$env:PYTHONIOENCODING = "utf-8"
python -I -X utf8 "$sp\rc.py" "$sp\pie_drive.py" | Where-Object { $_ -ne "OK None" }
