"""Run a Python file inside the Unreal editor via Web Remote Control. usage: python rc.py script.py"""
import json, sys, urllib.request
src = open(sys.argv[1], encoding="utf-8").read()
body = {"objectPath": "/Script/PythonScriptPlugin.Default__PythonScriptLibrary", "functionName": "ExecutePythonCommandEx",
        "parameters": {"PythonCommand": src, "ExecutionMode": "ExecuteFile", "FileExecutionScope": "Public"}}
req = urllib.request.Request("http://127.0.0.1:30010/remote/object/call", json.dumps(body).encode(), method="PUT",
                             headers={"Content-Type": "application/json"})
r = json.loads(urllib.request.urlopen(req, timeout=1800).read().decode("utf-8", "replace"))
for l in r.get("LogOutput", []):
    if "DeprecationWarning" in l["Output"]: continue
    print(("" if l["Type"] == "Info" else f"[{l['Type']}] ") + l["Output"].rstrip())
print("OK" if r.get("ReturnValue") else "FAILED", r.get("CommandResult", ""))
