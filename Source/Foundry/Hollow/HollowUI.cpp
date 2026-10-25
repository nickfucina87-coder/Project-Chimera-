#include "HollowUI.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformTime.h"
#include "HollowGameMode.h"
#include "HollowRules.h"

using namespace HollowRules;

namespace
{
	FLinearColor C(uint32 Rgb, float A = 1.f)
	{
		FLinearColor L = FLinearColor::FromSRGBColor(FColor((Rgb >> 16) & 255, (Rgb >> 8) & 255, Rgb & 255));
		L.A = A;
		return L;
	}
	const FLinearColor Fg = C(0xECE3D3), Dim = C(0xA497AE), Gold = C(0xD8A94F), Moss = C(0x9CC28A), Blood = C(0xF08A7E);
	const FLinearColor PanelC = C(0x0E0B12, 0.93f), Panel2 = C(0x1E1826), Line = C(0x3B3047);
	TWeakObjectPtr<AHollowGameMode> GUiGM;
	const FLinearColor Parch = C(0xEADBBD), ParchInk = C(0x2A1C12), ParchDim = C(0x6D5440), Wine = C(0x3A1712);

	const FSlateBrush* TexBrush(const FString& Path, ESlateBrushDrawType::Type Draw, float Margin)
	{
		static TMap<FString, FSlateBrush*> Cache;
		if (FSlateBrush** Found = Cache.Find(Path)) return *Found;
		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path);
		if (!Tex) return nullptr;
		Tex->AddToRoot();
		FSlateBrush* B = new FSlateBrush();
		B->SetResourceObject(Tex);
		B->ImageSize = FVector2D(Tex->GetSizeX(), Tex->GetSizeY());
		B->DrawAs = Draw;
		B->Margin = FMargin(Margin);
		Cache.Add(Path, B);
		return B;
	}
	const FSlateBrush* Frame() { return TexBrush(TEXT("/Game/Hollow/UI/Borders/panel-border-010.panel-border-010"), ESlateBrushDrawType::Box, 0.4f); }
	// Elemental frames sliced by UV from the 10x5 card-frame sheet (Docs/Concept/card_frames_sheet.png).
	const FSlateBrush* CardFrame(const FString& Kind)
	{
		static TMap<FString, FSlateBrush*> Cache;
		if (FSlateBrush** Found = Cache.Find(Kind)) return *Found;
		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Hollow/UI/T_CardFrames.T_CardFrames"));
		if (!Tex) { Cache.Add(Kind, nullptr); return nullptr; }
		Tex->AddToRoot();
		static const TMap<FString, FIntPoint> Cells = {
			{ TEXT("Attack"), FIntPoint(2, 3) }, { TEXT("Skill"), FIntPoint(3, 2) }, { TEXT("Spell"), FIntPoint(2, 0) },
			{ TEXT("Tempo"), FIntPoint(7, 0) }, { TEXT("Aura"), FIntPoint(8, 3) } };
		const FIntPoint Cell = Cells.Contains(Kind) ? Cells[Kind] : FIntPoint(0, 1);
		FSlateBrush* B = new FSlateBrush();
		B->SetResourceObject(Tex);
		B->DrawAs = ESlateBrushDrawType::Image;
		B->ImageSize = FVector2D(150, 200);
		B->SetUVRegion(FBox2f(FVector2f(Cell.X / 10.f + 0.004f, Cell.Y / 5.f + 0.006f), FVector2f((Cell.X + 1) / 10.f - 0.004f, (Cell.Y + 1) / 5.f - 0.006f)));
		Cache.Add(Kind, B);
		return B;
	}
	const FSlateBrush* Sprite(const FString& Name) { return TexBrush(FString::Printf(TEXT("/Game/Hollow/FX/Sprites/%s.%s"), *Name, *Name), ESlateBrushDrawType::Image, 0.f); }
	const FSlateBrush* Round()
	{
		static FSlateRoundedBoxBrush B(FLinearColor::White, 6.f);
		return &B;
	}
	const FButtonStyle& BtnStyle()
	{
		static FButtonStyle S = []
		{
			FButtonStyle X;
			X.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.82f, 0.82f, 0.82f), 5.f));
			X.SetHovered(FSlateRoundedBoxBrush(FLinearColor::White, 5.f));
			X.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.65f, 0.65f, 0.65f), 5.f));
			X.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.4f, 0.4f, 0.4f, 0.5f), 5.f));
			X.SetNormalPadding(FMargin(0)); X.SetPressedPadding(FMargin(0));
			return X;
		}();
		return S;
	}
	FSlateFontInfo Font(int32 Size, bool bBold = false) { return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size); }

	TSharedRef<STextBlock> T(const FString& S, int32 Size = 14, const FLinearColor& Col = Fg, bool bBold = false, bool bWrap = true)
	{
		return SNew(STextBlock).Text(FText::FromString(S)).Font(Font(Size, bBold)).ColorAndOpacity(FSlateColor(Col)).AutoWrapText(bWrap);
	}
	TSharedRef<SWidget> Panel(const TSharedRef<SWidget>& Content, const FLinearColor& Bg = PanelC, const FMargin& Pad = FMargin(14), bool bFrame = true)
	{
		TSharedRef<SOverlay> O = SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(Bg).Padding(Pad)[Content]];
		if (bFrame && Frame())
			O->AddSlot()[SNew(SBorder).BorderImage(Frame()).BorderBackgroundColor(C(0x8C6A36, 0.7f)).Visibility(EVisibility::HitTestInvisible)];
		return O;
	}
	TSharedRef<SWidget> Btn(const FString& Label, TFunction<void()> OnClick, bool bEnabled = true, const FLinearColor& Col = Panel2,
		const FLinearColor& TextCol = Fg, int32 Size = 14)
	{
		return SNew(SButton)
			.ButtonStyle(&BtnStyle())
			.ButtonColorAndOpacity(Col)
			.IsEnabled(bEnabled)
			.ContentPadding(FMargin(12, 7))
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([OnClick] { if (GUiGM.IsValid()) GUiGM->PlaySfx(TEXT("click"), 0.6f); OnClick(); return FReply::Handled(); })
			[T(Label, Size, TextCol, true, false)];
	}
	TSharedRef<SWidget> Bar(float Pct, const FLinearColor& Fill, const FString& Label, float Width = 220.f)
	{
		return SNew(SBox).WidthOverride(Width).HeightOverride(22.f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()[SNew(SProgressBar).Percent(FMath::Clamp(Pct, 0.f, 1.f)).FillColorAndOpacity(Fill).BackgroundImage(Round())]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[T(Label, 12, FLinearColor::White, true, false)]
			];
	}
	TSharedRef<SWidget> Stat(const FString& Label, const FString& Value)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[T(Label, 10, Dim, true, false)]
			+ SVerticalBox::Slot().AutoHeight()[T(Value, 16, Fg, true, false)];
	}
	TSharedRef<SWidget> Pips(int32 Have, int32 Max)
	{
		TSharedRef<SHorizontalBox> H = SNew(SHorizontalBox);
		for (int32 i = 0; i < Max; ++i)
			H->AddSlot().AutoWidth().Padding(2, 0)
			[
				SNew(SBox).WidthOverride(16).HeightOverride(16)[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(i < Have ? Gold : Line)]
			];
		return H;
	}
	TSharedRef<SWidget> Dimmer(const TSharedRef<SWidget>& Content)
	{
		return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(FLinearColor(0.02f, 0.01f, 0.03f, 0.6f))
			.HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(16)[Content];
	}
}

