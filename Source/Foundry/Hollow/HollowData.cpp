#include "HollowTypes.h"
#include "HollowLibrary.h"

namespace
{
	FOutcome O(const TCHAR* T) { FOutcome R; R.Text = T; return R; }

	FChoice Chk(const TCHAR* Label, TArray<FString> Skills, int32 DC, const FOutcome& Ok, const FOutcome& Fail)
	{
		FChoice C; C.Label = Label; C.Skills = MoveTemp(Skills); C.DC = DC; C.Ok = Ok; C.Fail = Fail; return C;
	}
	FChoice ChkCF(const TCHAR* Label, TArray<FString> Skills, int32 DC, const FOutcome& Ok, const FOutcome& Fail, const FOutcome& CF)
	{
		FChoice C = Chk(Label, MoveTemp(Skills), DC, Ok, Fail); C.CritFail = CF; C.bHasCritFail = true; return C;
	}
	FChoice Pl(const TCHAR* Label, const FOutcome& Out, int32 Cost = 0)
	{
		FChoice C; C.Label = Label; C.Out = Out; C.Cost = Cost; return C;
	}
	FEventDef Ev(const TCHAR* Id, TArray<ETerr> Terr, const TCHAR* Title, const TCHAR* Text, TArray<FChoice> Choices)
	{
		FEventDef E; E.Id = Id; E.Terr = MoveTemp(Terr); E.Title = Title; E.Text = Text; E.Choices = MoveTemp(Choices); return E;
	}
	FLinearColor Hex(const TCHAR* S) { return FLinearColor(FColor::FromHex(S)); }
}

const FTerrInfo& TerrInfo(ETerr T)
{
	static const TMap<ETerr, FTerrInfo> M = {
		{ ETerr::Forest, { TEXT("Gloamwood"),       Hex(TEXT("2c3b25")), Hex(TEXT("252c1b")), 3.f } },
		{ ETerr::Plains, { TEXT("Ashen Heath"),     Hex(TEXT("5a5232")), Hex(TEXT("3a3524")), 3.f } },
		{ ETerr::Hills,  { TEXT("Barrow Hills"),    Hex(TEXT("4f4532")), Hex(TEXT("332f22")), 2.f } },
		{ ETerr::Swamp,  { TEXT("Mirefen"),         Hex(TEXT("2b3d33")), Hex(TEXT("1f2a22")), 1.5f } },
		{ ETerr::Ruins,  { TEXT("Old Ruins"),       Hex(TEXT("45404d")), Hex(TEXT("2c2a2c")), 1.5f } },
		{ ETerr::River,  { TEXT("River Lull"),      Hex(TEXT("2d4a63")), Hex(TEXT("2a3324")), 1.f } },
		{ ETerr::Town,   { TEXT("Brindlemark"),     Hex(TEXT("8a5a2b")), Hex(TEXT("2d3322")), 0.f } },
		{ ETerr::Keep,   { TEXT("Hollowmere Keep"), Hex(TEXT("4a1a26")), Hex(TEXT("1d1b21")), 0.f } },
		{ ETerr::Shrine, { TEXT("Starfall Altar"),  Hex(TEXT("5b3c8a")), Hex(TEXT("2a2b26")), 0.f } },
	};

	return M[T];
}

