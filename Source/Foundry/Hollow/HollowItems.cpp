#include "HollowItems.h"

#include "HollowLibrary.h"

namespace HollowItems
{
TMap<FString, FItemDef>& Registry();
const TMap<FString, FItemDef>& Defs() { return Registry(); }

TMap<FString, FItemDef>& Registry()
{
	using enum EItemKind;
	auto I = [](TMap<FString, FItemDef>& M, const TCHAR* Id, const TCHAR* Name, EItemKind Kind, ERarity Rar, int32 Price, int32 Stack,
		const TCHAR* Icon, uint32 Rgb, const TCHAR* Desc) -> FItemDef&
	{
		FItemDef D;
		D.Id = Id; D.Name = Name; D.Kind = Kind; D.Rarity = Rar; D.Price = Price; D.MaxStack = Stack; D.Icon = Icon; D.Desc = Desc;
		D.Tint = FLinearColor::FromSRGBColor(FColor((Rgb >> 16) & 255, (Rgb >> 8) & 255, Rgb & 255));
		return M.Add(Id, D);
	};
	static TMap<FString, FItemDef> M = [&]
	{
		TMap<FString, FItemDef> O;
		// consumables
		I(O, TEXT("potion"), TEXT("Healing Potion"), Consumable, ERarity::Common, 8, 5, TEXT("circle_05"), 0xE0524A,
			TEXT("Thick red draught that tastes of iron and honey. Heals 2d8+4.")).Use = TEXT("heal");
		I(O, TEXT("ration"), TEXT("Trail Ration"), Consumable, ERarity::Common, 2, 10, TEXT("dirt_02"), 0xB98B57,
			TEXT("Hard bread, salt pork, a wrinkled apple. Scouting and camping eat these.")).Use = TEXT("eat");
		I(O, TEXT("holy"), TEXT("Holy Water"), Consumable, ERarity::Uncommon, 10, 5, TEXT("light_02"), 0x9FD4FF,
			TEXT("Blessed at the Starfall Altar. Burns the dead and stops their wounds from knitting."));
		// tools: carrying them gives field abilities
		{ FItemDef& D = I(O, TEXT("shovel"), TEXT("Shovel"), Tool, ERarity::Common, 15, 1, TEXT("dirt_01"), 0x9C8A74,
			TEXT("A gravedigger's spade, the handle worn smooth. Dig up loose earth and buried caches.")); D.Grants = { TEXT("dig") }; }
		{ FItemDef& D = I(O, TEXT("torch"), TEXT("Torch"), Tool, ERarity::Common, 1, 5, TEXT("flame_02"), 0xF29A3A,
			TEXT("Pitch-soaked rag on a stick. Lights dark places and sets things burning. Each one burns down after 3 uses.")); D.Grants = { TEXT("light"), TEXT("fire") }; D.Charges = 3; }
		{ FItemDef& D = I(O, TEXT("tinderbox"), TEXT("Tinderbox"), Tool, ERarity::Common, 3, 1, TEXT("spark_03"), 0xE8C26A,
			TEXT("Flint, steel and dry char-cloth. Light braziers, beacons and campfires.")); D.Grants = { TEXT("fire") }; }
		{ FItemDef& D = I(O, TEXT("lantern"), TEXT("Hooded Lantern"), Tool, ERarity::Uncommon, 25, 1, TEXT("light_01"), 0xFFD27A,
			TEXT("Never gutters in the wind. Lights dark places for as long as you like.")); D.Grants = { TEXT("light") }; }
		{ FItemDef& D = I(O, TEXT("hatchet"), TEXT("Hatchet"), Tool, ERarity::Common, 10, 1, TEXT("slash_03"), 0xA9B2B8,
			TEXT("Short-hafted woodsman's axe. Cuts through brambles and rotten doors.")); D.Grants = { TEXT("cut") }; }
		{ FItemDef& D = I(O, TEXT("rope"), TEXT("Rope & Grapnel"), Tool, ERarity::Common, 6, 1, TEXT("twirl_01"), 0xC7A57A,
			TEXT("Fifty feet of hemp and an iron hook. Climb ledges and cliff faces.")); D.Grants = { TEXT("climb") }; }
		{ FItemDef& D = I(O, TEXT("picks"), TEXT("Thieves' Tools"), Tool, ERarity::Uncommon, 20, 1, TEXT("scratch_01"), 0xBFC6D4,
			TEXT("Picks, tension wrenches and a bent nail for luck. Open locks with Thievery instead of a crowbar.")); D.Grants = { TEXT("pick") }; }
		// gear
		{ FItemDef& D = I(O, TEXT("bag"), TEXT("Bag of Holding"), Gear, ERarity::Rare, 45, 1, TEXT("twirl_03"), 0xB79CDB,
			TEXT("Bigger on the inside, and it hums when you sleep. Opens a second container with 16 slots.")); D.BagSlots = 16; }
		// valuables: sell them, or someone may want them
		I(O, TEXT("pelt"), TEXT("Wolf Pelt"), Valuable, ERarity::Common, 5, 10, TEXT("smoke_04"), 0x8C8478,
			TEXT("Grey and coarse. Maren pays five silver a pelt for the ridge bounty."));
		I(O, TEXT("trinket"), TEXT("Grave Trinket"), Valuable, ERarity::Uncommon, 12, 10, TEXT("star_06"), 0xD8A94F,
			TEXT("A ring, a locket, a tooth set in silver. The dead don't need it."));
		I(O, TEXT("shard"), TEXT("Star-Glass Shard"), Valuable, ERarity::Rare, 30, 10, TEXT("magic_05"), 0x9C6CFF,
			TEXT("Violet glass that is warm to the touch. The temple wants these badly, and won't say why."));
		// quest
		I(O, TEXT("charm"), TEXT("Odo's Bone Charm"), Quest, ERarity::Unique, 0, 1, TEXT("symbol_02"), 0xE6DCC6,
			TEXT("A finger bone wrapped in red thread. It belonged to Odo Tanner, who walked north-east humming."));
		return O;
	}();
	return M;
}

const FItemDef* Find(const FString& Id) { return Defs().Find(Id); }

FString FromLibrary(const FString& Name)
{
	const FString Ours = MatchName(Name);
	if (!Ours.IsEmpty()) return Ours;
	const FLibItem* L = HollowLib::Item(Name);
	if (!L) return FString();
	const FString Id = TEXT("lib:") + HollowLib::Slug(L->Name);
	if (Registry().Contains(Id)) return Id;
	FItemDef D;
	D.Id = Id; D.Name = L->Name; D.LibName = L->Name; D.Bulk = L->Bulk;
	D.Price = FMath::Max(0, FMath::RoundToInt(L->Price));
	D.Rarity = L->Rarity == TEXT("uncommon") ? ERarity::Uncommon : L->Rarity == TEXT("rare") ? ERarity::Rare : L->Rarity == TEXT("unique") ? ERarity::Unique : ERarity::Common;
	D.Desc = L->Text;
	auto Tint = [](uint32 Rgb) { return FLinearColor::FromSRGBColor(FColor((Rgb >> 16) & 255, (Rgb >> 8) & 255, Rgb & 255)); };
	if (L->Type == TEXT("weapon"))
	{
		D.Kind = EItemKind::Gear; D.Icon = TEXT("slash_01"); D.Tint = Tint(0xC9CED6);
		D.Desc = FString::Printf(TEXT("%d%s %s, %s %s. %s"), L->Dice, *L->Die, *L->DmgType, *L->Category, *L->Group, *L->Text);
	}
	else if (L->Type == TEXT("armor") || L->Type == TEXT("shield"))
	{
		D.Kind = EItemKind::Gear; D.Icon = TEXT("circle_03"); D.Tint = Tint(0x9AA3AE);
		D.Desc = FString::Printf(TEXT("+%d AC%s. %s"), L->Ac, L->DexCap < 99 ? *FString::Printf(TEXT(", Dex cap +%d"), L->DexCap) : TEXT(""), *L->Text);
	}
	else if (L->Type == TEXT("consumable"))
	{
		D.Kind = EItemKind::Consumable; D.MaxStack = 10; D.Icon = TEXT("circle_05"); D.Tint = Tint(0x7FB86A);
		if (L->Name.Contains(TEXT("Healing"))) { D.Use = TEXT("heal"); D.Tint = Tint(0xE0524A); }
	}
	else if (L->Type == TEXT("treasure")) { D.Kind = EItemKind::Valuable; D.MaxStack = 20; D.Icon = TEXT("star_06"); D.Tint = Tint(0xD8A94F); }
	else { D.Kind = EItemKind::Gear; D.MaxStack = L->Bulk < 0.5f ? 10 : 1; D.Icon = TEXT("symbol_01"); D.Tint = Tint(0xBFA78A); }
	Registry().Add(Id, D);
	return Id;
}

FString KindName(EItemKind K)
{
	switch (K)
	{
	case EItemKind::Tool: return TEXT("Tool");
	case EItemKind::Consumable: return TEXT("Consumable");
	case EItemKind::Gear: return TEXT("Gear");
	case EItemKind::Valuable: return TEXT("Valuable");
	default: return TEXT("Quest");
	}
}

FString RarityName(ERarity R)
{
	static const TCHAR* N[] = { TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"), TEXT("Legendary"), TEXT("Unique") };
	return N[(int32)R];
}

FLinearColor RarityColor(ERarity R)
{
	switch (R)
	{
	case ERarity::Uncommon: return FLinearColor(0.38f, 0.75f, 0.42f);
	case ERarity::Rare: return FLinearColor(0.36f, 0.56f, 1.f);
	case ERarity::Legendary: return FLinearColor(1.f, 0.55f, 0.15f);
	case ERarity::Unique: return FLinearColor(0.78f, 0.5f, 1.f);
	default: return FLinearColor(0.82f, 0.78f, 0.72f);
	}
}

const TArray<FString>& AbilityList()
{
	static const TArray<FString> L = { TEXT("dig"), TEXT("light"), TEXT("fire"), TEXT("cut"), TEXT("climb"), TEXT("pick") };
	return L;
}

FString AbilityName(const FString& A)
{
	static const TMap<FString, FString> N = {
		{ TEXT("dig"), TEXT("Dig") }, { TEXT("light"), TEXT("Light") }, { TEXT("fire"), TEXT("Kindle") },
		{ TEXT("cut"), TEXT("Cut") }, { TEXT("climb"), TEXT("Climb") }, { TEXT("pick"), TEXT("Pick Lock") } };
	const FString* F = N.Find(A);
	return F ? *F : A;
}

FString MatchName(const FString& Name)
{
	const FString L = Name.ToLower();
	static const TArray<TPair<const TCHAR*, const TCHAR*>> Keys = {
		{ TEXT("bag of holding"), TEXT("bag") }, { TEXT("healing potion"), TEXT("potion") }, { TEXT("elixir of life"), TEXT("potion") },
		{ TEXT("ration"), TEXT("ration") }, { TEXT("holy water"), TEXT("holy") }, { TEXT("shovel"), TEXT("shovel") }, { TEXT("spade"), TEXT("shovel") },
		{ TEXT("torch"), TEXT("torch") }, { TEXT("flint and steel"), TEXT("tinderbox") }, { TEXT("tinderbox"), TEXT("tinderbox") },
		{ TEXT("lantern"), TEXT("lantern") }, { TEXT("hatchet"), TEXT("hatchet") }, { TEXT("grappling"), TEXT("rope") },
		{ TEXT("rope"), TEXT("rope") }, { TEXT("thieves"), TEXT("picks") }, { TEXT("lockpick"), TEXT("picks") } };
	for (const auto& K : Keys) if (L.Contains(K.Key)) return K.Value;
	return FString();
}
}

/* ---------- Containers ---------- */

void FItemContainer::Init(const FString& InId, const FString& InName, int32 NumSlots)
{
	Id = InId; Name = InName;
	Slots.SetNum(NumSlots);
}

bool FItemContainer::Accepts(const FString& ItemId) const
{
	const FItemDef* D = HollowItems::Find(ItemId);
	return D && !Refuse.Contains(D->Kind);
}

int32 FItemContainer::Count(const FString& ItemId) const
{
	int32 N = 0;
	for (const FItemStack& S : Slots) if (S.Id == ItemId) N += S.Count;
	return N;
}

int32 FItemContainer::Used() const
{
	int32 N = 0;
	for (const FItemStack& S : Slots) if (!S.IsEmpty()) N++;
	return N;
}

int32 FItemContainer::Capacity(const FString& ItemId) const
{
	const FItemDef* D = HollowItems::Find(ItemId);
	if (!D || !Accepts(ItemId)) return 0;
	int32 N = 0;
	for (const FItemStack& S : Slots)
	{
		if (S.IsEmpty()) N += D->MaxStack;
		else if (S.Id == ItemId) N += FMath::Max(0, D->MaxStack - S.Count);
	}
	return N;
}

int32 FItemContainer::Add(const FString& ItemId, int32 N, int32 Serial)
{
	const FItemDef* D = HollowItems::Find(ItemId);
	if (!D || !Accepts(ItemId)) return N;
	// top up existing stacks first, then open new ones (like ItemStack.TryMerge before a new slot)
	for (FItemStack& S : Slots)
	{
		if (N <= 0) break;
		if (S.Id != ItemId || S.Count >= D->MaxStack) continue;
		const int32 Put = FMath::Min(N, D->MaxStack - S.Count);
		S.Count += Put; S.Serial = Serial; N -= Put;
	}
	for (FItemStack& S : Slots)
	{
		if (N <= 0) break;
		if (!S.IsEmpty()) continue;
		const int32 Put = FMath::Min(N, D->MaxStack);
		S.Id = ItemId; S.Count = Put; S.Charges = D->Charges; S.Serial = Serial; N -= Put;
	}
	return N;
}

int32 FItemContainer::Remove(const FString& ItemId, int32 N)
{
	int32 Done = 0;
	// take from the newest stacks first so partly-burnt torches get used up last
	for (int32 i = Slots.Num() - 1; i >= 0 && Done < N; --i)
	{
		FItemStack& S = Slots[i];
		if (S.Id != ItemId) continue;
		const int32 Take = FMath::Min(S.Count, N - Done);
		S.Count -= Take; Done += Take;
		if (S.Count <= 0) S = FItemStack();
	}
	return Done;
}

/* ---------- Inventory ---------- */

void FInventory::Init()
{
	Bags.Reset();
	FItemContainer& Pack = Bags.AddDefaulted_GetRef();
	Pack.Init(TEXT("pack"), TEXT("Pack"), 12);
}

int32 FInventory::Count(const FString& ItemId) const
{
	int32 N = 0;
	for (const FItemContainer& B : Bags) N += B.Count(ItemId);
	return N;
}

EAddResult FInventory::Add(const FString& ItemId, int32 N, int32* OutLeft)
{
	const int32 Want = N;
	const int32 Serial = NextSerial++;
	// destination routing (Nivalis: InventoryData.GetDestinationContainerForItemType): the pack first, overflow into the bag
	for (FItemContainer& B : Bags)
	{
		if (N <= 0) break;
		N = B.Add(ItemId, N, Serial);
	}
	SyncBags();
	if (OutLeft) *OutLeft = N;
	return N <= 0 ? EAddResult::Succeeded : N < Want ? EAddResult::Partial : EAddResult::Failed;
}

bool FInventory::Take(const FString& ItemId, int32 N)
{
	if (Count(ItemId) < N) return false;
	for (int32 b = Bags.Num() - 1; b >= 0 && N > 0; --b) N -= Bags[b].Remove(ItemId, N);
	SyncBags();
	return true;
}

FString FInventory::GrantingItem(const FString& Ability) const
{
	// prefer items that never wear out (a lantern over a torch)
	FString Spent;
	for (const FItemContainer& B : Bags)
		for (const FItemStack& S : B.Slots)
		{
			if (S.IsEmpty()) continue;
			const FItemDef* D = HollowItems::Find(S.Id);
			if (!D || !D->Grants.Contains(Ability)) continue;
			if (D->Charges == 0) return S.Id;
			Spent = S.Id;
		}
	return Spent;
}

bool FInventory::CanGrant(const FString& Ability) const { return !GrantingItem(Ability).IsEmpty(); }

bool FInventory::UseCharge(const FString& Ability)
{
	const FString Id = GrantingItem(Ability);
	const FItemDef* D = HollowItems::Find(Id);
	if (!D) return false;
	if (D->Charges == 0) return true;
	for (FItemContainer& B : Bags)
		for (FItemStack& S : B.Slots)
			if (S.Id == Id)
			{
				if (--S.Charges <= 0)
				{
					S.Count--;
					S.Charges = D->Charges;
					if (S.Count <= 0) S = FItemStack();
				}
				return true;
			}
	return false;
}

const FItemStack* FInventory::At(int32 Bag, int32 Slot) const
{
	return Bags.IsValidIndex(Bag) && Bags[Bag].Slots.IsValidIndex(Slot) ? &Bags[Bag].Slots[Slot] : nullptr;
}

bool FInventory::Move(int32 FromBag, int32 FromSlot, int32 ToBag, int32 ToSlot)
{
	if (!At(FromBag, FromSlot) || !At(ToBag, ToSlot) || (FromBag == ToBag && FromSlot == ToSlot)) return false;
	FItemStack& A = Bags[FromBag].Slots[FromSlot];
	FItemStack& B = Bags[ToBag].Slots[ToSlot];
	if (A.IsEmpty()) return false;
	if (!Bags[ToBag].Accepts(A.Id)) return false;
	if (!B.IsEmpty() && !Bags[FromBag].Accepts(B.Id)) return false;
	// the bag of holding can't go inside itself
	if (ToBag > 0 && A.Id == TEXT("bag")) return false;
	if (B.Id == A.Id)
	{
		const FItemDef* D = HollowItems::Find(A.Id);
		const int32 Put = FMath::Min(A.Count, D->MaxStack - B.Count);
		if (Put <= 0) Swap(A, B);
		else { B.Count += Put; A.Count -= Put; if (A.Count <= 0) A = FItemStack(); }
	}
	else Swap(A, B);
	return true;
}

bool FInventory::Drop(int32 Bag, int32 Slot, int32 N)
{
	if (!At(Bag, Slot) || At(Bag, Slot)->IsEmpty()) return false;
	FItemStack& S = Bags[Bag].Slots[Slot];
	const FItemDef* D = HollowItems::Find(S.Id);
	if (D && D->Kind == EItemKind::Quest) return false;
	if (S.Id == TEXT("bag") && Bags.Num() > 1 && Bags[1].Used() > 0) return false;
	S.Count -= FMath::Min(N, S.Count);
	if (S.Count <= 0) S = FItemStack();
	SyncBags();
	return true;
}

void FInventory::Sort(int32 Bag, int32 By)
{
	if (!Bags.IsValidIndex(Bag)) return;
	TArray<FItemStack>& S = Bags[Bag].Slots;
	S.StableSort([By](const FItemStack& A, const FItemStack& B)
	{
		if (A.IsEmpty() != B.IsEmpty()) return !A.IsEmpty();
		if (A.IsEmpty()) return false;
		const FItemDef* Da = HollowItems::Find(A.Id);
		const FItemDef* Db = HollowItems::Find(B.Id);
		if (By == 2) return A.Serial > B.Serial;
		if (By == 0 && Da->Kind != Db->Kind) return Da->Kind < Db->Kind;
		return Da->Name < Db->Name;
	});
}

void FInventory::SyncBags()
{
	int32 Slots = 0;
	for (const FItemStack& S : Bags[0].Slots)
		if (const FItemDef* D = HollowItems::Find(S.Id)) Slots = FMath::Max(Slots, D->BagSlots);
	if (Slots > 0 && Bags.Num() < 2)
	{
		FItemContainer& B = Bags.AddDefaulted_GetRef();
		B.Init(TEXT("bag"), TEXT("Bag of Holding"), Slots);
		B.Refuse = { EItemKind::Quest };
	}
	else if (Slots == 0 && Bags.Num() > 1 && Bags[1].Used() == 0) Bags.SetNum(1);
}