/* ---------- Animated cards ---------- */

namespace
{
	TMap<int32, double> GCardBorn;
	float EaseOutBack(float X)
	{
		const float C1 = 1.70158f, C3 = C1 + 1.f;
		X = FMath::Clamp(X, 0.f, 1.f);
		return 1.f + C3 * FMath::Pow(X - 1.f, 3.f) + C1 * FMath::Pow(X - 1.f, 2.f);
	}
	FString KindSprite(const FString& Kind)
	{
		if (Kind == TEXT("Attack")) return TEXT("slash_02");
		if (Kind == TEXT("Spell")) return TEXT("magic_03");
		if (Kind == TEXT("Tempo")) return TEXT("twirl_02");
		if (Kind == TEXT("Aura")) return TEXT("star_07");
		return TEXT("symbol_01");
	}
}

// A card that slides into the hand, lifts on hover, and keeps its sprite art drifting behind a passing shimmer.
class SHollowCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHollowCard) : _Scale(1.f), _Serial(-1), _Index(0), _Enabled(true), _Card(nullptr) {}
		SLATE_ARGUMENT(float, Scale)
		SLATE_ARGUMENT(int32, Serial)
		SLATE_ARGUMENT(int32, Index)
		SLATE_ARGUMENT(bool, Enabled)
		SLATE_ARGUMENT(const FCardDef*, Card)
		SLATE_ARGUMENT(TFunction<void()>, OnPlay)
	SLATE_END_ARGS()

	void Construct(const FArguments& A)
	{
		const FCardDef& Cd = *A._Card;
		const float S = A._Scale;
		Index = A._Index;
		const double NowT = FPlatformTime::Seconds();
		Born = A._Serial >= 0 ? GCardBorn.FindOrAdd(A._Serial, NowT) : NowT - 10.0;
		TFunction<void()> Play = A._OnPlay;
		TSharedRef<SHorizontalBox> Cost = SNew(SHorizontalBox);
		for (int32 i = 0; i < Cd.Cost; ++i)
			Cost->AddSlot().AutoWidth().Padding(1, 0)[SNew(SBox).WidthOverride(11 * S).HeightOverride(11 * S)[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(Gold)]];
		if (Cd.Cost == 0) Cost->AddSlot().AutoWidth()[T(TEXT("free"), 9, Gold, true, false)];
		const FSlateBrush* ArtB = Sprite(KindSprite(Cd.Kind));
		const FSlateBrush* ShineB = Sprite(TEXT("light_02"));
		SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		ChildSlot
		[
			SNew(SBox).WidthOverride(150 * S).HeightOverride(208 * S)
			[
				SNew(SButton).ButtonStyle(&BtnStyle()).ButtonColorAndOpacity(C(0x15101A)).IsEnabled(A._Enabled).ContentPadding(FMargin(0))
				.OnClicked_Lambda([Play] { if (Play) Play(); return FReply::Handled(); })
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[CardFrame(Cd.Kind) ? StaticCastSharedRef<SWidget>(SNew(SImage).Image(CardFrame(Cd.Kind))) : StaticCastSharedRef<SWidget>(SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(C(0x15101A)))]
					+ SOverlay::Slot().Padding(16 * S, 17 * S, 16 * S, 15 * S)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(78 * S)
							[
								SNew(SOverlay)
								+ SOverlay::Slot()[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(FLinearColor(0.01f, 0.008f, 0.015f, 0.55f))]
								+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
								[
									SAssignNew(Art, SImage).Image(ArtB).ColorAndOpacity(Cd.Col * 1.8f).DesiredSizeOverride(FVector2D(88 * S, 88 * S))
								]
								+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center)
								[
									SAssignNew(Shine, SImage).Image(ShineB).ColorAndOpacity(FLinearColor(1.f, 0.95f, 0.85f, 0.35f)).DesiredSizeOverride(FVector2D(56 * S, 110 * S)).Visibility(EVisibility::HitTestInvisible)
								]
								+ SOverlay::Slot().VAlign(VAlign_Top).HAlign(HAlign_Left).Padding(5)[T(Cd.Kind.ToUpper(), 8, FLinearColor::White, true, false)]
							]
						]
						+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, 4 * S, 0, 0)
						[
							SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(FLinearColor(0.01f, 0.008f, 0.015f, 0.72f)).Padding(FMargin(6 * S, 4 * S))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()[T(Cd.Name, 12, Fg, true)]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 1, 0, 2)[Cost]
								+ SVerticalBox::Slot().FillHeight(1.f)[T(Cd.Text, 8, Dim)]
							]
						]
					]
					+ SOverlay::Slot()[SNew(SBorder).BorderImage(Frame() ? Frame() : Round()).BorderBackgroundColor(CardFrame(Cd.Kind) ? FLinearColor::Transparent : Cd.Col * 0.6f + C(0x9A7A3C, 0.6f)).Visibility(EVisibility::HitTestInvisible)]
				]
			]
		];
	}

	virtual void Tick(const FGeometry& G, const double CurrentTime, const float Dt) override
	{
		SCompoundWidget::Tick(G, CurrentTime, Dt);
		const double NowT = FPlatformTime::Seconds();
		const float In = EaseOutBack(float((NowT - Born - Index * 0.08) / 0.5));
		HoverT = FMath::FInterpTo(HoverT, IsHovered() ? 1.f : 0.f, Dt, 12.f);
		const float Bob = FMath::Sin(float(NowT) * 1.8f + Index) * 3.f;
		const float Y = (1.f - In) * 260.f - HoverT * 26.f + Bob * (1.f - HoverT);
		SetRenderTransform(FSlateRenderTransform(FScale2D(1.f + HoverT * 0.08f), FVector2D(0.f, Y)));
		SetRenderOpacity(FMath::Clamp(In, 0.f, 1.f));
		if (Art.IsValid())
			Art->SetRenderTransform(FSlateRenderTransform(FScale2D(1.f + HoverT * 0.12f), FVector2D(FMath::Sin(float(NowT) * 0.7f + Index) * 6.f, HoverT * 8.f)));
		if (Shine.IsValid())
		{
			const float Sweep = FMath::Fmod(float(NowT) * (0.35f + HoverT * 0.8f) + Index * 0.3f, 1.6f);
			Shine->SetRenderTransform(FSlateRenderTransform(FVector2D(-60.f + Sweep * 160.f, 0.f)));
			Shine->SetRenderOpacity(0.25f + HoverT * 0.6f);
		}
	}

private:
	TSharedPtr<SImage> Art, Shine;
	double Born = 0.0;
	float HoverT = 0.f;
	int32 Index = 0;
};