namespace HollowData
{

const TArray<FEventDef>& Events()
{
	using enum ETerr;
	static const TArray<FEventDef> E = {
		Ev(TEXT("snare"), { Forest }, TEXT("The Snared Wolf"),
			TEXT("A grey wolf thrashes in a poacher's snare, one leg bloodied. It watches you with yellow eyes and does not howl."), {
			Chk(TEXT("Work the snare loose"), { TEXT("survival") }, 15,
				O(TEXT("You calm it with low words and slow hands. It limps into the pines, and nearby you find the poacher's cache.")).xp(15).rations(2),
				O(TEXT("The snare springs back and the wolf lunges. Teeth close on your forearm before it tears free and runs.")).dmg(TEXT("1d6")).xp(5)),
			Pl(TEXT("End its pain and take the meat"), O(TEXT("A quick death. You dress the carcass and pack what will keep.")).rations(3)),
			Pl(TEXT("Leave it to the forest"), O(TEXT("You walk on. Behind you the thrashing stops, eventually."))) }),
		Ev(TEXT("ridge"), { Forest, Hills }, TEXT("Howls on the Ridge"),
			TEXT("Three long howls answer each other across the ridge. Something has your scent."), {
			Chk(TEXT("Slip away downwind"), { TEXT("stealth") }, 16,
				O(TEXT("You wade a cold stream and the howls drift north.")).xp(10),
				O(TEXT("A branch cracks under your boot. The pack leader breaks from the trees.")).combat(TEXT("wolf"))),
			Pl(TEXT("Stand your ground"), O(TEXT("You plant your feet and wait. The leader comes alone.")).combat(TEXT("wolf"))) }),
		Ev(TEXT("widow"), { Forest, Plains }, TEXT("The Woodcutter's Widow"),
			TEXT("Smoke rises from a mossy cabin. An old woman on the step offers stew to anyone with news from the road."), {
			Chk(TEXT("Share news and stories"), { TEXT("diplomacy") }, 14,
				O(TEXT("She laughs at your tales and presses smoked meat on you. \"The keep's lord was a man once,\" she says. \"Holy water still burns him.\"")).rations(2).xp(10),
				O(TEXT("Your stories land badly. She ladles you a thin bowl and shuts the door.")).rations(1)),
			Pl(TEXT("Buy supplies"), O(TEXT("She sells you bread, cheese and a jar of something pickled.")).rations(3), 5),
			Pl(TEXT("Move on"), O(TEXT("You nod and keep walking."))) }),
		Ev(TEXT("barrow"), { Hills }, TEXT("The Old Barrow"),
			TEXT("A stone door is set into the hillside, carved with a ring of warding runes. Cold air leaks around its edges."), {
			Chk(TEXT("Read the warding runes"), { TEXT("religion"), TEXT("arcana") }, 15,
				O(TEXT("The runes name the dead and ask for peace. You speak the words and the door grinds open on grave-goods and a sealed vial.")).gold(25).potions(1).xp(15),
				O(TEXT("You mangle the rite. Something inside answers.")).combat(TEXT("skeleton"))),
			Chk(TEXT("Force the door"), { TEXT("athletics") }, 17,
				O(TEXT("The door gives with a groan. You grab what you can before the dust settles.")).gold(20).xp(10),
				O(TEXT("You wrench your shoulder on the stone.")).dmg(TEXT("1d6"))),
			Pl(TEXT("Leave the dead alone"), O(TEXT("Some doors are shut for a reason."))) }),
		Ev(TEXT("rockslide"), { Hills }, TEXT("Rockslide"),
			TEXT("The slope above you shifts with a sound like distant thunder."), {
			ChkCF(TEXT("Dive for the overhang"), { TEXT("acrobatics") }, 15,
				O(TEXT("You roll under the ledge as the stones roar past.")).xp(10),
				O(TEXT("A rock clips you before you reach cover.")).dmg(TEXT("2d6")).xp(5),
				O(TEXT("You are buried to the waist, and part of your pack is lost.")).dmg(TEXT("2d6")).rations(-1)),
			Chk(TEXT("Brace and cover your head"), { TEXT("athletics") }, 16,
				O(TEXT("The stones break around you like surf.")).xp(10),
				O(TEXT("You are knocked flat and battered.")).dmg(TEXT("2d6"))) }),
		Ev(TEXT("wisps"), { Swamp }, TEXT("Lights in the Bog"),
			TEXT("Pale lights bob over the black water, drifting away whenever you look straight at them."), {
			Chk(TEXT("Follow, carefully"), { TEXT("perception") }, 16,
				O(TEXT("You track the lights to a drowned merchant, still clutching his purse.")).gold(30).xp(15),
				O(TEXT("The ground gives way. You haul yourself out of the bog soaked and bitten.")).dmg(TEXT("1d8")).rations(-1)),
			Pl(TEXT("Look away and keep to the path"), O(TEXT("The lights sulk and fade."))) }),
		Ev(TEXT("drowned"), { Swamp, River }, TEXT("It Rises"),
			TEXT("The water bulges. A bloated shape in rotted mail climbs out of the reeds, weed trailing from its jaw."), {
			Pl(TEXT("Fight it on the bank"), O(TEXT("Steel out, you meet it at the waterline.")).combat(TEXT("drowned"))),
			Chk(TEXT("Drive it back with a war-cry"), { TEXT("intimidation") }, 17,
				O(TEXT("It flinches from your voice and sinks back under.")).xp(20),
				O(TEXT("It does not know fear any more.")).combat(TEXT("drowned"))) }),
		Ev(TEXT("choir"), { Ruins }, TEXT("The Bone Choir"),
			TEXT("In a roofless chapel, skeletons in monk's rags stand in rows, jaws moving without sound."), {
			Chk(TEXT("Sneak past to the altar"), { TEXT("stealth") }, 15,
				O(TEXT("You lift the offering bowl and leave the choir to its hymn.")).gold(20).xp(15),
				O(TEXT("A skull turns toward you. Then another.")).combat(TEXT("skeleton"))),
			Pl(TEXT("Break the choir"), O(TEXT("The nearest one steps out of the row to meet you.")).combat(TEXT("skeleton"))),
			Pl(TEXT("Back away"), O(TEXT("You leave them singing."))) }),
		Ev(TEXT("reliquary"), { Ruins }, TEXT("The Locked Reliquary"),
			TEXT("An iron box is chained to a collapsed pillar. The lock is old but intricate."), {
			Chk(TEXT("Pick the lock"), { TEXT("thievery") }, 16,
				O(TEXT("Click. Inside: coin, and a flask of blessed water.")).gold(20).holy(1).xp(15),
				O(TEXT("A needle in the keyhole bites your finger. Your hand throbs for hours.")).dmg(TEXT("1d6"))),
			Chk(TEXT("Smash the chain"), { TEXT("athletics") }, 18,
				O(TEXT("The chain snaps. The box is yours.")).gold(20).xp(10),
				O(TEXT("The racket echoes through the ruins. Something comes.")).combat(TEXT("ghoul"))) }),
		Ev(TEXT("toll"), { Plains, Forest }, TEXT("Toll on the Old Road"),
			TEXT("Two masked figures block the road. \"Fifteen silver for safe passage,\" says the taller one."), {
			Pl(TEXT("Pay the toll"), O(TEXT("They step aside with mocking bows.")), 15),
			Chk(TEXT("Make them reconsider"), { TEXT("intimidation") }, 16,
				O(TEXT("They look at each other, then at you, and decide it isn't worth it.")).xp(15),
				O(TEXT("\"Wrong answer.\"")).combat(TEXT("bandit"))),
			Pl(TEXT("Draw steel"), O(TEXT("The tall one draws too.")).combat(TEXT("bandit"))) }),
		Ev(TEXT("pilgrims"), { Plains, River }, TEXT("Pilgrim Caravan"),
			TEXT("Pilgrims of the Dawn have stopped by the road. One of them is coughing blood into a cloth."), {
			Chk(TEXT("Tend the sick pilgrim"), { TEXT("medicine") }, 14,
				O(TEXT("You ease the fever. The pilgrims bless you and press coins into your hand.")).gold(10).xp(15),
				O(TEXT("You do what you can. They thank you anyway.")).xp(5)),
			Pl(TEXT("Trade for supplies"), O(TEXT("They part with dried fruit and hard bread.")).rations(2), 4),
			Pl(TEXT("Pass by"), O(TEXT("Their hymn follows you down the road."))) }),
		Ev(TEXT("abandoned"), { Plains, Forest, Hills }, TEXT("Abandoned Camp"),
			TEXT("A cold firepit, a torn tent, a cookpot still half full. Whoever camped here left in a hurry."), {
			Chk(TEXT("Search for a hidden cache"), { TEXT("perception") }, 15,
				O(TEXT("Under a loose stone: a coin purse and a healing draught.")).gold(15).potions(1).xp(10),
				O(TEXT("You find only cold ashes and a bad feeling."))),
			Chk(TEXT("Salvage food"), { TEXT("survival") }, 13,
				O(TEXT("Enough salt pork for a few days.")).rations(2).xp(5),
				O(TEXT("Most of it has turned. You save a little.")).rations(1)) }),
		Ev(TEXT("ford"), { River }, TEXT("The Ford"),
			TEXT("The river runs high and brown. A ferryman dozes beside a flat-bottomed boat."), {
			Chk(TEXT("Wade across"), { TEXT("athletics") }, 14,
				O(TEXT("Cold to the chest, but you make it.")).xp(10),
				O(TEXT("The current takes your feet. You crawl out downstream, soaked and short on supplies.")).dmg(TEXT("1d6")).rations(-1)),
			Pl(TEXT("Pay the ferryman"), O(TEXT("He poles you across without a word.")), 3) }),
		Ev(TEXT("fish"), { River, Swamp }, TEXT("Fisher's Luck"),
			TEXT("The eddies are thick with fish, and you have a little line in your pack."), {
			Chk(TEXT("Fish for an hour"), { TEXT("survival") }, 12,
				O(TEXT("A good catch, smoked over a small fire.")).rations(3).xp(5),
				O(TEXT("One small fish. Better than nothing.")).rations(1)) }),
		Ev(TEXT("feeding"), { Ruins, Swamp, Forest }, TEXT("Feeding Ghoul"),
			TEXT("Something hunches over a carcass in the dark, cracking bones. It hasn't seen you yet."), {
			Pl(TEXT("Strike first"), O(TEXT("You charge before it can turn. It is off-guard.")).combat(TEXT("ghoul")).ambush()),
			Chk(TEXT("Slip away"), { TEXT("stealth") }, 16,
				O(TEXT("You leave it to its meal.")).xp(10),
				O(TEXT("Its head snaps up, nostrils flaring.")).combat(TEXT("ghoul"))) }),

		Ev(TEXT("shrine"), {}, TEXT("Shrine of the Dawn"),
			TEXT("A sunstone shrine, its gilding worn thin by pilgrims' hands. The air around it is warm."), {
			Chk(TEXT("Kneel and pray"), { TEXT("religion"), TEXT("diplomacy") }, 13,
				O(TEXT("Warmth floods through you. Your wounds close and your mind clears.")).heal(TEXT("full")).slots().xp(10),
				O(TEXT("You feel a faint warmth, no more.")).heal(TEXT("half"))),
			Chk(TEXT("Take the offerings"), { TEXT("thievery") }, 12,
				O(TEXT("You pocket the coins pilgrims left. The warmth fades from the stone.")).gold(25),
				O(TEXT("Your fingers burn. The shrine does not forgive.")).dmg(TEXT("2d6"))) }),
		Ev(TEXT("keep"), {}, TEXT("The Gates of Hollowmere"),
			TEXT("The keep stands black against a bruised sky. The gate is open. Somewhere inside, the Hollow Lord is waiting for you."), {
			Chk(TEXT("Speak the rite of unbinding"), { TEXT("religion") }, 18,
				O(TEXT("Your words ring through the hall. Varek staggers as his stolen life bleeds out of him.")).combat(TEXT("boss")).weaken(),
				O(TEXT("He laughs at your faith and rises from his throne.")).combat(TEXT("boss"))),
			Pl(TEXT("Storm the hall"), O(TEXT("You go in fighting.")).combat(TEXT("boss"))),
			Pl(TEXT("Not yet. Turn back"), O(TEXT("You retreat into the dark. The keep will wait.")).noclear()) }),
		Ev(TEXT("quiet"), {}, TEXT("Quiet Country"),
			TEXT("Wind in the grass and nothing else. A good place to forage."), {
			Chk(TEXT("Forage as you walk"), { TEXT("survival") }, 15,
				O(TEXT("Roots, berries and a snared hare.")).rations(2),
				O(TEXT("Slim pickings."))),
			Pl(TEXT("Keep moving"), O(TEXT("You make good time."))) }),
		Ev(TEXT("ambush_wolf"), {}, TEXT("Teeth in the Dark"),
			TEXT("You hear it before you see it: a low growl from the brush beside the trail."), {
			Pl(TEXT("Fight"), O(TEXT("It comes for your throat.")).combat(TEXT("wolf"))),
			Chk(TEXT("Back away slowly"), { TEXT("survival"), TEXT("intimidation") }, 15,
				O(TEXT("You hold its eyes and give ground. It lets you go.")).xp(5),
				O(TEXT("It takes your retreat as weakness.")).combat(TEXT("wolf"))) }),
	};
	return E;
}

const FEventDef* FindEvent(const FString& Id)
{
	return Events().FindByPredicate([&](const FEventDef& E) { return E.Id == Id; });
}

const TMap<FString, FFoeDef>& Foes()
{
	auto F = [](const TCHAR* Id, const TCHAR* Name, int32 Hp, int32 Ac, int32 Per, int32 Fort, int32 Ref, int32 Will, int32 Atk,
		const TCHAR* Dmg, const TCHAR* Weapon, int32 Xp, int32 Gold, int32 Speed, bool bUndead, const TCHAR* Special)
	{
		FFoeDef D; D.Id = Id; D.Name = Name; D.Hp = Hp; D.Ac = Ac; D.Per = Per; D.Fort = Fort; D.Ref = Ref; D.Will = Will; D.Atk = Atk;
		D.Dmg = Dmg; D.Weapon = Weapon; D.Xp = Xp; D.Gold = Gold; D.Speed = Speed; D.bUndead = bUndead; D.Special = Special;
		return TPair<FString, FFoeDef>(Id, D);
	};
	static TMap<FString, FFoeDef> M = [&]
	{
		TMap<FString, FFoeDef> R;
		R.Add(F(TEXT("wolf"),     TEXT("Grey Wolf"),      22, 15, 7, 7, 9, 5, 7, TEXT("1d6+2"), TEXT("jaws"), 20, 0, 30, false, TEXT("knockdown")));
		R.Add(F(TEXT("skeleton"), TEXT("Skeleton Monk"),  16, 16, 6, 4, 8, 4, 6, TEXT("1d6+2"), TEXT("rusted scimitar"), 15, 8, 25, true, TEXT("")));
		R.Add(F(TEXT("bandit"),   TEXT("Masked Bandit"),  20, 16, 7, 7, 8, 5, 8, TEXT("1d8+2"), TEXT("shortsword"), 20, 18, 25, false, TEXT("dirty")));
		R.Add(F(TEXT("ghoul"),    TEXT("Ghoul"),          20, 16, 7, 6, 8, 5, 7, TEXT("1d6+1"), TEXT("claws"), 20, 5, 25, true, TEXT("paralyze")));
		R.Add(F(TEXT("drowned"),  TEXT("Drowned Knight"), 30, 17, 8, 10, 6, 7, 9, TEXT("1d8+3"), TEXT("grasping hands"), 30, 12, 20, true, TEXT("grab")));
		FFoeDef B = F(TEXT("boss"), TEXT("Varek, the Hollow Lord"), 55, 19, 10, 11, 9, 10, 10, TEXT("1d10+3"), TEXT("cold talons"), 100, 0, 25, true, TEXT("lord")).Value;
		B.bBoss = true; B.Regen = 3;
		R.Add(B.Id, B);
		return R;
	}();
	return M;
}

TMap<FString, FCardDef>& MutableCards();
const TMap<FString, FCardDef>& Cards() { return MutableCards(); }

TMap<FString, FCardDef>& MutableCards()
{
	auto C = [](const TCHAR* Id, const TCHAR* Name, const TCHAR* Kind, int32 Cost, ECardFx Fx, bool bTarget, const TCHAR* Text)
	{
		static const TMap<FString, FLinearColor> KindCol = {
			{ TEXT("Attack"), Hex(TEXT("b8452f")) }, { TEXT("Skill"), Hex(TEXT("4f7f3c")) }, { TEXT("Spell"), Hex(TEXT("4b55a8")) },
			{ TEXT("Tempo"), Hex(TEXT("c58a1e")) }, { TEXT("Aura"), Hex(TEXT("8a4bb0")) } };
		FCardDef D; D.Id = Id; D.Name = Name; D.Kind = Kind; D.Cost = Cost; D.Fx = Fx; D.bNeedsTarget = bTarget; D.Text = Text;
		D.Col = KindCol.FindRef(Kind);
		return TPair<FString, FCardDef>(Id, D);
	};
	using enum ECardFx;
	static TMap<FString, FCardDef> M = {
		C(TEXT("power_attack"), TEXT("Power Attack"), TEXT("Attack"), 2, PowerAttack, true, TEXT("Strike with an extra weapon die. Counts as two attacks for your penalty.")),
		C(TEXT("double_slice"), TEXT("Double Slice"), TEXT("Attack"), 2, DoubleSlice, true, TEXT("Strike twice. Both use your current attack penalty.")),
		C(TEXT("sudden_charge"), TEXT("Sudden Charge"), TEXT("Attack"), 2, SuddenCharge, true, TEXT("Strike at +2. On a hit the foe's next turn is pushed back.")),
		C(TEXT("intimidate"), TEXT("Demoralize"), TEXT("Skill"), 1, Intimidate, true, TEXT("Intimidation vs Will DC. Frightened 1, or 2 on a crit.")),
		C(TEXT("feint"), TEXT("Feint"), TEXT("Skill"), 1, Feint, true, TEXT("Deception vs Perception DC. Foe is off-guard (-2 AC) until your next turn.")),
		C(TEXT("tumble"), TEXT("Tumble Through"), TEXT("Skill"), 1, Tumble, true, TEXT("Acrobatics vs Reflex DC. Foe off-guard and you gain +1 AC.")),
		C(TEXT("hamstring"), TEXT("Hamstring"), TEXT("Tempo"), 1, Hamstring, true, TEXT("Athletics vs Fortitude DC. Push the foe's next turn far back.")),
		C(TEXT("force_barrage"), TEXT("Force Barrage"), TEXT("Spell"), 2, ForceBarrage, true, TEXT("Two bolts of force, 1d4+1 each. Never misses.")),
		C(TEXT("electric_arc"), TEXT("Electric Arc"), TEXT("Spell"), 2, ElectricArc, false, TEXT("Every foe takes 2d4+mod, basic Reflex save.")),
		C(TEXT("fear"), TEXT("Fear"), TEXT("Spell"), 1, Fear, true, TEXT("Will save. Frightened 1, 2 on a failure, 3 on a critical failure.")),
		C(TEXT("shield_spell"), TEXT("Shield"), TEXT("Spell"), 1, ShieldSpell, false, TEXT("+1 AC until your next turn, and absorb the first 5 damage.")),
		C(TEXT("heal"), TEXT("Heal"), TEXT("Spell"), 2, Heal, false, TEXT("Restore 1d8+8 HP.")),
		C(TEXT("divine_lance"), TEXT("Divine Lance"), TEXT("Spell"), 2, DivineLance, true, TEXT("Spell attack for 2d4+mod. +1d4 against undead.")),
		C(TEXT("bless"), TEXT("Bless"), TEXT("Aura"), 1, Bless, false, TEXT("Aura, 3 turns: +1 to all your attack rolls.")),
		C(TEXT("battle_hymn"), TEXT("Battle Hymn"), TEXT("Aura"), 1, BattleHymn, false, TEXT("Aura, 3 turns: +2 damage on everything you hit with.")),
		C(TEXT("holy_flame"), TEXT("Searing Light"), TEXT("Spell"), 2, HolyFlame, true, TEXT("Spell attack for 3d6 fire; undead can't regenerate this round.")),
		C(TEXT("quicken"), TEXT("Quicken"), TEXT("Tempo"), 0, Quicken, false, TEXT("Your next turn comes around much sooner on the timeline.")),
		C(TEXT("lucky"), TEXT("Lucky Coin"), TEXT("Tempo"), 0, LuckyCoin, false, TEXT("Fortune: your next d20 is rolled twice; keep the higher.")),
		C(TEXT("second_wind"), TEXT("Second Wind"), TEXT("Skill"), 1, SecondWind, false, TEXT("Catch your breath: heal 1d8 + twice your level.")),
		C(TEXT("insight"), TEXT("Battle Insight"), TEXT("Skill"), 1, Insight, false, TEXT("Read the fight. Draw two cards.")),
		C(TEXT("riposte"), TEXT("Riposte"), TEXT("Aura"), 1, Riposte, false, TEXT("Until your next turn, any foe that misses you takes your weapon damage.")),
	};
	return M;
}

void RegisterCard(const FCardDef& Card) { MutableCards().Add(Card.Id, Card); }

FString SpellCard(const FString& SpellName, int32 Rank)
{
	const FLibSpell* S = HollowLib::Spell(SpellName);
	if (!S) return FString();
	const int32 At = S->bCantrip ? Rank : FMath::Max(Rank, S->Rank);
	const FString Id = FString::Printf(TEXT("spell:%s:%d"), *HollowLib::Slug(S->Name), At);
	if (Cards().Contains(Id)) return Id;
	FCardDef D;
	D.Id = Id; D.Name = S->Name; D.Kind = TEXT("Spell"); D.Fx = ECardFx::LibSpell; D.Spell = S->Name; D.Rank = At;
	D.Cost = S->Actions();
	D.bExhaust = !S->bCantrip && !S->bFocus;
	D.bNeedsTarget = S->AreaType.IsEmpty() && !S->bHeal;
	const FString Sum = HollowLib::SpellSummary(*S, At);
	D.Text = FString::Printf(TEXT("%s %d. %s"), S->bCantrip ? TEXT("Cantrip") : S->bFocus ? TEXT("Focus") : TEXT("Rank"), At, Sum.IsEmpty() ? *S->Text.Left(90) : *Sum);
	// colour by what it does: healing gold, fire red, cold/water blue, mental/void violet, everything else arcane blue
	const FString Ty = S->Damage.Num() ? S->Damage[0].Value : FString();
	D.Col = S->bHeal ? Hex(TEXT("c9a227")) : Ty == TEXT("fire") ? Hex(TEXT("b8452f")) : Ty == TEXT("cold") ? Hex(TEXT("3f7fb8"))
		: Ty == TEXT("electricity") ? Hex(TEXT("c5b21e")) : (Ty == TEXT("mental") || Ty == TEXT("void")) ? Hex(TEXT("6a3fa0")) : Ty == TEXT("vitality") ? Hex(TEXT("d8c27a")) : Hex(TEXT("4b55a8"));
	RegisterCard(D);
	return Id;
}

TArray<FString> StarterDeck(const FString& Kit)
{
	if (Kit == TEXT("rogue"))
		return { TEXT("feint"), TEXT("feint"), TEXT("feint"), TEXT("tumble"), TEXT("tumble"), TEXT("double_slice"), TEXT("hamstring"), TEXT("quicken"), TEXT("lucky"), TEXT("insight") };
	if (Kit == TEXT("wizard"))
		return { TEXT("force_barrage"), TEXT("force_barrage"), TEXT("force_barrage"), TEXT("electric_arc"), TEXT("electric_arc"), TEXT("fear"), TEXT("shield_spell"), TEXT("shield_spell"), TEXT("insight"), TEXT("quicken") };
	if (Kit == TEXT("cleric"))
		return { TEXT("heal"), TEXT("heal"), TEXT("divine_lance"), TEXT("divine_lance"), TEXT("divine_lance"), TEXT("bless"), TEXT("bless"), TEXT("holy_flame"), TEXT("second_wind"), TEXT("lucky") };
	return { TEXT("power_attack"), TEXT("power_attack"), TEXT("double_slice"), TEXT("sudden_charge"), TEXT("intimidate"), TEXT("intimidate"), TEXT("riposte"), TEXT("second_wind"), TEXT("quicken"), TEXT("lucky") };
}

const TArray<FString>& RewardPool(const FString& Kit)
{
	static const TArray<FString> Shared = { TEXT("quicken"), TEXT("lucky"), TEXT("second_wind"), TEXT("insight"), TEXT("hamstring"), TEXT("battle_hymn") };
	static const TMap<FString, TArray<FString>> M = [&]
	{
		TMap<FString, TArray<FString>> R;
		R.Add(TEXT("fighter"), { TEXT("power_attack"), TEXT("double_slice"), TEXT("sudden_charge"), TEXT("intimidate"), TEXT("riposte") });
		R.Add(TEXT("rogue"), { TEXT("feint"), TEXT("tumble"), TEXT("double_slice"), TEXT("riposte"), TEXT("intimidate") });
		R.Add(TEXT("wizard"), { TEXT("force_barrage"), TEXT("electric_arc"), TEXT("fear"), TEXT("shield_spell") });
		R.Add(TEXT("cleric"), { TEXT("heal"), TEXT("divine_lance"), TEXT("bless"), TEXT("holy_flame"), TEXT("shield_spell") });
		for (auto& P : R) P.Value.Append(Shared);
		return R;
	}();
	return M.FindChecked(M.Contains(Kit) ? Kit : FString(TEXT("fighter")));
}

const TArray<FTopicDef>& Topics()
{
	auto T = [](const TCHAR* Id, const TCHAR* Label, TArray<FString> K) { FTopicDef D; D.Id = Id; D.Label = Label; D.Keywords = MoveTemp(K); return D; };
	static const TArray<FTopicDef> L = {
		T(TEXT("wolves"), TEXT("wolves"), { TEXT("wolf"), TEXT("wolves"), TEXT("howl"), TEXT("pack") }),
		T(TEXT("ridge"), TEXT("the ridge"), { TEXT("ridge"), TEXT("north"), TEXT("hills"), TEXT("mountain"), TEXT("den") }),
		T(TEXT("barrow"), TEXT("the barrow"), { TEXT("barrow"), TEXT("tomb"), TEXT("grave"), TEXT("burial"), TEXT("crypt"), TEXT("runes") }),
		T(TEXT("keep"), TEXT("the keep"), { TEXT("keep"), TEXT("castle"), TEXT("hollowmere"), TEXT("fortress"), TEXT("tower") }),
		T(TEXT("varek"), TEXT("Varek"), { TEXT("varek"), TEXT("lord"), TEXT("undead"), TEXT("vampire"), TEXT("hollow lord") }),
		T(TEXT("holy"), TEXT("holy water"), { TEXT("holy"), TEXT("blessed"), TEXT("flask") }),
		T(TEXT("rite"), TEXT("the rite"), { TEXT("rite"), TEXT("ritual"), TEXT("prayer"), TEXT("unbinding"), TEXT("chapel") }),
		T(TEXT("shrine"), TEXT("the shrine"), { TEXT("shrine"), TEXT("dawn"), TEXT("pilgrim"), TEXT("sun"), TEXT("temple") }),
		T(TEXT("river"), TEXT("the river"), { TEXT("river"), TEXT("ford"), TEXT("ferry"), TEXT("fish"), TEXT("lull"), TEXT("south") }),
		T(TEXT("bandits"), TEXT("bandits"), { TEXT("bandit"), TEXT("toll"), TEXT("road"), TEXT("thief"), TEXT("thieves"), TEXT("robber"), TEXT("west") }),
		T(TEXT("fen"), TEXT("the fen"), { TEXT("fen"), TEXT("swamp"), TEXT("bog"), TEXT("mire"), TEXT("marsh"), TEXT("lights"), TEXT("wisp") }),
		T(TEXT("rumors"), TEXT("rumors"), { TEXT("rumor"), TEXT("rumour"), TEXT("news"), TEXT("gossip"), TEXT("heard") }),
		T(TEXT("work"), TEXT("work"), { TEXT("work"), TEXT("job"), TEXT("quest"), TEXT("task"), TEXT("money"), TEXT("help") }),
		T(TEXT("fighting"), TEXT("fighting"), { TEXT("fight"), TEXT("combat"), TEXT("tips"), TEXT("train"), TEXT("battle") }),
		T(TEXT("cards"), TEXT("cards"), { TEXT("card"), TEXT("deck"), TEXT("hand") }),
		T(TEXT("town"), TEXT("Brindlemark"), { TEXT("brindlemark"), TEXT("town"), TEXT("village") }),
		T(TEXT("yourself"), TEXT("yourself"), { TEXT("yourself"), TEXT("who are you"), TEXT("your name"), TEXT("about you") }),
		T(TEXT("ysolde"), TEXT("Ysolde"), { TEXT("ysolde"), TEXT("sister"), TEXT("priestess") }),
		T(TEXT("maren"), TEXT("Maren"), { TEXT("maren"), TEXT("captain"), TEXT("guard") }),
		T(TEXT("tobb"), TEXT("Tobb"), { TEXT("tobb"), TEXT("innkeeper"), TEXT("tavern"), TEXT("inn") }),
		T(TEXT("pip"), TEXT("Pip"), { TEXT("pip"), TEXT("goblin"), TEXT("trader"), TEXT("shop") }),
	};
	return L;
}

const FTopicDef* FindTopic(const FString& Id)
{
	return Topics().FindByPredicate([&](const FTopicDef& T) { return T.Id == Id; });
}

const TArray<FNpcDef>& Npcs()
{
	auto R = [](const TCHAR* Text, const TCHAR* Lead = TEXT(""), int32 Gold = 0, int32 Rations = 0, int32 Holy = 0)
	{
		FNpcReply Rp; Rp.Text = Text; Rp.Lead = Lead; Rp.Gold = Gold; Rp.Rations = Rations; Rp.Holy = Holy; return Rp;
	};
	auto Opt = [](const TCHAR* Id, const TCHAR* Label, const TCHAR* Ok) { FTalkOption O; O.Id = Id; O.Label = Label; O.Ok = Ok; return O; };
	static const TArray<FNpcDef> L = [&]
	{
		TArray<FNpcDef> Out;
		{
			FNpcDef N; N.Id = TEXT("maren"); N.Name = TEXT("Maren Holt"); N.Role = TEXT("Retired gate-captain");
			N.Body = Hex(TEXT("2a3442")); N.Skin = Hex(TEXT("b08a6a")); N.Hat = Hex(TEXT("5d636b")); N.HatStyle = 1;
			N.Greeting = TEXT("Another sword for the March? Good. Ask me about the [wolves], the [ridge] or [fighting] if you want to live. The [keep] is a fool's errand.");
			N.Unknown = TEXT("Not my business. Ask Tobb at the tavern, he collects [rumors] like coin.");
			N.Replies.Add(TEXT("wolves"), R(TEXT("Grey wolves came down off the [ridge] this spring. The pack leader's clever. Hit it hard before it hits you twice.")));
			N.Replies.Add(TEXT("ridge"), R(TEXT("North-east of town the land climbs into the hills. Wolf dens, and an old [barrow] or two. I've marked the trail on your map. Scouting it will cost you."), TEXT("ridge")));
			N.Replies.Add(TEXT("barrow"), R(TEXT("There's a barrow north-west, door carved with runes. Read the runes right and the dead let you in. Read them wrong... I'll mark it."), TEXT("barrow")));
			N.Replies.Add(TEXT("fighting"), R(TEXT("Three actions a turn. Your second swing in a breath is worse than your first, the third worse still. Spend spare actions on your [cards], or a raised shield.")));
			N.Replies.Add(TEXT("keep"), R(TEXT("Varek's keep sits at the edge of the March. Don't go until you've got [holy] water and a few scars. Tobb can draw you the road, if you insist.")));
			N.Replies.Add(TEXT("cards"), R(TEXT("Every fighter has tricks. Pip sells new ones. A trick you can't afford to use is just paper.")));
			N.Replies.Add(TEXT("yourself"), R(TEXT("Thirty years on the gate. Knees gave out before my nerve did.")));
			N.Replies.Add(TEXT("town"), R(TEXT("Brindlemark's all lanterns and stubbornness. We don't run.")));
			N.Replies.Add(TEXT("work"), R(TEXT("Work? Clear the [wolves] off the ridge and the woodcutters will bless you. Or go poke the [barrow], if you want coin.")));
			{ FTalkOption O = Opt(TEXT("pelts"), TEXT("I've brought wolf pelts for the bounty."), TEXT("Grey ridge wolves, all right. Five silver a pelt, as the board says. Keep them coming."));
			  O.NeedItem = TEXT("pelt"); O.bGive = true; O.bGiveAll = true; O.Gold = 5; O.bOnce = false; N.Options.Add(O); }
			{ FTalkOption O = Opt(TEXT("charm"), TEXT("I found this bone charm in a cave."), TEXT("That's Odo's. He never took it off. If it was in a cave, he's in the [barrow] country now. Here. For bringing it home."));
			  O.NeedItem = TEXT("charm"); O.bGive = true; O.Gold = 20; O.Xp = 25; O.Lead = TEXT("barrow"); N.Options.Add(O); }
			{ FTalkOption O = Opt(TEXT("rope"), TEXT("Lend me your old climbing rope, Captain."), TEXT("Ha. You've a gate-guard's nerve. Take it, the grapnel too. Bring it back or don't come back."));
			  O.Skill = TEXT("intimidation"); O.DC = 15; O.Fail = TEXT("Ask me nicely, or ask Pip. That rope saw me up the north wall in the siege."); O.GiveItem = TEXT("rope"); N.Options.Add(O); }
			Out.Add(N);
		}
		{
			FNpcDef N; N.Id = TEXT("tobb"); N.Name = TEXT("Tobb Alderwick"); N.Role = TEXT("Keeper of the Sleeping Lantern"); N.Service = TEXT("inn");
			N.Body = Hex(TEXT("4a2616")); N.Skin = Hex(TEXT("c49a78")); N.Hat = Hex(TEXT("8c8478")); N.HatStyle = 0;
			N.Greeting = TEXT("Welcome to the Sleeping Lantern! A bed's five silver. Ask me for [rumors], or about the [river] road, or those [bandits].");
			N.Unknown = TEXT("Can't say I've heard of that. More ale?");
			N.Replies.Add(TEXT("rumors"), R(TEXT("They say [Varek] can't die while there's blood in his cellar. Pilgrims swear by the Dawn [shrine]. And lights in the [fen] lead folk off the path.")));
			N.Replies.Add(TEXT("river"), R(TEXT("The River Lull runs south-east. The ferryman's honest, the water isn't. I'll sketch the river road on your map."), TEXT("river")));
			N.Replies.Add(TEXT("bandits"), R(TEXT("Masked lot on the old road west. They call it a toll. I call it robbery with manners. Here's the road, if you want a word with them."), TEXT("road")));
			N.Replies.Add(TEXT("keep"), R(TEXT("You want the keep? Fine. The straightest road there runs right through the worst of it. I've drawn it. Don't thank me."), TEXT("keep")));
			N.Replies.Add(TEXT("varek"), R(TEXT("Lord here once. Drank something he shouldn't have. Now he won't stay dead. Sister [Ysolde] knows more than I do.")));
			N.Replies.Add(TEXT("fen"), R(TEXT("Mirefen. Some come back rich. Most come back wet. Pip knows the way, the little ghoul.")));
			N.Replies.Add(TEXT("ysolde"), R(TEXT("She keeps the temple. Kind woman. Terrifying in an argument.")));
			N.Replies.Add(TEXT("yourself"), R(TEXT("Tobb. Third Tobb to run the Lantern. The first one built the bar, the second one broke it.")));
			N.Replies.Add(TEXT("town"), R(TEXT("Best ale on the March. Only ale on the March, but still.")));
			N.Replies.Add(TEXT("work"), R(TEXT("The pilgrims lose people on the road. Folks pay well for news of the [shrine].")));
			{ FTalkOption O = Opt(TEXT("shovel"), TEXT("Your grandfather buried things, you said. Got his shovel?"), TEXT("The second Tobb buried half his debts in the hills. Take the shovel, and if you dig anything up, the first round's on you."));
			  O.Skill = TEXT("diplomacy"); O.DC = 14; O.Fail = TEXT("That shovel stays with the family. Pip sells shovels, if you're set on grave-robbing."); O.GiveItem = TEXT("shovel"); N.Options.Add(O); }
			{ FTalkOption O = Opt(TEXT("odo"), TEXT("What happened to Odo Tanner?"), TEXT("Walked out north-east, humming, a week back. Wore a bone charm on red thread. If you find that charm, Maren will want it."));
			  O.Xp = 5; N.Options.Add(O); }
			N.Stock = { TEXT("ration"), TEXT("torch"), TEXT("lantern") };
			Out.Add(N);
		}
		{
			FNpcDef N; N.Id = TEXT("ysolde"); N.Name = TEXT("Sister Ysolde"); N.Role = TEXT("Keeper of the Starfall Altar"); N.Service = TEXT("temple");
			N.Body = Hex(TEXT("3d2a52")); N.Skin = Hex(TEXT("a87452")); N.Hat = Hex(TEXT("9c6cff")); N.HatStyle = 3;
			N.Greeting = TEXT("The Dawn keep you. Ask what troubles you: the [shrine], the [rite], or [holy] water.");
			N.Unknown = TEXT("The Dawn has not shown me that.");
			N.Replies.Add(TEXT("shrine"), R(TEXT("Pilgrims walk to the Dawn Shrine out in the wilds. Pray there and the sun mends you. I'll show you the pilgrim road."), TEXT("shrine")));
			N.Replies.Add(TEXT("rite"), R(TEXT("The rite of unbinding. Speak it at [Varek]'s own gate and his stolen life bleeds out. It needs a devout tongue.")));
			N.Replies.Add(TEXT("holy"), R(TEXT("It burns the dead and stops Varek's wounds from knitting. Take this flask freely. More are ten silver."), TEXT(""), 0, 0, 1));
			N.Replies.Add(TEXT("varek"), R(TEXT("He fed on his own people to cheat death. The [rite] can undo what he did, if you reach his [keep].")));
			N.Replies.Add(TEXT("keep"), R(TEXT("Go with the Dawn, or don't go.")));
			N.Replies.Add(TEXT("yourself"), R(TEXT("I came for a season. That was eleven years ago.")));
			N.Replies.Add(TEXT("town"), R(TEXT("They're good people. Frightened people, which is not the same as weak.")));
			{ FTalkOption O = Opt(TEXT("rite"), TEXT("Teach me the words of the unbinding."), TEXT("Then listen, and don't stumble. The Dawn hears the stumbles. Go, and carry its blessing with you."));
			  O.Skill = TEXT("religion"); O.DC = 15; O.Fail = TEXT("Not yet. You'd say it like a shopping list. Come back when you've prayed at the [shrine]."); O.Xp = 20; N.Options.Add(O); }
			{ FTalkOption O = Opt(TEXT("shard"), TEXT("I found a shard of violet glass."), TEXT("Give it here. Quickly. Don't look at it too long. The altar will keep it safe. Take this for your trouble, and say nothing."));
			  O.NeedItem = TEXT("shard"); O.bGive = true; O.Gold = 25; O.Xp = 10; O.bOnce = false; N.Options.Add(O); }
			N.Stock = { TEXT("holy"), TEXT("potion") };
			Out.Add(N);
		}
		{
			FNpcDef N; N.Id = TEXT("pip"); N.Name = TEXT("Pip Quickfingers"); N.Role = TEXT("Goblin trader"); N.Service = TEXT("shop");
			N.Body = Hex(TEXT("33213f")); N.Skin = Hex(TEXT("5a8a36")); N.Hat = Hex(TEXT("6a4a1e")); N.HatStyle = 2;
			N.Greeting = TEXT("Shiny hero! Pip sells [cards], potions, rations. Pip knows the [fen] too, oh yes.");
			N.Unknown = TEXT("Pip does not know. Pip could sell you something instead?");
			N.Replies.Add(TEXT("cards"), R(TEXT("Fight-cards! Each turn you draw a hand. Cards cost the same three actions you swing with. Buy a pack, pick the best of three!")));
			N.Replies.Add(TEXT("fen"), R(TEXT("South-west, past the reeds. Lights in the bog! Follow carefully, find treasure. Pip draws the map, yes, yes."), TEXT("fen")));
			N.Replies.Add(TEXT("wolves"), R(TEXT("Wolves eat goblins. Pip does not go north.")));
			N.Replies.Add(TEXT("yourself"), R(TEXT("Pip is Pip. Pip is a businessman.")));
			N.Replies.Add(TEXT("bandits"), R(TEXT("Pip pays the toll. Pip is smart. Pip also sells them things. Ha!")));
			N.Replies.Add(TEXT("town"), R(TEXT("Good town. Nobody here has eaten Pip. Yet.")));
			{ FTalkOption O = Opt(TEXT("bag"), TEXT("That big sack you're sitting on. What's in it?"), TEXT("Nothing! Everything! It is a very big sack, shiny hero. Bigger inside. Pip sells it, if you have the silver."));
			  O.Skill = TEXT("perception"); O.DC = 13; O.Fail = TEXT("Sack? What sack? Pip sits on nothing. Buy something."); O.Xp = 5; N.Options.Add(O); }
			{ FTalkOption O = Opt(TEXT("discount"), TEXT("Pip, we both know that potion's watered."), TEXT("Pip is wounded! Wounded. Fine. Have one, and tell no one Pip has a heart."));
			  O.Skill = TEXT("deception"); O.DC = 16; O.Fail = TEXT("Pip's potions are pure! Purer than you. Rude."); O.GiveItem = TEXT("potion"); N.Options.Add(O); }
			N.Stock = { TEXT("potion"), TEXT("ration"), TEXT("torch"), TEXT("tinderbox"), TEXT("rope"), TEXT("hatchet"), TEXT("shovel"), TEXT("picks"), TEXT("lantern"), TEXT("bag") };
			Out.Add(N);
		}
		return Out;
	}();
	return L;
}

const FNpcDef* FindNpc(const FString& Id)
{
	return Npcs().FindByPredicate([&](const FNpcDef& N) { return N.Id == Id; });
}

const TArray<FLeadDef>& Leads()
{
	using enum ETerr;
	auto D = [](const TCHAR* Id, const TCHAR* Name, TArray<ETerr> Prefer, int32 Dir, int32 Ring)
	{
		FLeadDef L; L.Id = Id; L.Name = Name; L.Prefer = MoveTemp(Prefer); L.Dir = Dir; L.Ring = Ring; return L;
	};
	static const TArray<FLeadDef> L = {
		D(TEXT("ridge"), TEXT("The Wolf Ridge"), { Hills, Forest }, 1, 2),
		D(TEXT("barrow"), TEXT("The Old Barrow"), { Hills }, 2, 3),
		D(TEXT("river"), TEXT("River Road"), { River }, 5, 2),
		D(TEXT("road"), TEXT("The Old Road"), { Plains }, 3, 2),
		D(TEXT("fen"), TEXT("Mirefen"), { Swamp }, 4, 3),
		D(TEXT("shrine"), TEXT("Pilgrim Road"), { Shrine }, 0, 2),
		D(TEXT("keep"), TEXT("Hollowmere Keep"), { Keep }, 0, 3),
	};
	return L;
}

const FLeadDef* FindLead(const FString& Id)
{
	return Leads().FindByPredicate([&](const FLeadDef& L) { return L.Id == Id; });
}


const TMap<FString, TPair<FString, FString>>& Lore()
{
	static const TMap<FString, TPair<FString, FString>> L = {
		{ TEXT("monolith"), { TEXT("The Fallen Shard"), TEXT("A slab of black stone that is warm to the touch. The villagers say it fell from the sky the night Varek's lord-father died. Violet light crawls along runes no mason carved. If you hold your ear close, it hums in a voice that sounds like your own, counting down.") } },
		{ TEXT("fountain"), { TEXT("The Tentacle Fountain"), TEXT("The fountain was a saint once. Last winter the stone began to coil. Now a single grey tentacle rises from the basin and cradles an orb of violet light, and the water glows the same colour. Children say it turns to watch them. The Choir comes at night to drink from it.") } },
		{ TEXT("notice"), { TEXT("Notice Board"), TEXT("WANTED: wolf pelts, 5 silver each, ask Maren.  MISSING: Odo Tanner, last seen walking north-east humming.  BY ORDER OF THE ALTAR: no prayers after moonrise.  Someone has scratched a spiral into every notice.") } },
		{ TEXT("temple"), { TEXT("Temple of the Sleeping Star"), TEXT("Once a chapel of the Dawn. Now the sun-disc above the door has been chiselled into a many-pointed star, and the hymnals inside have their pages glued shut. Sister Ysolde says the old faith is still here, underneath. She doesn't sound sure.") } },
		{ TEXT("graves"), { TEXT("Fresh Graves"), TEXT("Seven new graves, all dug the same night. Every headstone faces upward instead of east. The soil on top has been raked into neat concentric rings.") } },
		{ TEXT("crypt"), { TEXT("Family Crypt"), TEXT("The Alderwick crypt. The iron door is chained from the inside.") } },
		{ TEXT("tavern"), { TEXT("The Sleeping Lantern"), TEXT("Warm light, cheap ale, and a fire nobody lets go out. Above the bar hangs a lantern that has burned for ninety years without oil. Tobb says it's the only reason the town still stands.") } },
		{ TEXT("arch"), { TEXT("Ruined Arch"), TEXT("A fire burns in the brazier beneath the arch. No one tends it. The flames lean toward the sky.") } },
		{ TEXT("circle"), { TEXT("Ritual Circle"), TEXT("Three rings of inlaid stone, the outermost glowing faintly. Burnt candle stubs, a child's shoe, and a star chart drawn in what you hope is rust.") } },
		{ TEXT("wagon"), { TEXT("Abandoned Wagon"), TEXT("A pilgrim's wagon, one wheel snapped. The cargo was prayer candles, thousands of them, every one burned down to the wick.") } },
		{ TEXT("bones"), { TEXT("Bleached Bones"), TEXT("Something large died here and was picked clean. The ribs are scored with tiny, careful marks, like writing.") } },
		{ TEXT("deadtree"), { TEXT("Gnarled Oak"), TEXT("The bark is cold. Where the branches split, the wood has grown into the shape of reaching hands.") } },
		{ TEXT("sky"), { TEXT("The Sleeping Star"), TEXT("It hangs over the March day and night, a little closer every year. The cult in the old temple prays it will wake and fall. Everyone else prays they're wrong.") } },
	};
	return L;
}

}
