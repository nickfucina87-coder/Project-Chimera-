// Items, stacks, containers and the inventory that routes between them.
// Layout follows the split Nivalis Nights uses (studied via its IL2CPP metadata, Docs/InventoryDesign.md):
//   an item TYPE is static data looked up by id (FItemDef),
//   a STACK is a type id + count + per-instance data such as charges (FItemStack),
//   a CONTAINER is a fixed row of slots plus a restriction on what it accepts (FItemContainer),
//   the INVENTORY owns the containers and decides which one a new item goes to (FInventory).
// Tools carry "grants" (dig, light, fire, cut, climb, pick): carrying the tool gives the hero that field ability.
#pragma once

#include "CoreMinimal.h"

enum class EItemKind : uint8 { Tool, Consumable, Gear, Valuable, Quest };
enum class ERarity : uint8 { Common, Uncommon, Rare, Legendary, Unique };
// Same three outcomes as Nivalis' ItemCollectionOperationResult.
enum class EAddResult : uint8 { Failed, Partial, Succeeded };

struct FItemDef
{
	FString Id, Name, Desc, Icon;   // Icon: sprite under /Game/Hollow/FX/Sprites
	EItemKind Kind = EItemKind::Gear;
	ERarity Rarity = ERarity::Common;
	FLinearColor Tint = FLinearColor::White;
	int32 Price = 0;
	int32 MaxStack = 1;
	int32 Charges = 0;              // 0 = never wears out; otherwise uses per item (a torch burns down)
	int32 BagSlots = 0;             // carrying it opens an extra container with this many slots
	TArray<FString> Grants;         // field abilities it gives while carried
	FString Use;                    // what Use does out of combat: heal, eat, read
	FString LibName;                // rules-library entry this came from, if any
	float Bulk = 0.f;
};

struct FItemStack
{
	FString Id;
	int32 Count = 0;
	int32 Charges = 0;              // uses left on the top item of the stack
	int32 Serial = 0;               // pickup order, for "recent first" sorting
	bool IsEmpty() const { return Id.IsEmpty() || Count <= 0; }
};

struct FItemContainer
{
	FString Id, Name;
	TArray<FItemStack> Slots;       // fixed length; an empty stack is a free slot
	TArray<EItemKind> Refuse;       // kinds this container will not hold

	void Init(const FString& InId, const FString& InName, int32 NumSlots);
	bool Accepts(const FString& ItemId) const;
	int32 Count(const FString& ItemId) const;
	int32 Used() const;
	int32 Capacity(const FString& ItemId) const;       // how many more of this item fit
	int32 Add(const FString& ItemId, int32 N, int32 Serial);   // returns how many did NOT fit
	int32 Remove(const FString& ItemId, int32 N);              // returns how many were removed
};

struct FInventory
{
	TArray<FItemContainer> Bags;    // [0] the pack; [1] the bag of holding while you carry one
	int32 NextSerial = 1;

	void Init();
	int32 Count(const FString& ItemId) const;
	bool Has(const FString& ItemId, int32 N = 1) const { return Count(ItemId) >= N; }
	EAddResult Add(const FString& ItemId, int32 N = 1, int32* OutLeft = nullptr);
	bool Take(const FString& ItemId, int32 N = 1);    // all or nothing
	bool CanGrant(const FString& Ability) const;
	FString GrantingItem(const FString& Ability) const;  // id of the item that gives it, empty if none
	bool UseCharge(const FString& Ability);              // spend a use of the granting item; false if none
	bool Move(int32 FromBag, int32 FromSlot, int32 ToBag, int32 ToSlot);  // merge, move or swap
	bool Drop(int32 Bag, int32 Slot, int32 N);
	void Sort(int32 Bag, int32 By);  // 0 kind, 1 name, 2 recent
	void SyncBags();                 // open or close the bag-of-holding container to match what you carry
	const FItemStack* At(int32 Bag, int32 Slot) const;
};

namespace HollowItems
{
	const TMap<FString, FItemDef>& Defs();
	// Registers a rules-library item (any pf2e weapon, armor, consumable, gear...) and returns its id ("lib:<slug>").
	// Our hand-made items win when the name matches one (a Pathbuilder "Rope" stays our Rope & Grapnel). Empty if unknown.
	FString FromLibrary(const FString& Name);
	const FItemDef* Find(const FString& Id);
	FString KindName(EItemKind K);
	FString RarityName(ERarity R);
	FLinearColor RarityColor(ERarity R);
	// Field abilities: display name and the verb shown on prompts.
	FString AbilityName(const FString& Ability);
	const TArray<FString>& AbilityList();
	// Best-effort match of a free-form gear name (Pathbuilder equipment, loot text) to an item id.
	FString MatchName(const FString& Name);
}