// The card you just played, flying up off the hand and fading.
class SHollowFly : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHollowFly) : _Start(0.0) {}
		SLATE_ARGUMENT(TSharedPtr<SWidget>, Content)
		SLATE_ARGUMENT(double, Start)
	SLATE_END_ARGS()

	void Construct(const FArguments& A)
	{
		Start = A._Start;
		SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		SetVisibility(EVisibility::HitTestInvisible);
		ChildSlot[A._Content.ToSharedRef()];
	}

	virtual void Tick(const FGeometry& G, const double CurrentTime, const float Dt) override
	{
		SCompoundWidget::Tick(G, CurrentTime, Dt);
		const float U = GUiGM.IsValid() ? FMath::Clamp(float(GUiGM->Now() - Start) / 0.85f, 0.f, 1.f) : 1.f;
		const float E = 1.f - FMath::Pow(1.f - U, 3.f);
		SetRenderTransform(FSlateRenderTransform(FScale2D(1.15f - 0.5f * E), FVector2D(0.f, -340.f * E)));
		SetRenderOpacity(1.f - U);
	}

private:
	double Start = 0.0;
};

void SHollowUI::Construct(const FArguments& InArgs)
{
	GM = InArgs._GM;
	Rebuild();
}

void SHollowUI::Rebuild()
{
	AHollowGameMode* G = GM.Get();
	if (!G) return;
	GUiGM = G;
	AskBox.Reset();

	TSharedRef<SOverlay> Root = SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible);
	TSharedPtr<SWidget> Main;
	switch (G->Mode)
	{
	case EHollowMode::Title: Main = BuildTitle(); break;
	case EHollowMode::Create: Main = BuildCreate(); break;
	case EHollowMode::Explore: Main = BuildExplore(); break;
	case EHollowMode::Map: Main = BuildMap(); break;
	case EHollowMode::Combat: Main = BuildCombat(); break;
	case EHollowMode::End: Main = BuildEnd(); break;
	}
	Root->AddSlot()[Main.ToSharedRef()];

	if (G->Mode != EHollowMode::End)
	{
		if (G->Note.IsSet()) Root->AddSlot()[Dimmer(BuildNote())];
		else if (G->Event.IsSet()) Root->AddSlot()[Dimmer(BuildEvent())];
		else if (G->Reward.IsSet()) Root->AddSlot()[Dimmer(BuildReward())];
		else if (G->Talk.IsSet()) Root->AddSlot().VAlign(VAlign_Bottom).HAlign(HAlign_Center).Padding(16, 16, 16, 24)[BuildTalk()];
		if (G->bSheetOpen) Root->AddSlot()[Dimmer(BuildSheet())];
		else if (G->bDeckOpen) Root->AddSlot()[Dimmer(BuildDeck())];
	}

	TWeakObjectPtr<AHollowGameMode> W = G;
	Root->AddSlot().VAlign(VAlign_Top).HAlign(HAlign_Center).Padding(0, 92, 0, 0)
	[
		SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(Gold).Padding(FMargin(16, 8))
		.Visibility_Lambda([W] { return W.IsValid() && W->GetWorld() && W->GetWorld()->GetTimeSeconds() < W->ToastUntil ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		[
			SNew(STextBlock).Font(Font(15, true)).ColorAndOpacity(FSlateColor(C(0x1B1206)))
			.Text_Lambda([W] { return FText::FromString(W.IsValid() ? W->ToastText : FString()); })
		]
	];

	ChildSlot[Root];

	if (AskBox.IsValid())
		FSlateApplication::Get().SetKeyboardFocus(AskBox, EFocusCause::SetDirectly);
	else if (G->Mode != EHollowMode::Create && G->Mode != EHollowMode::Title)
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

/* ---------- Create ---------- */

TSharedRef<SWidget> SHollowUI::BuildTitle()
{
	AHollowGameMode* G = GM.Get();
	auto MenuBtn = [&](const FString& Label, TFunction<void()> Fn, bool bEnabled)
	{
		return SNew(SBox).WidthOverride(340).Padding(FMargin(0, 5))
		[
			SNew(SButton).ButtonStyle(&BtnStyle()).ButtonColorAndOpacity(C(0x120C16, 0.82f)).IsEnabled(bEnabled).ContentPadding(FMargin(16, 11)).HAlign(HAlign_Center)
			.OnClicked_Lambda([Fn] { if (GUiGM.IsValid()) GUiGM->PlaySfx(TEXT("select"), 0.7f); Fn(); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(Font(18, true)).ColorAndOpacity(FSlateColor(bEnabled ? C(0xF0C27A) : C(0x6A5A50)))
			]
		];
	};
	FSlateFontInfo TitleFont = Font(96, true);
	TitleFont.LetterSpacing = 260;
	FSlateFontInfo SubFont = Font(15, true);
	SubFont.LetterSpacing = 600;
	return SNew(SOverlay)
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(420).Visibility(EVisibility::HitTestInvisible)
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.45f))
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("EMBERFALL"))).Font(TitleFont).ColorAndOpacity(FSlateColor(C(0xF2A54A)))
				.ShadowOffset(FVector2D(0, 4)).ShadowColorAndOpacity(FLinearColor(0.35f, 0.05f, 0.02f, 0.9f))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 6)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("THE MARCH OF HOLLOWMERE"))).Font(SubFont).ColorAndOpacity(FSlateColor(C(0xB79CDB)))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 34)
			[
				T(TEXT("The Star is falling. In the old temple, they are calling it down."), 14, C(0xC9B8A6), false, false)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[MenuBtn(TEXT("New Game"), [G] { G->NewGame(); }, true)]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[MenuBtn(TEXT("Continue"), [] {}, false)]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[MenuBtn(TEXT("Settings"), [] {}, false)]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[MenuBtn(TEXT("Quit"), [G] { G->QuitToDesktop(); }, true)]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(24)
		[
			T(G->bBrainOnline ? FString::Printf(TEXT("prototype  -  NPC voices: %s"), *G->BrainModel) : FString(TEXT("prototype  -  NPC voices: response library")), 11, C(0x7A6A80), false, false)
		];
}

