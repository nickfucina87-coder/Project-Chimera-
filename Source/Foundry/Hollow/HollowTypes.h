// Plain data for Hollowmere: terrain, hexes, event cards, foes, action cards, NPCs and the hero sheet.
// Content tables live in HollowData.cpp; rules math in HollowRules.cpp.
#pragma once

#include "CoreMinimal.h"

enum class ETerr : uint8 { Forest, Plains, Hills, Swamp, Ruins, River, Town, Keep, Shrine };

// Hidden: unknown. Rumored: someone told you about it, pay to scout it open. Open: travel there.
// Explored: you have been there. Cleared: its event is resolved.
enum class EHexState : uint8 { Hidden, Rumored, Open, Explored, Cleared };

struct FTerrInfo
{
	FString Name;
	FLinearColor Fill;
	FLinearColor Ground;
	float Weight = 0.f;
};
const FTerrInfo& TerrInfo(ETerr T);

struct FHollowHex
{
	int32 Q = 0, R = 0;
	ETerr T = ETerr::Plains;
	EHexState State = EHexState::Hidden;
	FString EventId;
	bool bEventDone = false;
	FString LeadName;          // label shown once a conversation reveals it
	TArray<bool> ChestTaken;   // small loot spots in the local area
	TArray<bool> FogCleared;   // fog-of-war cells already revealed in the local area
	bool IsOpen() const { return State >= EHexState::Open; }
	bool IsVisited() const { return State >= EHexState::Explored; }
};

/* ---------- Event cards ---------- */

struct FOutcome
{
	FString Text;
	int32 Xp = 0, Gold = 0, Rations = 0, Potions = 0, Holy = 0;
	FString Dmg, Combat, Heal;
	bool bSlots = false, bAmbush = false, bWeaken = false, bNoClear = false;

	FOutcome& xp(int32 V) { Xp = V; return *this; }
	FOutcome& gold(int32 V) { Gold = V; return *this; }
	FOutcome& rations(int32 V) { Rations = V; return *this; }
	FOutcome& potions(int32 V) { Potions = V; return *this; }
	FOutcome& holy(int32 V) { Holy = V; return *this; }
	FOutcome& dmg(const TCHAR* V) { Dmg = V; return *this; }
	FOutcome& combat(const TCHAR* V) { Combat = V; return *this; }
	FOutcome& heal(const TCHAR* V) { Heal = V; return *this; }
	FOutcome& slots() { bSlots = true; return *this; }
	FOutcome& ambush() { bAmbush = true; return *this; }
	FOutcome& weaken() { bWeaken = true; return *this; }
	FOutcome& noclear() { bNoClear = true; return *this; }
};

struct FChoice
{
	FString Label;
	TArray<FString> Skills;    // empty = no check, use Out
	int32 DC = 0;
	int32 Cost = 0;
	FOutcome Ok, Fail, Out, CritFail;
	bool bHasCritFail = false;
	bool IsCheck() const { return Skills.Num() > 0; }
};

struct FEventDef
{
	FString Id;
	TArray<ETerr> Terr;
	FString Title, Text;
	TArray<FChoice> Choices;
};

/* ---------- Foes ---------- */

struct FFoeDef
{
	FString Id, Name;
	int32 Hp = 10, Ac = 15, Per = 5, Fort = 5, Ref = 5, Will = 5, Atk = 5;
	FString Dmg, Weapon;
	int32 Xp = 10, Gold = 0, Speed = 25;
	bool bUndead = false, bBoss = false;
	int32 Regen = 0;
	FString Special;   // AI flavour: knockdown, dirty, paralyze, grab, lord
};

/* ---------- Action cards ---------- */

enum class ECardFx : uint8
{
	PowerAttack, DoubleSlice, SuddenCharge, Intimidate, Feint, Tumble, Hamstring,
	ForceBarrage, ElectricArc, Fear, ShieldSpell, Heal, DivineLance, Bless,
	Quicken, LuckyCoin, SecondWind, Insight, BattleHymn, Riposte, HolyFlame
};

struct FCardDef
{
	FString Id, Name, Text, Kind;   // Kind: Attack, Skill, Spell, Tempo, Aura
	int32 Cost = 1;
	ECardFx Fx = ECardFx::PowerAttack;
	FLinearColor Col;
	bool bNeedsTarget = true;
};

/* ---------- NPCs and talk ---------- */

struct FTopicDef
{
	FString Id, Label;
	TArray<FString> Keywords;
};

struct FNpcReply
{
	FString Text;      // [topic] in brackets reveals that topic as a chip
	FString Lead;      // reveals a route on the map toward this lead
	int32 Gold = 0, Rations = 0, Holy = 0;
};

struct FNpcDef
{
	FString Id, Name, Role, Greeting, Unknown, Service;
	TMap<FString, FNpcReply> Replies;
	FLinearColor Body, Skin, Hat;
	int32 HatStyle = 0;
};

struct FLeadDef
{
	FString Id, Name;
	TArray<ETerr> Prefer;
	int32 Dir = 0;        // preferred compass sextant 0..5 (axial DIRS order)
	int32 Ring = 3;
};

/* ---------- Hero ---------- */

struct FPC
{
	FString Name, Ancestry, Kit, ClassName;
	int32 Level = 1, Xp = 0;
	TMap<FString, int32> Abil, Ranks, Saves;
	int32 PerRank = 1;
	FString ArmorName;
	int32 AcItem = 10, ArmorRank = 1;
	FString WeaponName, WeaponAbil;
	int32 WeaponDie = 6, WeaponRank = 1;
	bool bCaster = false;
	FString SpellAbil;
	int32 SpellRank = 1;
	int32 HpPer = 8, HpMax = 10, Hp = 10;
	int32 Gold = 20, Rations = 6, Potions = 1, Holy = 0;
	int32 Speed = 25;
	TArray<FString> Deck;
	FLinearColor Body, Skin, Hat;
	int32 HatStyle = 0;
};

/* ---------- Content access ---------- */

namespace HollowData
{
	const TArray<FEventDef>& Events();
	const FEventDef* FindEvent(const FString& Id);
	const TMap<FString, FFoeDef>& Foes();
	const TMap<FString, FCardDef>& Cards();
	const TArray<FString>& RewardPool(const FString& Kit);
	TArray<FString> StarterDeck(const FString& Kit);
	const TArray<FTopicDef>& Topics();
	const FTopicDef* FindTopic(const FString& Id);
	const TArray<FNpcDef>& Npcs();
	const FNpcDef* FindNpc(const FString& Id);
	const TArray<FLeadDef>& Leads();
	const FLeadDef* FindLead(const FString& Id);
	const TMap<FString, TPair<FString, FString>>& Lore();
}
