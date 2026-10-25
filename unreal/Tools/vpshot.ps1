# usage: vpshot.ps1 name x y z tx ty tz   -> shots\vp_<name>.png (real editor viewport, real exposure)
param($name, $x, $y, $z, $tx, $ty, $tz)
$sp = "C:\Users\nickf\Documents\Unreal Projects\Chimera\Saved\SephScripts\tools"
$dir = "C:\Users\nickf\Documents\Unreal Projects\Chimera\Saved\Screenshots\WindowsEditor"
"{`"pos`":[$x,$y,$z],`"tgt`":[$tx,$ty,$tz]}" | Set-Content "$sp\vp.json" -Encoding ascii
$env:PYTHONIOENCODING = "utf-8"
python -I "$sp\rc.py" "$sp\vp.py" | Out-Null
Start-Sleep -Seconds 4
$before = if (Test-Path $dir) { (Get-ChildItem $dir -Filter *.png | Measure-Object).Count } else { 0 }
"import unreal`nunreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(), 'HighResShot 1280x720')" | Set-Content "$sp\vp_cmd.py" -Encoding ascii
python -I "$sp\rc.py" "$sp\vp_cmd.py" | Out-Null
for ($i = 0; $i -lt 30; $i++) { Start-Sleep -Milliseconds 500; if ((Test-Path $dir) -and (Get-ChildItem $dir -Filter *.png | Measure-Object).Count -gt $before) { break } }
Start-Sleep -Milliseconds 700
$f = Get-ChildItem $dir -Filter *.png | Sort-Object LastWriteTime | Select-Object -Last 1
Copy-Item $f.FullName "$sp\shots\vp_$name.png" -Force; "vp_$name.png"