TSharedRef<SWidget> SHollowUI::BuildCreate()
{
	AHollowGameMode* G = GM.Get();
	const FLinearColor Ink = ParchInk, InkDim = ParchDim, Sel = C(0x5A1A14), Unsel = C(0xE6D6B4);
	auto Opt = [&](const FString& Name, const FString& Note, bool bSel, float W, TFunction<void()> Fn)
	{
		return SNew(SBox).WidthOverride(W)
		[
			SNew(SButton).ButtonStyle(&BtnStyle()).ButtonColorAndOpacity(bSel ? Sel : Unsel).ContentPadding(FMargin(10, 7))
			.OnClicked_Lambda([Fn] { if (GUiGM.IsValid()) GUiGM->PlaySfx(TEXT("click"), 0.6f); Fn(); return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[T(Name, 15, bSel ? C(0xF6E7C8) : Ink, true)]
				+ SVerticalBox::Slot().AutoHeight()[T(Note, 9, bSel ? C(0xD8B98A) : InkDim)]
			]
		];
	};
	TSharedRef<SWrapBox> Anc = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6, 6));
	for (const FAncestry& A : Ancestries())
	{
		const FString Id = A.Id;
		Anc->AddSlot()[Opt(A.Name, A.Note, G->CreateAnc == A.Id, 158, [G, Id] { G->CreateAnc = Id; G->MarkUI(); })];
	}
	TSharedRef<SWrapBox> Kit = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6, 6));
	for (const FKit& K : Kits())
	{
		const FString Id = K.Id;
		Kit->AddSlot()[Opt(K.Name, K.Note, G->CreateKit == K.Id, 240, [G, Id] { G->CreateKit = Id; G->MarkUI(); })];
	}
	const FPC P = BuildPC(G->CreateName, G->CreateAnc, G->CreateKit);
	TMap<FString, int32> Counts;
	for (const FString& Id : P.Deck) Counts.FindOrAdd(Id)++;
	FString DeckLine;
	for (auto& Pr : Counts) if (const FCardDef* Cd = HollowData::Cards().Find(Pr.Key)) DeckLine += FString::Printf(TEXT("%s%s x%d"), DeckLine.IsEmpty() ? TEXT("") : TEXT(",  "), *Cd->Name, Pr.Value);
	auto InkStat = [&](const FString& L, const FString& V)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[T(L, 9, InkDim, true, false)]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[T(V, 20, Ink, true, false)];
	};
	auto Label = [&](const FString& S) { return T(S, 11, C(0x5A2A1A), true, false); };

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(28, 24)
		[
			SNew(SBox).WidthOverride(580)
			[
				Panel(SNew(SScrollBox) + SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[T(TEXT("HOLLOWMERE"), 44, C(0x2A1610), true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)[T(TEXT("A lantern-lit village, a march of wild hexes, and a keep where something old refuses to die. Above it all, the Star is falling. Talk to open roads. Fight with steel, d20s and cards."), 12, InkDim)]
					+ SVerticalBox::Slot().AutoHeight()[Label(TEXT("NAME"))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 10)
					[
						SNew(SEditableTextBox).Text(FText::FromString(G->CreateName)).Font(Font(16)).BackgroundColor(C(0xF4E9D2)).ForegroundColor(Ink)
						.OnTextChanged_Lambda([G](const FText& Tx) { G->CreateName = Tx.ToString(); })
						.OnTextCommitted_Lambda([G](const FText& Tx, ETextCommit::Type) { G->CreateName = Tx.ToString(); G->MarkUI(); })
					]
					+ SVerticalBox::Slot().AutoHeight()[Label(TEXT("ANCESTRY"))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 10)[Anc]
					+ SVerticalBox::Slot().AutoHeight()[Label(TEXT("CLASS"))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 10)[Kit]
					+ SVerticalBox::Slot().AutoHeight()[Label(TEXT("STATS"))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 6)
					[
						SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(C(0xD9C7A2)).Padding(FMargin(10, 6))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f)[InkStat(TEXT("HP"), FString::FromInt(P.HpMax))]
							+ SHorizontalBox::Slot().FillWidth(1.f)[InkStat(TEXT("AC"), FString::FromInt(AC(P)))]
							+ SHorizontalBox::Slot().FillWidth(1.f)[InkStat(TEXT("ATTACK"), Sgn(AtkMod(P)))]
							+ SHorizontalBox::Slot().FillWidth(1.4f)[InkStat(TEXT("PERCEPTION"), Sgn(SkillMod(P, TEXT("perception"))))]
							+ SHorizontalBox::Slot().FillWidth(1.f)[InkStat(TEXT("SPEED"), FString::Printf(TEXT("%d ft"), P.Speed))]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 2)[T(FString::Printf(TEXT("Starting gear: %s, %s, adventurer's pack, %d rations, a healing potion."), *P.WeaponName, *P.ArmorName, P.Rations), 11, Ink)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)[T(TEXT("Starter deck: ") + DeckLine, 10, InkDim)]
					+ SVerticalBox::Slot().AutoHeight()[Btn(TEXT("Begin in Brindlemark"), [G] { G->BeginAdventure(); }, true, Wine, C(0xF6E7C8), 17)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)[T(TEXT("WASD or click to walk.  E to talk or examine.  Mouse wheel to zoom.  M for the map.  C for your sheet."), 10, InkDim)]
				], Parch, FMargin(26, 22))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)];
}

/* ---------- HUD ---------- */

TSharedRef<SWidget> SHollowUI::BuildHud()
{
	AHollowGameMode* G = GM.Get();
	const FPC& P = G->PC;
	const int32 NextXp = XpForLevel(P.Level + 1);
	return Panel(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 16, 0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[T(P.Name, 18, Gold, true, false)]
			+ SVerticalBox::Slot().AutoHeight()[T(FString::Printf(TEXT("%s %s  -  level %d  (%d/%d xp)"), *P.Ancestry, *P.ClassName, P.Level, P.Xp, NextXp), 11, Dim, false, false)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 18, 0)[Bar((float)P.Hp / FMath::Max(1, P.HpMax), C(0xB83A2F), FString::Printf(TEXT("HP %d / %d"), P.Hp, P.HpMax))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)[Stat(TEXT("AC"), FString::FromInt(AC(P)))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)[Stat(TEXT("SILVER"), FString::FromInt(P.Gold))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)[Stat(TEXT("RATIONS"), FString::FromInt(P.Rations))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)[Stat(TEXT("POTIONS"), FString::FromInt(P.Potions))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)[Stat(TEXT("HOLY WATER"), FString::FromInt(P.Holy))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)[Stat(TEXT("DAY"), FString::FromInt(G->Day))]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)[Btn(G->Mode == EHollowMode::Map ? TEXT("Back [M]") : TEXT("Map [M]"), [G] { G->ToggleMap(); }, G->Mode == EHollowMode::Map || G->CanWalk())]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)[Btn(FString::Printf(TEXT("Deck (%d)"), P.Deck.Num()), [G] { G->ToggleDeck(); })]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)[Btn(TEXT("Sheet [C]"), [G] { G->ToggleSheet(); })],
		PanelC, FMargin(14, 8));
}

