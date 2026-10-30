# Steam friend invite — implementation brief

**For:** Mike’s agents in [Azrothreborn](https://github.com/Doomious/Azrothreborn)  
**From:** Nick, as aid, so he and Mike can co-op test  
**Date:** 2026-10-30  
**Owner:** Mike. This request does not retarget Azeroth Reborn.  
**Done when:** [HANDSHAKE.md](HANDSHAKE.md) passes.

Project-Chimera- holds this brief only. Do not add Blueprints, plugins, ini, or a game module here. Every file listed under [What changes in Azrothreborn](#what-changes-in-azrothreborn) is a change in Mike’s repo.

## Hard rules

1. **Mike owns the project.** Add the smallest session and replication setup that lets two people meet in the crater. Do not redesign the world, the regions, or the First Person handling beyond what the definition of done requires.
2. **The path is Steam.** Host creates a Steam session (listen server), then a Steam friend invite. The client accepts that invite and joins. Steam Sockets carries the connection (NAT traversal and Valve’s relay).
3. **App ID 480 is temporary.** `480` is Spacewar, so two normal Steam accounts can test. Replace it with the real App ID before any shareable or shipped build. Do not ship 480.
4. **Phase 2 does not block.** First-person ↔ third-person on the mouse wheel, and clan/team PlayerStarts, wait until the handshake works.
5. **Leave licensed packs out of git.** The First Person template and the Fab packs stay git-ignored.

## Outside this test

Do not make any of these the way Nick and Mike connect:

- A LAN session (`Use LAN` / `bIsLANMatch`).
- A typed LAN or WAN address.
- Console `open <ip>:7777`.
- Port-forwarding or opening 7777.
- `DefaultPlatformService=NULL`.
- Editor Play-In-Editor with extra local players, reported as a pass.

`IpNetDriver` may appear only as `DriverClassNameFallback` if the SteamSockets module fails to load. If the log shows that IP driver actually carrying the connection, the setup is wrong. Fix Steam. Do not switch the test plan to a direct IP.

## Project facts

Verified on Azrothreborn `main` (2026-10-30):

| Fact | Value |
|------|--------|
| Engine | Unreal Engine **5.8** (`AzrothReborn.uproject`, `EngineAssociation` 5.8) |
| Project kind | Blueprint. No `Source/` game module yet. |
| Default map | `/Game/Maps/StarfallCrater_WP` (World Partition). `GameDefaultMap` and `EditorStartupMap` in `Config/DefaultEngine.ini`. |
| Playable start | Breach Gate, first person, as the Azrothreborn README already describes. |
| Fallback map | `/Game/Maps/StarfallCrater` (non-streaming). Keep it as a fallback. The co-op test stays on the WP map. |
| Game mode | `/Game/FirstPerson/Blueprints/BP_FirstPersonGameMode` (`GlobalDefaultGameMode`) |
| Player content | Epic First Person template at `/Game/FirstPerson`, `/Game/Characters`, `/Game/Input`. **Git-ignored.** Each person installs it locally: **Add → Add Feature or Content Pack → Blueprint → First Person**, at those paths. |
| Content git policy | `.gitignore` allowlists project folders under `Content/`. Everything else under `Content/` stays local. |
| Online plugins | Not enabled. Plugins today: ModelingToolsEditorMode, ModelContextProtocol, AllToolsets, ProceduralVegetationEditor. |
| Net driver | Engine default. No Steam net driver yet. |

Streaming on the WP map: props, foliage, ruins, town plots, and FX stream in within 256 m; terrain, water, sky, and lighting stay loaded. Two pawns have to remain inside that setup. See [World Partition](#world-partition).

## Plugins

Enable these **engine** plugins and save them into `AzrothReborn.uproject` so the next clone gets them:

- Online Subsystem
- Online Subsystem Steam
- Steam Sockets

```json
{ "Name": "OnlineSubsystem", "Enabled": true },
{ "Name": "OnlineSubsystemSteam", "Enabled": true },
{ "Name": "SteamSockets", "Enabled": true }
```

Steam Sockets is the net driver on purpose. On UE 5.6 and later, listen-server joins fail when the driver is the legacy `OnlineSubsystemSteam.SteamNetDriver`. Steam Sockets is also the relay path, which is why this test does not open a port.

**Advanced Sessions is appropriate** for this project. The game is Blueprint, and Epic’s default GameInstance does not surface Steam’s “invite accepted” event to Blueprint. Advanced Sessions’ `AdvancedFriendsGameInstance` does (`OnSessionInviteAccepted`), which is the client half of the handshake.

- Use a build that compiles on **5.8**. Install it as a project plugin at `Plugins/AdvancedSessions` (the plugin’s own `.uplugin` name is the uproject entry, usually `AdvancedSessions`).
- The first load compiles the plugin and turns the project into a code project. That is expected. Windows agents need the UE 5.8 C++ toolchain (Visual Studio 2022, game development workload) for that compile. Gameplay stays Blueprint.
- Read the plugin license before committing it. If the license allows this repo to carry the source, commit `Plugins/AdvancedSessions`. If it does not, git-ignore that folder and record the exact version and download URL in Azrothreborn `Docs/` so both people install the same build.
- Do not copy the plugin into Project-Chimera-.

If Advanced Sessions does not compile on 5.8, keep SteamSockets and do not fall through to LAN or a raw IP. Create and join with the engine’s Online Subsystem session nodes, and add the smallest C++ game subsystem that binds `IOnlineSession::OnSessionUserInviteAccepted` and calls `JoinSession` with the invite result.

## Config

Add this to Azrothreborn `Config/DefaultEngine.ini`. Clear the existing net-driver array first so the stock IP `GameNetDriver` is not left in front of SteamSockets. Use plain ASCII quotes.

```ini
[/Script/Engine.GameEngine]
!NetDriverDefinitions=ClearArray
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

[OnlineSubsystem]
DefaultPlatformService=Steam

[OnlineSubsystemSteam]
bEnabled=true
SteamDevAppId=480
bUseSteamNetworking=true
bInitServerOnClient=true
```

Check the property names against the 5.8 `OnlineSubsystemSteam` and `SteamSockets` plugins in the installed engine (unknown-property warnings on load). The driver class stays `/Script/SteamSockets.SteamSocketsNetDriver`. If that engine’s SteamSockets notes say `bUseSteamNetworking` must be false once SteamSockets is the driver, follow that note. Do not replace the block with a LAN or `open ip:7777` setup.

`SteamDevAppId=480` is the committed test id. When a real App ID exists, change this value and delete local `steam_appid.txt` files. One place, then stop using 480.

Local file, **git-ignored**, one line `480`:

- Project root, for editor launches.
- Next to the `.exe` in `Binaries/Win64/`, for a packaged Development build.

The game will show up as **Spacewar** while the id is 480. That is expected for this test.

Both testers: Steam running first, two different accounts, those accounts friends, overlay enabled (no `-nooverlay`). Spacewar is available to every Steam account.

## Session flow

Listen server. One person hosts. The other joins. No dedicated server.

Put the flow on a GameInstance so it survives travel. Parent it on `AdvancedFriendsGameInstance`. Suggested name: `BP_ARGameInstance`. Assign it as the project Game Instance (Project Settings → Maps & Modes, or `GameInstanceClass` in `DefaultEngine.ini`).

**Do not create the session from the crater map’s BeginPlay.** A known failure: the client accepts the invite, travels, and BeginPlay creates a new session, so the client becomes a second host. Create only from an explicit Host action. Accept-invite only joins.

### Host

1. If a `GameSession` already exists, destroy it first so a second click can create again.
2. **Create Advanced Session** (Advanced Sessions node), with the local player controller:
   - Public connections: at least 2 (4 is fine)
   - Use LAN: **false**
   - Allow invites: **true**
   - Use presence: **true**
   - Allow join via presence: **true**
   - Use lobbies if available: **true**
   - Allow join in progress: **true**
   - Should advertise: **true**
3. On success, **Open Level** `/Game/Maps/StarfallCrater_WP` with Options `listen`.

   That options string is listen-server travel of the WP map. It is not an address. In an Open Level node the value is `listen` (the engine adds the `?`). Do not append an IP.

4. Once the host is in that listen session, send the Steam friend invite.
   - The handshake action is the Steam overlay: Shift+Tab → the friend → **Invite to Game**. Presence and lobbies are what make that overlay entry real.
   - A widget button may also call Advanced Sessions’ send-session-invite on that friend’s id. It must raise the same Steam invite. Shipping the button does not replace the overlay path; the overlay path has to work.

### Client

1. Accept the invite in the Steam overlay.
2. `OnSessionInviteAccepted` on `BP_ARGameInstance` runs and provides the session to join.
3. **Join Session** with the local player controller and that session result.
4. Do not call Create Session on this path.
5. On success, Join Session travels the client to the host. The connect string comes from the Steam session (SteamSockets). Do not also Open Level on the client, and do not run `open <ip>`.

Bind the accept event on the GameInstance so it works from a menu and from inside a level.

If a pin name on Create Advanced Session differs in the 5.8 plugin build, match by meaning: Steam lobby, presence on, invites allowed, not LAN. Host and client need the same session settings and the same App ID.

## Replication

The definition of done is that each person sees the other pawn and both can move. `BP_FirstPersonGameMode` stays the mode. The mode itself stays server-only; do not try to replicate it. The listen server possesses each pawn the usual way.

The First Person assets are git-ignored, so a flag flipped on one machine never reaches the other. Do both of the following.

### 1. Shared assets in git

- Create `Content/Multiplayer/`.
- Subclass the First Person character and `BP_FirstPersonGameMode` into that folder.
- Add a `.gitignore` allowlist exception in the same style as the existing ones, and nothing broader:

  ```gitignore
  !Content/Multiplayer/
  ```

- Point `GlobalDefaultGameMode` at the committed game-mode subclass.
- On that subclass, set Default Pawn to the committed character subclass.
- Turn the replication flags on **on the subclass**, so they are in the repo even though the Epic parent is not.
- Do not commit `/Game/FirstPerson`, `/Game/Characters`, `/Game/Input`, or any Fab pack.

Confirm the parent asset names locally after the First Person pack is installed. The game mode path above is the one already in `DefaultEngine.ini`. The character lives next to it under `/Game/FirstPerson/Blueprints/`; use the asset the game mode actually spawns.

### 2. Flags and a mesh other people can see

On the committed character subclass, Class Defaults:

- **Replicates** = true
- **Replicate Movement** = true

The template arms mesh is owner-only. With only that mesh, replication succeeds and the other player still sees nothing, which fails the handshake. Add a simple mesh on the subclass that remote players can see:

- Owner No See = true (the local player keeps the first-person arms)
- Only Owner See = false
- The arms mesh stays Only Owner See

A placeholder mesh is enough. A finished third-person body is Phase 2.

Character movement already replicates when Replicate Movement is on. Confirm the owning client still walks and looks, and the other machine sees the pawn move. Do not add a second movement-replication path.

The first-person camera stays local. Do not replicate the camera in a way that fights the owner’s view.

## World Partition

Test on `/Game/Maps/StarfallCrater_WP`. After the join, both pawns should be on loaded ground near the same start (Breach Gate / the map’s player start), not falling through an unloaded cell.

If the joining pawn falls through terrain, fix streaming inside this Steam session: the possessed pawn has to be a World Partition streaming source, and the listen server has to stream the cells that pawn needs. Check the 5.8 World Partition multiplayer settings before changing maps. Do not move the test to the non-streaming map, and do not “fix” a streaming miss by connecting over IP.

One shared player start is enough for two people. The engine’s spawn collision handling is enough if they overlap. Team spawns are Phase 2.

## Test widget

A small debug widget is enough. It is not the game’s front end.

- **Host** calls the GameInstance create-session flow.
- **Invite** (optional) sends the Steam invite to a chosen friend. The overlay invite still has to work without this button.
- Status text: Steam display name, whether the online subsystem is **Steam** (not NULL), session created, invite sent, join result.

If Steam fails to initialize, the engine silently uses the NULL subsystem and the session is not a Steam session. Show that on the widget. Fix Steam (client running, App ID 480, overlay on, two accounts) before changing Blueprint.

## How to test

Windows. Two computers, or one editor plus one packaged **Development** build. Two Steam accounts that are friends. One account cannot accept its own invite.

1. Steam is running on both sides before the game or editor launches. Overlay is on. `steam_appid.txt` is `480` when the launch is not a Steam-installed app.
2. Output Log shows the Steam online subsystem up, not NULL, and not `SteamAPI_Init failed`.
3. Host creates the session and lands in `StarfallCrater_WP` as a listen server (`LogNet` / session log shows listen, not a second standalone game).
4. Host invites the friend from the Steam overlay.
5. Client accepts. The client log shows join success and does **not** show a new Create Session.
6. Each person sees the other pawn. Both move, and the movement is visible on the other machine.
7. The connect path in the log is SteamSockets. Fail the run if the working path was `open <ip>:7777`, a LAN session, or a port forward.

If the overlay never appears inside the editor, package a Development build and retest before changing the session graph. A Shipping build is not required. Extra-player PIE does not count.

## Log

| Pass | Fail |
|------|------|
| Steam API init, subsystem is Steam | NULL subsystem, or `SteamAPI_Init failed` |
| Create session complete, then listen travel to the WP map | Create session with LAN enabled |
| Invite accepted → `JoinSession` true → client travel | `JoinSession` false, or the client creates a session after accept |
| Net traffic on `SteamSockets` | Traffic on `IpNetDriver` / a `:7777` address as the path that worked |
| Both pawns visible and moving | Replication on, mesh still owner-only, other pawn invisible |

## Phase 2 — do not block

Build these only after [HANDSHAKE.md](HANDSHAKE.md) passes:

- **Mouse wheel** swaps first person and third person (camera, and which mesh is visible to the owner).
- **Clan / team PlayerStarts**, so a team spawns together on its own start instead of the single shared start.

Neither one is required to see each other and move.

## What changes in Azrothreborn

| Path | Change |
|------|--------|
| `AzrothReborn.uproject` | Online Subsystem, Online Subsystem Steam, Steam Sockets. Advanced Sessions once installed. |
| `Config/DefaultEngine.ini` | SteamSockets net driver, `DefaultPlatformService=Steam`, `SteamDevAppId=480`. Game instance and default game mode if set here. |
| `Plugins/AdvancedSessions/` | Commit only if the license allows. Otherwise git-ignore and document the exact build. |
| `Content/Multiplayer/` | `BP_ARGameInstance`, debug widget, character subclass, game mode subclass. |
| `.gitignore` | `!Content/Multiplayer/` only. First Person pack stays ignored. `steam_appid.txt` stays ignored. |
| Azrothreborn `Docs/` | Optional pointer back at this brief and the plugin version, if the plugin is not committed. |

Do not commit `steam_appid.txt`. Do not change Project-Chimera- while doing this work. Open the implementation pull request on Azrothreborn.

## Agent checklist

- [ ] Engine plugins saved in `AzrothReborn.uproject`
- [ ] `DefaultEngine.ini` uses `SteamSocketsNetDriver` and App ID 480
- [ ] Session is a Steam lobby: presence, invites, lobbies, **not** LAN
- [ ] `BP_ARGameInstance` (`AdvancedFriendsGameInstance`) is the project Game Instance
- [ ] Host: Create Advanced Session, then Open Level `StarfallCrater_WP` with `listen`
- [ ] Client: `OnSessionInviteAccepted` → Join Session only
- [ ] Crater BeginPlay does not create a session
- [ ] Character subclass in `Content/Multiplayer/` has Replicates, Replicate Movement, and a mesh other players can see
- [ ] `.gitignore` allowlists `Content/Multiplayer/` and still ignores the First Person pack
- [ ] Two Steam accounts, overlay invite accepted, both pawns visible, both can move, on the WP map
- [ ] Log is Steam / SteamSockets, not NULL and not `open ip:7777`
- [ ] Phase 2 left undone unless the handshake already passes
