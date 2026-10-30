# Handshake — when both are ready

**Project:** Azeroth Reborn (Mike owns it)  
**People:** Mike and Nick  
**Date:** 2026-10-30  
**Implementation repo:** https://github.com/Doomious/Azrothreborn  
**How to build it:** [STEAM_INVITE_SETUP.md](STEAM_INVITE_SETUP.md)

This file is the agreement for the co-op test. Nick is asking for the infrastructure as aid. It does not change the island, the art, or Mike’s lanes.

## The handshake is the Steam invite

1. Both people are in the Azrothreborn build, Steam is running, and the two accounts are friends.
2. One person hosts: creates a Steam session and is in the World Partition crater as a listen server.
3. The host invites the other person from Steam (overlay → friend → Invite to Game, or the project’s invite button, which sends that same Steam invite).
4. The other person accepts the invite in the Steam overlay.
5. Their game joins that session and travels to the host.

That accept is the handshake. The connection is Steam’s, via Steam Sockets. Nobody types an address.

## Optional Discord: “AR ready”

Either person may post **AR ready** in Discord when their build is up, Steam is logged in, and they can send or accept an invite.

That message means “start the Steam invite.” It does not join the game, carry an IP, or replace the invite. If Discord is quiet, the Steam invite still is the handshake.

## Definition of done

All of these, on two machines (or one editor and one packaged Development build) and two Steam accounts:

- The second person entered by **accepting the Steam friend invite**.
- Both are on `/Game/Maps/StarfallCrater_WP`.
- Each person **sees the other person’s pawn**.
- **Both can move**, and the other machine sees that movement.

## Not done yet

- The second person got in by a LAN session, a typed IP, `open <ip>:7777`, or a forwarded port.
- The invite was accepted, then the client created its own session and became a second host.
- Pawns replicate but stay invisible (first-person arms are owner-only until a mesh for everyone else exists).
- Only one side can move.
- The only “multiplayer” tried was editor Play-In-Editor with extra local players. That path does not use a Steam invite.

Phase 2 (first-person / third-person mouse wheel, clan or team PlayerStarts) is not part of this handshake. Do not wait on it.

## Where the work goes

Agents implement [STEAM_INVITE_SETUP.md](STEAM_INVITE_SETUP.md) in Azrothreborn and open the pull request there. Project-Chimera- stays a written brief.