TSharedRef<SWidget> SHollowUI::BuildExplore()
{
	AHollowGameMode* G = GM.Get();
	TWeakObjectPtr<AHollowGameMode> W = G;
	const FHollowHex* H = G->HexAt(G->Pos);
	const FString Place = H ? TerrInfo(H->T).Name : FString();
	TSharedRef<SVerticalBox> J = SNew(SVerticalBox);
	for (int32 i = 0; i < FMath::Min(5, G->Journal.Num()); ++i)
		J->AddSlot().AutoHeight().Padding(0, 1)[T(G->Journal[i], 11, i == 0 ? Fg : Dim)];

	return SNew(SVerticalBox).Visibility(EVisibility::SelfHitTestInvisible)
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 12, 16, 0)[BuildHud()]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(16, 8)
		[
			SNew(SBox).WidthOverride(380).Visibility(EVisibility::SelfHitTestInvisible)
			[
				Panel(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[T(Place, 20, Gold, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[T(FString::Printf(TEXT("Hex %d, %d  -  %s"), G->Pos.X, G->Pos.Y, G->Pos == FIntPoint(0, 0) ? TEXT("home") : *G->Compass(G->Pos)), 11, Dim)]
					+ SVerticalBox::Slot().AutoHeight()[J], C(0x1D1724, 0.8f), FMargin(12, 10))
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SSpacer).Visibility(EVisibility::HitTestInvisible)]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 28)
		[
			SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(C(0x1D1724, 0.9f)).Padding(FMargin(18, 10))
			.Visibility_Lambda([W] { return W.IsValid() && !W->Prompt.IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(STextBlock).Font(Font(16, true)).ColorAndOpacity(FSlateColor(Gold))
				.Text_Lambda([W] { return FText::FromString(W.IsValid() ? W->Prompt : FString()); })
			]
		];
}

/* ---------- Map ---------- */

TSharedRef<SWidget> SHollowUI::BuildMap()
{
	AHollowGameMode* G = GM.Get();
	TSharedRef<SVerticalBox> Info = SNew(SVerticalBox);
	if (G->MapSelected.IsSet())
	{
		const FIntPoint Sel = G->MapSelected.GetValue();
		const FHollowHex* H = G->HexAt(Sel);
		if (H)
		{
			const bool bHidden = H->State == EHexState::Hidden;
			const FString Name = bHidden ? FString(TEXT("Unknown country")) : TerrInfo(H->T).Name;
			static const TCHAR* States[] = { TEXT("Unknown. Nobody has told you about it."), TEXT("Rumored. Pay to scout a way in."), TEXT("Open. You can travel here."), TEXT("Explored."), TEXT("Cleared.") };
			Info->AddSlot().AutoHeight()[T(Name, 20, Gold, true)];
			if (!H->LeadName.IsEmpty() && !bHidden) Info->AddSlot().AutoHeight()[T(H->LeadName, 13, Fg, true)];
			Info->AddSlot().AutoHeight().Padding(0, 2, 0, 8)[T(FString::Printf(TEXT("%s  -  %s"), *G->Compass(Sel), States[(int32)H->State]), 12, Dim)];
			if (Sel == G->Pos) Info->AddSlot().AutoHeight()[Btn(TEXT("You are here: walk around"), [G] { G->ToggleMap(); })];
			else if (G->CanScout(Sel))
			{
				int32 Gd, Ra; G->ScoutCost(Sel, Gd, Ra);
				Info->AddSlot().AutoHeight()[Btn(FString::Printf(TEXT("Scout route (%d silver, %d ration)"), Gd, Ra), [G, Sel] { G->MapAction(Sel); }, G->PC.Gold >= Gd && G->PC.Rations >= Ra, Gold, C(0x1B1206))];
			}
			else if (G->CanTravel(Sel))
				Info->AddSlot().AutoHeight()[Btn(TEXT("Travel here (1 day, 1 ration)"), [G, Sel] { G->MapAction(Sel); }, true, Gold, C(0x1B1206))];
			else if (H->State == EHexState::Rumored)
				Info->AddSlot().AutoHeight()[T(TEXT("Scout the nearer hexes of this route first."), 12, Dim)];
			else if (H->IsOpen())
				Info->AddSlot().AutoHeight()[T(TEXT("Travel one hex at a time."), 12, Dim)];
		}
	}
	else Info->AddSlot().AutoHeight()[T(TEXT("Click a hex to inspect it. Click it again to act."), 13, Dim)];

	TSharedRef<SVerticalBox> Leads = SNew(SVerticalBox);
	for (const FLeadDef& L : HollowData::Leads())
		if (G->LeadsGiven.Contains(L.Id)) Leads->AddSlot().AutoHeight().Padding(0, 1)[T(TEXT("-  ") + L.Name, 12, Fg)];
	if (G->LeadsGiven.Num() == 0) Leads->AddSlot().AutoHeight()[T(TEXT("No leads yet. Ask the townsfolk about the wolves, the river, bandits, the shrine..."), 12, Dim)];

	return SNew(SVerticalBox).Visibility(EVisibility::SelfHitTestInvisible)
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 12, 16, 0)[BuildHud()]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(16, 10)
		[
			SNew(SHorizontalBox).Visibility(EVisibility::SelfHitTestInvisible)
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer).Visibility(EVisibility::HitTestInvisible)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(340)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[Panel(Info)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[Panel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)[T(TEXT("LEADS"), 11, Dim, true)]
						+ SVerticalBox::Slot().AutoHeight()[Leads])]
					+ SVerticalBox::Slot().AutoHeight()[Panel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[Btn(TEXT("Make camp (1 ration, heal half)"), [G] { G->MakeCamp(); }, G->PC.Rations > 0)]
						+ SVerticalBox::Slot().AutoHeight()[T(TEXT("Conversations reveal routes (?). Scouting a route costs silver and food; each step costs a day and a ration."), 11, Dim)])]
				]
			]
		];
}

/* ---------- Combat ---------- */

TSharedRef<SWidget> SHollowUI::BuildCard(const FCardDef& Card, bool bEnabled, TFunction<void()> OnClick, float Scale, int32 Serial, int32 Index)
{
	return SNew(SHollowCard).Card(&Card).Enabled(bEnabled).OnPlay(OnClick).Scale(Scale).Serial(Serial).Index(Index);
}

TSharedRef<SWidget> SHollowUI::BuildNote()
{
	AHollowGameMode* G = GM.Get();
	const FNoteView& V = G->Note.GetValue();
	return SNew(SBox).WidthOverride(520)[Panel(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[T(TEXT("you examine"), 11, ParchDim, true)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 10)[T(V.Title, 26, C(0x3A1712), true)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)[T(V.Text, 14, ParchInk)]
		+ SVerticalBox::Slot().AutoHeight()[Btn(TEXT("Close  [E]"), [G] { G->CloseNote(); }, true, Wine, C(0xF6E7C8), 14)],
		Parch, FMargin(24, 20))];
}

