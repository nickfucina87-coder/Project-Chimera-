"""Minimal MCP-over-HTTP client for the Unreal editor.
usage: python ue.py <toolset|-> <tool> [json-args]   ('-' toolset = top-level meta tool)"""
import json, sys, urllib.request

URL = "http://127.0.0.1:8000/mcp"
SESSION = None

def rpc(method, params=None, rid=1, notify=False):
    global SESSION
    body = {"jsonrpc": "2.0", "method": method}
    if params is not None: body["params"] = params
    if not notify: body["id"] = rid
    req = urllib.request.Request(URL, json.dumps(body).encode(), method="POST",
                                 headers={"Content-Type": "application/json", "Accept": "application/json, text/event-stream",
                                          **({"Mcp-Session-Id": SESSION} if SESSION else {})})
    with urllib.request.urlopen(req, timeout=600) as r:
        SESSION = r.headers.get("Mcp-Session-Id") or SESSION
        raw = r.read().decode("utf-8", "replace")
    if notify or not raw.strip(): return None
    if raw.lstrip().startswith("data:") or "\ndata:" in raw:
        raw = [l[5:].strip() for l in raw.splitlines() if l.startswith("data:")][-1]
    return json.loads(raw)

def init():
    rpc("initialize", {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "seph-cli", "version": "1"}})
    rpc("notifications/initialized", notify=True)

def call(toolset, tool, args=None):
    if toolset in ("-", None):
        p = {"name": tool, "arguments": args or {}}
    else:
        p = {"name": "call_tool", "arguments": {"toolset_name": toolset, "tool_name": tool, "arguments": args or {}}}
    res = rpc("tools/call", p, rid=2)
    if "error" in res: return json.dumps(res["error"], indent=1)
    out = []
    for c in res["result"].get("content", []):
        out.append(c.get("text", json.dumps(c)))
    if res["result"].get("isError"): out.insert(0, "[ISERROR]")
    return "\n".join(out)

if __name__ == "__main__":
    init()
    if sys.argv[1] == "--list-tools":
        print(json.dumps(rpc("tools/list", {}, rid=3), indent=1)[:4000]); sys.exit()
    args = json.loads(sys.argv[3]) if len(sys.argv) > 3 else {}
    if len(sys.argv) > 3 and sys.argv[3].startswith("@"):
        args = json.load(open(sys.argv[3][1:], encoding="utf-8"))
    print(call(sys.argv[1], sys.argv[2], args))