TSharedRef<SWidget> SHollowUI::BuildCombat()
{
	AHollowGameMode* G = GM.Get();
	const FCombatState& Cb = G->Combat.GetValue();
	const bool bMine = G->IsHeroTurn();

	// turn order strip, FFX style
	TSharedRef<SHorizontalBox> Order = SNew(SHorizontalBox);
	Order->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)[T(TEXT("TURN ORDER"), 11, Dim, true, false)];
	const TArray<int32> Prev = G->TurnPreview(9);
	for (int32 k = 0; k < Prev.Num(); ++k)
	{
		const FFighter& F = Cb.F[Prev[k]];
		const bool bHero = F.bHero;
		Order->AddSlot().AutoWidth().Padding(3, 0)
		[
			Panel(T(bHero ? FString(TEXT("YOU")) : F.Name.Left(12), k == 0 ? 13 : 11, bHero ? C(0x1B1206) : Fg, true, false),
				bHero ? Gold : (k == 0 ? C(0x7A2620) : C(0x3A2A40)), FMargin(10, 5))
		];
	}

	// foes, clickable to target
	TSharedRef<SVerticalBox> Foes = SNew(SVerticalBox);
	for (int32 i = 1; i < Cb.F.Num(); ++i)
	{
		const FFighter& F = Cb.F[i];
		FString Tags;
		if (F.OffGuardTurns > 0) Tags += TEXT("off-guard  ");
		if (F.Frightened > 0) Tags += FString::Printf(TEXT("frightened %d  "), F.Frightened);
		if (F.GuardAc > 0) Tags += TEXT("braced  ");
		if (F.Regen > 0) Tags += FString::Printf(TEXT("regen %d  "), F.Regen);
		if (F.bUndead) Tags += TEXT("undead");
		const bool bTarget = Cb.Target == i;
		Foes->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[
			SNew(SButton).ButtonStyle(&BtnStyle()).ButtonColorAndOpacity(F.bDead ? C(0x222222, 0.6f) : bTarget ? C(0x5A2A22) : C(0x2A1B22)).ContentPadding(FMargin(12, 8))
			.IsEnabled(!F.bDead)
			.OnClicked_Lambda([G, i] { G->SetTarget(i); return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)[T((bTarget ? FString(TEXT("> ")) : FString()) + F.Name, 15, bTarget ? Gold : Fg, true, false)]
					+ SHorizontalBox::Slot().AutoWidth()[T(FString::Printf(TEXT("AC %d"), F.Ac - F.Frightened - (F.OffGuardTurns > 0 ? 2 : 0)), 12, Dim, true, false)]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Bar((float)F.Hp / FMath::Max(1, F.MaxHp), C(0xB83A2F), FString::Printf(TEXT("%d / %d"), F.Hp, F.MaxHp), 300.f)]
				+ SVerticalBox::Slot().AutoHeight()[T(Tags, 10, Gold)]
				+ SVerticalBox::Slot().AutoHeight()[T(F.bDead ? FString(TEXT("defeated")) : TEXT("Intent: ") + F.IntentText, 11, Blood)]
			]
		];
	}

	// hero status and the FF-style command menu
	const FFighter& H = Cb.F[0];
	FString Tags;
	if (H.GuardAc > 0) Tags += FString::Printf(TEXT("guard +%d  "), H.GuardAc);
	if (H.ShieldHp > 0) Tags += FString::Printf(TEXT("shield %d  "), H.ShieldHp);
	if (H.Frightened > 0) Tags += FString::Printf(TEXT("frightened %d  "), H.Frightened);
	if (H.OffGuardTurns > 0) Tags += TEXT("off-guard  ");
	if (H.Slowed > 0) Tags += TEXT("slowed  ");
	if (Cb.BlessTurns > 0) Tags += FString::Printf(TEXT("bless %d  "), Cb.BlessTurns);
	if (Cb.HymnTurns > 0) Tags += FString::Printf(TEXT("hymn %d  "), Cb.HymnTurns);
	if (Cb.bLucky) Tags += TEXT("lucky  ");
	if (Cb.bQuicken) Tags += TEXT("quickened  ");
	if (Cb.bRiposte) Tags += TEXT("riposte");
	const int32 Pen = Cb.Attacks == 0 ? 0 : Cb.Attacks == 1 ? -5 : -10;
	const int32 Act = bMine ? Cb.Actions : 0;
	const bool bBoss = Cb.F.ContainsByPredicate([](const FFighter& F) { return F.bBoss && !F.bDead; });

	TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox);
	auto Cmd = [&](const FString& Label, const FString& Sub, int32 Cost, bool bOk, TFunction<void()> Fn)
	{
		Menu->AddSlot().AutoHeight().Padding(0, 2)
		[
			SNew(SButton).ButtonStyle(&BtnStyle()).ButtonColorAndOpacity(Panel2).ContentPadding(FMargin(10, 6)).IsEnabled(bMine && bOk && Act >= Cost)
			.OnClicked_Lambda([Fn] { Fn(); return FReply::Handled(); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[T(Label, 14, Fg, true, false)]
					+ SVerticalBox::Slot().AutoHeight()[T(Sub, 10, Dim, false, false)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Pips(Cost, Cost)]
			]
		];
	};
	Cmd(TEXT("Strike"), FString::Printf(TEXT("%s, d20 %s%s"), *G->PC.WeaponName, *Sgn(AtkMod(G->PC)), Pen ? *FString::Printf(TEXT(" (%d MAP)"), Pen) : TEXT("")), 1, true, [G] { G->CombatStrike(); });
	Cmd(TEXT("Raise Guard"), TEXT("+2 AC until your next turn"), 1, true, [G] { G->CombatGuard(); });
	Cmd(FString::Printf(TEXT("Potion (%d)"), G->PC.Potions), TEXT("Heal 2d8+4"), 1, G->PC.Potions > 0, [G] { G->CombatPotion(); });
	Cmd(FString::Printf(TEXT("Holy Water (%d)"), G->PC.Holy), TEXT("Burns undead, stops regeneration"), 1, G->PC.Holy > 0, [G] { G->CombatHoly(); });
	Cmd(TEXT("Flee"), bBoss ? TEXT("Not from Varek") : TEXT("Acrobatics/Athletics vs their Perception"), 2, !bBoss, [G] { G->CombatFlee(); });
	Menu->AddSlot().AutoHeight().Padding(0, 6, 0, 0)[Btn(TEXT("End Turn"), [G] { G->CombatEndTurn(); }, bMine, C(0x5A4220))];

	TSharedRef<SHorizontalBox> Hand = SNew(SHorizontalBox);
	for (int32 i = 0; i < Cb.Hand.Num(); ++i)
		if (const FCardDef* Cd = HollowData::Cards().Find(Cb.Hand[i]))
			Hand->AddSlot().AutoWidth().Padding(5, 0)[BuildCard(*Cd, bMine && Act >= Cd->Cost, [G, i] { G->PlayCard(i); }, 1.f, Cb.HandSerial.IsValidIndex(i) ? Cb.HandSerial[i] : -1, i)];

	TSharedRef<SVerticalBox> LogBox = SNew(SVerticalBox);
	for (int32 i = 0; i < FMath::Min(9, Cb.Log.Num()); ++i)
		LogBox->AddSlot().AutoHeight().Padding(0, 1)[T(Cb.Log[i], 11, i == 0 ? Fg : Dim)];

	TSharedRef<SOverlay> Out = SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible);
	Out->AddSlot()
	[
		SNew(SVerticalBox).Visibility(EVisibility::SelfHitTestInvisible)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(16, 14, 16, 0)[Panel(Order, PanelC, FMargin(12, 8))]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(16, 10)
		[
			SNew(SHorizontalBox).Visibility(EVisibility::SelfHitTestInvisible)
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer).Visibility(EVisibility::HitTestInvisible)]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(330)[Foes]]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 16)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
			[
				SNew(SBox).WidthOverride(290)
				[
					Panel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f)[T(G->PC.Name, 16, Gold, true, false)]
							+ SHorizontalBox::Slot().AutoWidth()[T(FString::Printf(TEXT("AC %d"), G->HeroAC()), 13, Fg, true, false)]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Bar((float)G->PC.Hp / FMath::Max(1, G->PC.HpMax), C(0xB83A2F), FString::Printf(TEXT("HP %d / %d"), G->PC.Hp, G->PC.HpMax), 262.f)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)[T(bMine ? TEXT("ACTIONS") : TEXT("WAITING"), 11, Dim, true, false)]
							+ SHorizontalBox::Slot().AutoWidth()[Pips(Act, 3)]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 6)[T(Tags, 10, Gold)]
						+ SVerticalBox::Slot().AutoHeight()[Menu])
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(10, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Hand]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 0)
				[T(FString::Printf(TEXT("Draw pile %d   -   Discard %d   -   Cards cost the same actions as your Strikes"), Cb.Draw.Num(), Cb.Discard.Num()), 11, Dim, false, false)]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
			[
				SNew(SBox).WidthOverride(360)[Panel(LogBox, C(0x0E0B12, 0.9f), FMargin(12, 10))]
			]
		]
	];
	if (!G->PlayedCard.IsEmpty() && G->Now() - G->PlayedAt < 0.85)
		if (const FCardDef* Pc = HollowData::Cards().Find(G->PlayedCard))
			Out->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, 60)[SNew(SHollowFly).Start(G->PlayedAt).Content(BuildCard(*Pc, true, [] {}, 1.1f))];
	if (Cb.bOver && Cb.bWon && !Cb.bBusy)
	{
		Out->AddSlot()[Dimmer(SNew(SBox).WidthOverride(420)[Panel(SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[T(TEXT("Victory!"), 34, Gold, true, false)]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 14)[T(FString::Printf(TEXT("+%d XP     +%d silver"), Cb.XpGain, Cb.GoldGain), 16, Fg, true, false)]
			+ SVerticalBox::Slot().AutoHeight()[Btn(TEXT("Continue"), [G] { G->CombatContinue(); }, true, Gold, C(0x1B1206), 16)], PanelC, FMargin(24)) ])];
	}
	return Out;
}

/* ---------- End ---------- */

TSharedRef<SWidget> SHollowUI::BuildEnd()
{
	AHollowGameMode* G = GM.Get();
	return Dimmer(SNew(SBox).WidthOverride(560)[Panel(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[T(G->EndTitle, 34, G->bVictory ? Gold : Blood, true, false)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 18)[T(G->EndText, 15, Fg)]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[T(FString::Printf(TEXT("Level %d   -   %d days   -   %d silver"), G->PC.Level, G->Day, G->PC.Gold), 13, Dim, false, false)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 0)[Btn(TEXT("Begin a new tale"), [G] { G->Restart(); }, true, Gold, C(0x1B1206), 16)], PanelC, FMargin(28))]);
}

/* ---------- Overlays ---------- */

TSharedRef<SWidget> SHollowUI::BuildEvent()
{
	AHollowGameMode* G = GM.Get();
	const FEventView& V = G->Event.GetValue();
	const FEventDef* E = HollowData::FindEvent(V.EventId);
	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	Body->AddSlot().AutoHeight()[T(TEXT("encounter"), 11, ParchDim, true)];
	Body->AddSlot().AutoHeight().Padding(0, 2, 0, 8)[T(E ? E->Title : FString(), 26, C(0x3A1712), true)];
	Body->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[T(E ? E->Text : FString(), 14, ParchInk)];
	if (V.Choice < 0 && E)
	{
		for (int32 i = 0; i < E->Choices.Num(); ++i)
		{
			const FChoice& Ch = E->Choices[i];
			FString Sub;
			if (Ch.IsCheck())
			{
				const FString Sk = BestSkill(G->PC, Ch.Skills);
				Sub = FString::Printf(TEXT("%s DC %d (%s)"), *SkillName(Sk), Ch.DC, *Sgn(SkillMod(G->PC, Sk)));
			}
			if (Ch.Cost) Sub = FString::Printf(TEXT("costs %d silver"), Ch.Cost);
			const bool bOk = Ch.Cost <= G->PC.Gold;
			Body->AddSlot().AutoHeight().Padding(0, 3)
			[
				SNew(SButton).ButtonStyle(&BtnStyle()).ButtonColorAndOpacity(C(0xF4E9D2)).ContentPadding(FMargin(12, 8)).IsEnabled(bOk)
				.OnClicked_Lambda([G, i] { G->ChooseEventOption(i); return FReply::Handled(); })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[T(Ch.Label, 14, ParchInk, true)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)[T(Sub, 11, ParchDim, true, false)]
				]
			];
		}
	}
	else
	{
		if (!V.RollLine.IsEmpty())
			Body->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[Panel(T(V.RollLine, 12, ParchInk, true), C(0xD2BC93), FMargin(8, 6))];
		Body->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[T(V.Result, 14, ParchInk)];
		TSharedRef<SWrapBox> Chips = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6, 6));
		for (const auto& Cp : V.Chips)
			Chips->AddSlot()[Panel(T(Cp.Key, 12, FLinearColor::White, true, false), Cp.Value ? C(0x3F5A2E) : C(0x7A2620), FMargin(8, 3))];
		Body->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[Chips];
		Body->AddSlot().AutoHeight()[Btn(V.PendingCombat.IsEmpty() ? TEXT("Continue  [E]") : TEXT("To battle!  [E]"), [G] { G->CloseEvent(); }, true, Wine, C(0xF6E7C8), 15)];
	}
	return SNew(SBox).WidthOverride(500)[Panel(Body, Parch, FMargin(22, 18))];
}

TSharedRef<SWidget> SHollowUI::BuildTalk()
{
	AHollowGameMode* G = GM.Get();
	const FTalkView& V = G->Talk.GetValue();
	const FNpcDef* N = HollowData::FindNpc(V.NpcId);
	if (!N) return SNew(SSpacer);

	TSharedRef<SWrapBox> Chips = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6, 6));
	for (const FTopicDef& Tp : HollowData::Topics())
	{
		if (!G->KnownTopics.Contains(Tp.Id)) continue;
		const bool bKnows = N->Replies.Contains(Tp.Id);
		const FString Id = Tp.Id;
		Chips->AddSlot()[Btn(Tp.Label, [G, Id] { G->AskTopic(Id); }, true, bKnows ? C(0x3A2A40) : C(0x262030), bKnows ? Fg : Dim, 12)];
	}

	TSharedRef<SHorizontalBox> Services = SNew(SHorizontalBox);
	auto Svc = [&](const FString& Label, const FString& What) { Services->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Btn(Label, [G, What] { G->Service(What); }, true, C(0x5A4220))]; };
	if (N->Service == TEXT("inn")) Svc(TEXT("Rest the night (5 silver)"), TEXT("rest"));
	if (N->Service == TEXT("temple")) { Svc(TEXT("Holy water (10)"), TEXT("holy")); Svc(TEXT("Healing prayer (6)"), TEXT("prayer")); }
	if (N->Service == TEXT("shop")) { Svc(TEXT("Potion (8)"), TEXT("potion")); Svc(TEXT("Ration (2)"), TEXT("ration")); Svc(TEXT("Card pack (12)"), TEXT("pack")); }

	TSharedPtr<SEditableTextBox> Box;
	SAssignNew(Box, SEditableTextBox).Font(Font(14)).HintText(FText::FromString(TEXT("Ask about anything...  e.g. \"what's going on with the wolves up north?\"")))
		.OnTextCommitted_Lambda([G](const FText& Tx, ETextCommit::Type How) { if (How == ETextCommit::OnEnter && !Tx.IsEmpty()) G->AskText(Tx.ToString()); });
	AskBox = Box;
	TWeakPtr<SEditableTextBox> WeakBox = Box;

	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	Body->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[T(N->Name, 20, Gold, true, false)]
			+ SVerticalBox::Slot().AutoHeight()[T(N->Role, 11, Dim, false, false)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)[Btn(TEXT("Leave"), [G] { G->CloseTalk(); })]
	];
	Body->AddSlot().AutoHeight().Padding(0, 2, 0, 0)[T(G->bBrainOnline ? FString::Printf(TEXT("voice: %s (local AI, bound to what %s knows)"), *G->BrainModel, *N->Name) : FString(TEXT("voice: response library (start Ollama for free-form answers)")), 10, G->bBrainOnline ? Moss : Dim, false, false)];
	if (!V.Asked.IsEmpty()) Body->AddSlot().AutoHeight().Padding(0, 8, 0, 0)[T(TEXT("You: ") + V.Asked, 12, Dim)];
	Body->AddSlot().AutoHeight().Padding(0, 6, 0, 8)[T(V.bThinking ? V.Line : TEXT("\"") + V.Line.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT("")) + TEXT("\""), 16, V.bThinking ? Dim : Fg)];
	if (!V.LeadNote.IsEmpty()) Body->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[Panel(T(V.LeadNote, 13, C(0x1B1206), true), Gold, FMargin(10, 6))];
	Body->AddSlot().AutoHeight().Padding(0, 0, 0, 4)[T(TEXT("TOPICS YOU KNOW"), 10, Dim, true)];
	Body->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[Chips];
	Body->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 6, 0)[Box.ToSharedRef()]
		+ SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("Ask"), [G, WeakBox] { if (auto B = WeakBox.Pin()) { const FString S = B->GetText().ToString(); if (!S.IsEmpty()) G->AskText(S); } }, true, Gold, C(0x1B1206))]
	];
	Body->AddSlot().AutoHeight()[Services];
	return SNew(SBox).WidthOverride(860)[Panel(Body, PanelC, FMargin(18, 14))];
}

TSharedRef<SWidget> SHollowUI::BuildReward()
{
	AHollowGameMode* G = GM.Get();
	const FRewardView& V = G->Reward.GetValue();
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 i = 0; i < V.Options.Num(); ++i)
		if (const FCardDef* Cd = HollowData::Cards().Find(V.Options[i]))
			Row->AddSlot().AutoWidth().Padding(8, 0)[BuildCard(*Cd, true, [G, i] { G->PickReward(i); }, 1.15f)];
	return Panel(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 12)[T(V.Title, 22, Gold, true, false)]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Row]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 0)[Btn(TEXT("Take nothing"), [G] { G->PickReward(-1); })],
		PanelC, FMargin(24));
}

TSharedRef<SWidget> SHollowUI::BuildSheet()
{
	AHollowGameMode* G = GM.Get();
	const FPC& P = G->PC;
	TSharedRef<SHorizontalBox> Ab = SNew(SHorizontalBox);
	for (const TCHAR* K : { TEXT("str"), TEXT("dex"), TEXT("con"), TEXT("int"), TEXT("wis"), TEXT("cha") })
		Ab->AddSlot().AutoWidth().Padding(0, 0, 14, 0)[Stat(FString(K).ToUpper(), Sgn(P.Abil.FindRef(K)))];
	TSharedRef<SVerticalBox> Sk = SNew(SVerticalBox);
	static const TCHAR* Rk[] = { TEXT("U"), TEXT("T"), TEXT("E"), TEXT("M"), TEXT("L") };
	for (const FString& S : SkillList())
	{
		const int32 R = P.Ranks.FindRef(S);
		Sk->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)[T(Rk[FMath::Clamp(R, 0, 4)], 11, R ? Moss : Dim, true, false)]
			+ SHorizontalBox::Slot().FillWidth(1.f)[T(SkillName(S), 12, Fg, false, false)]
			+ SHorizontalBox::Slot().AutoWidth()[T(Sgn(SkillMod(P, S)), 12, Fg, true, false)]
		];
	}
	return SNew(SBox).WidthOverride(560)[Panel(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[T(P.Name, 26, Gold, true, false)]
			+ SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("Close"), [G] { G->ToggleSheet(); })]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[T(FString::Printf(TEXT("Level %d %s %s  -  %d / %d XP"), P.Level, *P.Ancestry, *P.ClassName, P.Xp, XpForLevel(P.Level + 1)), 12, Dim)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[Ab]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 14, 0)[Stat(TEXT("AC"), FString::FromInt(AC(P)))]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 14, 0)[Stat(TEXT("FORT"), Sgn(SaveMod(P, TEXT("fort"))))]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 14, 0)[Stat(TEXT("REF"), Sgn(SaveMod(P, TEXT("ref"))))]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 14, 0)[Stat(TEXT("WILL"), Sgn(SaveMod(P, TEXT("will"))))]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 14, 0)[Stat(TEXT("PERCEPTION"), Sgn(SkillMod(P, TEXT("perception"))))]
			+ SHorizontalBox::Slot().AutoWidth()[Stat(TEXT("SPELL DC"), FString::FromInt(SpellDC(P)))]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[T(FString::Printf(TEXT("%s: d20 %s, 1d%d %s damage.   Armor: %s."), *P.WeaponName, *Sgn(AtkMod(P)), P.WeaponDie, *Sgn(DmgBonus(P)), *P.ArmorName), 12, Fg)]
		+ SVerticalBox::Slot().AutoHeight()[Sk], PanelC, FMargin(20))];
}

TSharedRef<SWidget> SHollowUI::BuildDeck()
{
	AHollowGameMode* G = GM.Get();
	TMap<FString, int32> Counts;
	for (const FString& Id : G->PC.Deck) Counts.FindOrAdd(Id)++;
	TSharedRef<SWrapBox> Wrap = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8, 8));
	for (auto& Pr : Counts)
		if (const FCardDef* Cd = HollowData::Cards().Find(Pr.Key))
			Wrap->AddSlot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[BuildCard(*Cd, true, [] {}, 0.9f)]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[T(FString::Printf(TEXT("x%d"), Pr.Value), 13, Gold, true, false)]
			];
	return SNew(SBox).WidthOverride(900).MaxDesiredHeight(640)[Panel(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[T(FString::Printf(TEXT("Your deck  (%d cards)"), G->PC.Deck.Num()), 22, Gold, true, false)]
			+ SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("Close"), [G] { G->ToggleDeck(); })]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[Wrap]], PanelC, FMargin(20))];
}
