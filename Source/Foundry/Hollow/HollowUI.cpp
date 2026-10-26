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
#include "Engine/Font.h"
#include "Framework/Text/SlateTextRun.h"
#include "Styling/SlateStyle.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "HAL/PlatformTime.h"
#include "HollowGameMode.h"
#include "HollowRules.h"
#include "HollowLibrary.h"

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
	// Seph's OFL fonts (Docs/Credits.md): Cinzel for display and labels, Alegreya for body text, IM Fell English for spoken lines.
	// Each loads by path and falls back to the engine font, so a missing asset never breaks the UI.
	const UObject* FontAsset(const TCHAR* Name)
	{
		static TMap<FString, const UObject*> Cache;
		if (const UObject** Found = Cache.Find(Name)) return *Found;
		UObject* F = LoadObject<UObject>(nullptr, *FString::Printf(TEXT("/Game/Hollow/UI/Fonts/%s_Font.%s_Font"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!F || !F->IsA<UFont>()) F = LoadObject<UObject>(nullptr, *FString::Printf(TEXT("/Game/Hollow/UI/Fonts/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (F && !F->IsA<UFont>()) F = nullptr;
		if (F) F->AddToRoot();
		Cache.Add(Name, F);
		return F;
	}
	FSlateFontInfo FontOf(const TCHAR* Name, int32 Size, const ANSICHAR* Fallback)
	{
		if (const UObject* F = FontAsset(Name)) return FSlateFontInfo(F, Size);
		return FCoreStyle::GetDefaultFontStyle(Fallback, Size);
	}
	FSlateFontInfo Display(int32 Size) { return FontOf(TEXT("Cinzel-Variable"), Size, "Bold"); }
	FSlateFontInfo Prose(int32 Size, bool bItalic = false) { return FontOf(bItalic ? TEXT("IMFellEnglish-Italic") : TEXT("IMFellEnglish-Regular"), Size, "Regular"); }
	FSlateFontInfo Body(int32 Size, bool bItalic = false) { return FontOf(bItalic ? TEXT("Alegreya-Italic-Variable") : TEXT("Alegreya-Variable"), Size, "Regular"); }
	FSlateFontInfo Font(int32 Size, bool bBold = false) { return bBold ? Display(FMath::Max(9, Size - 1)) : Body(Size + 1); }
	TSharedRef<STextBlock> TF(const FString& S, const FSlateFontInfo& F, const FLinearColor& Col, bool bWrap = true)
	{
		return SNew(STextBlock).Text(FText::FromString(S)).Font(F).ColorAndOpacity(FSlateColor(Col)).AutoWrapText(bWrap);
	}

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
		else if (G->LevelUp.IsSet()) Root->AddSlot()[Dimmer(BuildLevelUp())];
		else if (G->Trade.IsSet()) Root->AddSlot()[Dimmer(BuildTrade())];
		else if (G->Talk.IsSet())
		{
			// cinematic letterbox while someone is speaking
			Root->AddSlot().VAlign(VAlign_Top)[SNew(SBox).HeightOverride(56)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(FLinearColor(0, 0, 0, 0.85f))]];
			Root->AddSlot().VAlign(VAlign_Bottom)[SNew(SBox).HeightOverride(40)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(FLinearColor(0, 0, 0, 0.85f))]];
			Root->AddSlot().VAlign(VAlign_Bottom).HAlign(HAlign_Center).Padding(16, 16, 16, 20)[BuildTalk()];
		}
		if (G->bPaused) Root->AddSlot()[BuildPause()];
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
	FSlateFontInfo TitleFont = Display(96);
	TitleFont.LetterSpacing = 260;
	FSlateFontInfo SubFont = Display(15);
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
						SNew(SEditableTextBox).Text(FText::FromString(G->CreateName)).Font(Font(16)).ForegroundColor(C(0xF6E7C8))
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
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 2)[T(FString::Printf(TEXT("Starting gear: %s, %s, adventurer's pack, %d rations, a healing potion, torches and a kit tool."), *P.WeaponName, *P.ArmorName, P.Count(TEXT("ration"))), 11, Ink)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)[T(TEXT("Starter deck: ") + DeckLine, 10, InkDim)]
					+ SVerticalBox::Slot().AutoHeight()[Label(TEXT("OR BRING YOUR OWN: PATHBUILDER 2E"))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 6, 0)
						[
							SNew(SEditableTextBox).Text(FText::FromString(G->PbId)).Font(Font(13)).HintText(FText::FromString(TEXT("Build id from Pathbuilder: Menu > Export > Export JSON (or paste the JSON)")))
							.OnTextChanged_Lambda([G](const FText& Tx) { G->PbId = Tx.ToString(); })
							.OnTextCommitted_Lambda([G](const FText& Tx, ETextCommit::Type How) { G->PbId = Tx.ToString(); if (How == ETextCommit::OnEnter) G->ImportPathbuilder(G->PbId); })
						]
						+ SHorizontalBox::Slot().AutoWidth()[Btn(G->bPbBusy ? TEXT("Fetching...") : TEXT("Import"), [G] { G->ImportPathbuilder(G->PbId); }, !G->bPbBusy, C(0x5A4220), C(0xF6E7C8))]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[T(G->PbStatus.IsEmpty() ? FString(TEXT("Your level, ability scores, proficiencies, HP, AC, weapon, coin and gear come across. Recognised gear (rope, shovel, torches, thieves' tools...) gives field abilities.")) : G->PbStatus, 10, G->bImported ? C(0x3F5A2E) : InkDim)]
					+ SVerticalBox::Slot().AutoHeight()[Btn(G->bImported ? FString::Printf(TEXT("Begin as %s"), *G->PC.Name) : FString(TEXT("Begin in Brindlemark")), [G] { G->BeginAdventure(); }, true, Wine, C(0xF6E7C8), 17)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)[T(TEXT("WASD or click to walk.  E to talk or examine.  Mouse wheel to zoom.  M map.  I inventory.  C sheet.  J journal.  Esc pause."), 10, InkDim)]
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
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)[Stat(TEXT("RATIONS"), FString::FromInt(P.Count(TEXT("ration"))))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)[Stat(TEXT("POTIONS"), FString::FromInt(P.Count(TEXT("potion"))))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)[Stat(TEXT("HOLY WATER"), FString::FromInt(P.Count(TEXT("holy"))))]
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
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(16, 0)[BuildAbilities()]
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
				Info->AddSlot().AutoHeight()[Btn(FString::Printf(TEXT("Scout route (%d silver, %d ration)"), Gd, Ra), [G, Sel] { G->MapAction(Sel); }, G->PC.Gold >= Gd && G->PC.Count(TEXT("ration")) >= Ra, Gold, C(0x1B1206))];
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
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[Btn(TEXT("Make camp (1 ration, heal half)"), [G] { G->MakeCamp(); }, G->PC.Count(TEXT("ration")) > 0)]
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
	Cmd(FString::Printf(TEXT("Potion (%d)"), G->PC.Count(TEXT("potion"))), TEXT("Heal 2d8+4"), 1, G->PC.Count(TEXT("potion")) > 0, [G] { G->CombatPotion(); });
	Cmd(FString::Printf(TEXT("Holy Water (%d)"), G->PC.Count(TEXT("holy"))), TEXT("Burns undead, stops regeneration"), 1, G->PC.Count(TEXT("holy")) > 0, [G] { G->CombatHoly(); });
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

namespace
{
	// Rich text for spoken lines: [topic] words render in gold. Built per frame for the typewriter reveal.
	const ISlateStyle& RichStyle()
	{
		static TSharedPtr<FSlateStyleSet> S;
		if (!S)
		{
			S = MakeShared<FSlateStyleSet>(TEXT("HollowRich"));
			const FTextBlockStyle Base = FTextBlockStyle().SetFont(Prose(20)).SetColorAndOpacity(FSlateColor(Fg));
			S->Set(TEXT("n"), Base);
			S->Set(TEXT("t"), FTextBlockStyle(Base).SetColorAndOpacity(FSlateColor(Gold)));
		}
		return *S;
	}
	// The first N visible characters of a [bracketed] line as <t>markup</>, closing any tag the cut lands inside.
	FString MarkupPrefix(const FString& Src, int32 N)
	{
		FString O;
		bool bIn = false;
		int32 Shown = 0;
		for (TCHAR Ch : Src)
		{
			if (Ch == TEXT('[')) { O += TEXT("<t>"); bIn = true; continue; }
			if (Ch == TEXT(']')) { O += TEXT("</>"); bIn = false; continue; }
			if (Shown >= N) break;
			if (Ch == TEXT('<')) O += TEXT("&lt;");
			else O.AppendChar(Ch);
			Shown++;
		}
		if (bIn) O += TEXT("</>");
		return O;
	}
	FLinearColor TagColor(int32 Style)
	{
		switch (Style)
		{
		case 1: return C(0x3B4F7A);   // skill check
		case 2: return C(0x3F5A2E);   // item
		case 3: return C(0x6A4E1E);   // silver / trade
		case 4: return C(0x4A2622);   // locked
		default: return C(0x2A2430);
		}
	}
}

TSharedRef<SWidget> SHollowUI::BuildTalk()
{
	AHollowGameMode* G = GM.Get();
	const FTalkView& V = G->Talk.GetValue();
	const FNpcDef* N = HollowData::FindNpc(V.NpcId);
	if (!N) return SNew(SSpacer);
	TWeakObjectPtr<AHollowGameMode> W = G;

	// portrait medallion: the speaker's colours, their initial in Cinzel, ringed in their hat colour
	static FSlateRoundedBoxBrush Outer(FLinearColor::White, 56.f), Inner(FLinearColor::White, 51.f);
	const TSharedRef<SWidget> Portrait = SNew(SBox).WidthOverride(112).HeightOverride(112)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SBorder).BorderImage(&Outer).BorderBackgroundColor(N->Hat)]
			+ SOverlay::Slot().Padding(5)[SNew(SBorder).BorderImage(&Inner).BorderBackgroundColor(N->Body * 0.8f)
				.HAlign(HAlign_Center).VAlign(VAlign_Center)[TF(N->Name.Left(1), Display(52), N->Skin, false)]]
		];

	TSharedRef<SVerticalBox> Speech = SNew(SVerticalBox);
	Speech->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)[TF(N->Name, Display(24), Gold, false)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(12, 0, 0, 3)[TF(N->Role, Body(14, true), Dim, false)]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)[Btn(V.bLog ? TEXT("Back") : TEXT("Log"), [G] { G->ToggleTalkLog(); }, true, Panel2, Dim, 11)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Btn(TEXT("Leave  [Esc]"), [G] { G->CloseTalk(); }, true, Panel2, Dim, 11)]
	];
	Speech->AddSlot().AutoHeight().Padding(0, 2, 0, 6)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(C(0x8C6A36, 0.6f))]];
	if (!V.LastCheck.IsEmpty())
		Speech->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0, 0, 0, 6)[Panel(TF(V.LastCheck, Display(12), FLinearColor::White, false), V.bLastOk ? C(0x3F5A2E) : C(0x7A2620), FMargin(10, 4), false)];
	if (!V.Asked.IsEmpty()) Speech->AddSlot().AutoHeight().Padding(0, 0, 0, 4)[TF(TEXT("You: ") + V.Asked, Body(14, true), Dim)];

	if (V.bLog)
	{
		TSharedRef<SVerticalBox> Lg = SNew(SVerticalBox);
		for (const auto& H : V.History)
		{
			Lg->AddSlot().AutoHeight().Padding(0, 4, 0, 0)[TF(TEXT("You: ") + H.Key, Body(13, true), Dim)];
			Lg->AddSlot().AutoHeight()[TF(H.Value.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT("")), Prose(16), Fg)];
		}
		if (!V.History.Num()) Lg->AddSlot().AutoHeight()[TF(TEXT("Nothing said yet."), Body(13, true), Dim)];
		Speech->AddSlot().AutoHeight()[SNew(SBox).MaxDesiredHeight(300)[SNew(SScrollBox) + SScrollBox::Slot()[Lg]]];
	}
	else
	{
		Speech->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[
			SNew(SRichTextBlock).AutoWrapText(true).DecoratorStyleSet(&RichStyle()).TextStyle(&RichStyle().GetWidgetStyle<FTextBlockStyle>(TEXT("n")))
			.Text_Lambda([W]
			{
				if (!W.IsValid() || !W->Talk.IsSet()) return FText::GetEmpty();
				const FTalkView& Tv = W->Talk.GetValue();
				if (Tv.bThinking) return FText::FromString(Tv.Line);
				const int32 Shown = (int32)((FPlatformTime::Seconds() - Tv.LineAt) * 55.0);
				return FText::FromString(TEXT("“") + MarkupPrefix(Tv.Line, FMath::Max(0, Shown)) + TEXT("”"));
			})
		];
		if (!V.LeadNote.IsEmpty()) Speech->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0, 0, 0, 8)[Panel(TF(V.LeadNote, Display(12), C(0x1B1206)), Gold, FMargin(10, 5), false)];
	}

	// numbered responses, BG3 / Disco Elysium style: requirement tags on the right, heard lines dimmed, locked lines greyed with the reason
	TSharedRef<SVerticalBox> Resp = SNew(SVerticalBox);
	if (!V.bLog)
	{
		const TArray<FTalkChoice> L = G->TalkChoices();
		for (int32 i = 0; i < L.Num(); ++i)
		{
			const FTalkChoice& Ch = L[i];
			const FLinearColor TextCol = Ch.bLocked ? C(0x6A5E66) : (Ch.bSeen || Ch.Kind == FTalkChoice::Leave) ? Dim : Fg;
			TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)
				[SNew(SBox).WidthOverride(22)[TF(i < 9 ? FString::Printf(TEXT("%d."), i + 1) : FString(TEXT("-")), Display(14), Ch.bLocked ? C(0x6A5E66) : Gold, false)]]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[TF(Ch.Text, Body(17, Ch.Kind == FTalkChoice::Say), TextCol)];
			if (!Ch.Tag.IsEmpty())
				Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)[Panel(TF(Ch.Tag, Display(11), Ch.bLocked ? C(0xC9A6A0) : FLinearColor::White, false), TagColor(Ch.TagStyle), FMargin(8, 2), false)];
			Resp->AddSlot().AutoHeight().Padding(0, 1)
			[
				SNew(SButton).ButtonStyle(&BtnStyle()).ButtonColorAndOpacity(C(0x1A1520, Ch.bLocked ? 0.35f : 0.75f)).ContentPadding(FMargin(10, 5))
				.OnClicked_Lambda([G, i] { G->ChooseTalk(i); return FReply::Handled(); })
				[Row]
			];
		}
	}
	if (V.bTyping)
	{
		TSharedPtr<SEditableTextBox> Box;
		SAssignNew(Box, SEditableTextBox).Font(Body(16)).HintText(FText::FromString(FString::Printf(TEXT("Say anything to %s...  e.g. \"what's going on with the wolves up north?\""), *N->Name)))
			.OnTextCommitted_Lambda([G](const FText& Tx, ETextCommit::Type How) { if (How == ETextCommit::OnEnter && !Tx.IsEmpty() && G->Talk.IsSet()) { G->Talk->bTyping = false; G->AskText(Tx.ToString()); } });
		AskBox = Box;
		TWeakPtr<SEditableTextBox> WeakBox = Box;
		Resp->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 6, 0)[Box.ToSharedRef()]
			+ SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("Say"), [G, WeakBox] { if (auto B = WeakBox.Pin()) { const FString S = B->GetText().ToString(); if (!S.IsEmpty() && G->Talk.IsSet()) { G->Talk->bTyping = false; G->AskText(S); } } }, true, Gold, C(0x1B1206))]
		];
		Resp->AddSlot().AutoHeight().Padding(0, 2, 0, 0)[TF(G->bBrainOnline ? FString::Printf(TEXT("Free voice: %s, a local model bound to what %s knows."), *G->BrainModel, *N->Name) : FString(TEXT("Free voice is offline (start Ollama). Matching your words to what they know instead.")), Body(12, true), G->bBrainOnline ? Moss : Dim)];
	}

	return SNew(SBox).WidthOverride(1120)
	[
		Panel(SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0, 0, 18, 0)[Portrait]
				+ SHorizontalBox::Slot().FillWidth(1.f)[Speech]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(130, 4, 0, 0)[Resp]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 8, 0, 0)[TF(TEXT("1-9 choose    E skip    Log    Esc leave"), Display(10), C(0x6E6278), false)],
		PanelC, FMargin(22, 16))
	];
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
	// what the rules library knows about you: origins, feats by level, spells (spent ones dimmed)
	TSharedRef<SVerticalBox> Extra = SNew(SVerticalBox);
	TArray<FString> Origin;
	for (const FString& O : { P.Heritage, P.Background, P.Deity }) if (!O.IsEmpty()) Origin.Add(O);
	if (Origin.Num()) Extra->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[TF(FString::Join(Origin, TEXT("  -  ")), Body(13, true), Dim)];
	if (P.Feats.Num())
	{
		Extra->AddSlot().AutoHeight()[TF(TEXT("FEATS"), Display(11), C(0xB79CDB), false)];
		TSharedRef<SWrapBox> Fw = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
		for (const auto& F : P.Feats)
		{
			const FLibFeat* L = HollowLib::Feat(F.Value);
			Fw->AddSlot()[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(Panel2).Padding(FMargin(6, 2)).ToolTipText(FText::FromString(L ? L->Text : FString()))[TF(FString::Printf(TEXT("%d  %s"), F.Key, *F.Value), Body(12), Fg, false)]];
		}
		Extra->AddSlot().AutoHeight().Padding(0, 2, 0, 6)[Fw];
	}
	if (P.Spells.Num())
	{
		Extra->AddSlot().AutoHeight()[TF(FString::Printf(TEXT("SPELLS  -  %s, DC %d, attack %s"), *G->CasterTradition(), SpellDC(P), *Sgn(SpellAtk(P))), Display(11), C(0xB79CDB), false)];
		TSharedRef<SWrapBox> Sw = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
		for (const FString& Sp : P.Spells)
		{
			const FLibSpell* L = HollowLib::Spell(Sp);
			bool bSpent = false;
			for (const FString& Id : G->SpentCards) if (const FCardDef* Cd = HollowData::Cards().Find(Id)) bSpent |= Cd->Spell == Sp;
			Sw->AddSlot()[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(C(0x2A2A4A)).Padding(FMargin(6, 2)).ToolTipText(FText::FromString(L ? HollowLib::SpellSummary(*L, L->Rank) + TEXT(". ") + L->Text : FString()))
				[TF(L ? FString::Printf(TEXT("%s %s"), L->bCantrip ? TEXT("c") : *FString::FromInt(L->Rank), *Sp) : Sp, Body(12), bSpent ? C(0x6A5E66) : Fg, false)]];
		}
		Extra->AddSlot().AutoHeight().Padding(0, 2, 0, 0)[Sw];
	}

	return SNew(SBox).WidthOverride(760)[Panel(SNew(SVerticalBox)
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
		+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).MaxDesiredHeight(300)[SNew(SScrollBox) + SScrollBox::Slot()[Sk]]]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)[Extra], PanelC, FMargin(20))];
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

/* ---------- Pause screen (Inventory / Character / Deck / Journal / System) ---------- */

namespace
{
	const FSlateBrush* Tiled(const FString& Path)
	{
		static TMap<FString, FSlateBrush*> Cache;
		if (FSlateBrush** Found = Cache.Find(Path)) return *Found;
		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		FSlateBrush* B = nullptr;
		if (Tex)
		{
			Tex->AddToRoot();
			B = new FSlateBrush();
			B->SetResourceObject(Tex);
			B->ImageSize = FVector2D(512, 512);
			B->Tiling = ESlateBrushTileType::Both;
		}
		Cache.Add(Path, B);
		return B;
	}
	const FSlateBrush* Leather() { return Tiled(TEXT("/Game/Hollow/UI/Panels/Leather011/Leather011_1K-PNG_Color.Leather011_1K-PNG_Color")); }
	const FSlateBrush* Paper() { return Tiled(TEXT("/Game/Hollow/UI/Panels/Paper001/Paper001_1K-PNG_Color.Paper001_1K-PNG_Color")); }

	// A textured panel: tiled leather or paper under a tint, with the gold border on top.
	TSharedRef<SWidget> Hide(const TSharedRef<SWidget>& Content, const FSlateBrush* Tex, const FLinearColor& Tint, const FMargin& Pad)
	{
		TSharedRef<SOverlay> O = SNew(SOverlay);
		if (Tex) O->AddSlot()[SNew(SImage).Image(Tex).ColorAndOpacity(Tint)];
		O->AddSlot()[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(Tex ? FLinearColor(0, 0, 0, 0.35f) : PanelC).Padding(Pad)[Content]];
		if (Frame()) O->AddSlot()[SNew(SBorder).BorderImage(Frame()).BorderBackgroundColor(C(0x8C6A36, 0.8f)).Visibility(EVisibility::HitTestInvisible)];
		return O;
	}
	TSharedRef<SWidget> Chip(const FString& S, const FLinearColor& Bg, const FLinearColor& Fgc = FLinearColor::White)
	{
		return Panel(TF(S, Display(11), Fgc, false), Bg, FMargin(8, 3), false);
	}
	TSharedRef<SWidget> Section(const FString& S) { return TF(S, Display(12), C(0xB79CDB), false); }
}

TSharedRef<SWidget> SHollowUI::BuildPause()
{
	AHollowGameMode* G = GM.Get();
	struct FTab { EPauseTab Tab; const TCHAR* Name; };
	static const FTab Tabs[] = { { EPauseTab::Inventory, TEXT("Inventory  [I]") }, { EPauseTab::Character, TEXT("Character  [C]") },
		{ EPauseTab::Deck, TEXT("Deck") }, { EPauseTab::Journal, TEXT("Journal  [J]") }, { EPauseTab::Library, TEXT("Library") }, { EPauseTab::System, TEXT("System") } };
	TSharedRef<SHorizontalBox> Bar = SNew(SHorizontalBox);
	for (const FTab& Tb : Tabs)
	{
		const EPauseTab Which = Tb.Tab;
		const bool bOn = G->PauseTab == Which;
		Bar->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Btn(Tb.Name, [G, Which] { G->SetPauseTab(Which); }, true, bOn ? Gold : Panel2, bOn ? C(0x1B1206) : Fg, 13)];
	}
	TSharedPtr<SWidget> Content;
	switch (G->PauseTab)
	{
	case EPauseTab::Inventory: Content = BuildInventory(); break;
	case EPauseTab::Character: Content = BuildSheet(); break;
	case EPauseTab::Deck: Content = BuildDeck(); break;
	case EPauseTab::Journal: Content = BuildJournal(); break;
	case EPauseTab::Library: Content = BuildLibrary(); break;
	default: Content = BuildSystem(); break;
	}
	return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(FLinearColor(0.01f, 0.005f, 0.02f, 0.78f)).Padding(FMargin(40, 30))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 24, 0)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[TF(TEXT("PAUSED"), Display(30), Gold, false)]
					+ SVerticalBox::Slot().AutoHeight()[TF(FString::Printf(TEXT("Day %d  -  %s"), G->Day, *G->PC.Name), Body(13, true), Dim, false)]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Bar]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Btn(TEXT("Resume  [Esc]"), [G] { G->SetPaused(false); }, true, Wine, C(0xF6E7C8), 14)]
			]
			+ SVerticalBox::Slot().FillHeight(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)[Content.ToSharedRef()]
		];
}

TSharedRef<SWidget> SHollowUI::BuildSlot(int32 Bag, int32 Slot)
{
	AHollowGameMode* G = GM.Get();
	const FItemStack* S = G->PC.Inv.At(Bag, Slot);
	const FItemDef* D = S && !S->IsEmpty() ? HollowItems::Find(S->Id) : nullptr;
	const bool bSel = G->InvSel == FIntPoint(Bag, Slot);
	const bool bHit = !D || G->InvFilter < 0 || (int32)D->Kind == G->InvFilter;
	TSharedRef<SOverlay> O = SNew(SOverlay);
	O->AddSlot()[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(bSel ? Gold : D ? HollowItems::RarityColor(D->Rarity) * 0.55f : C(0x2A2230))];
	O->AddSlot().Padding(2)[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(C(0x120E16, 0.95f))];
	if (D)
	{
		if (const FSlateBrush* Ic = Sprite(D->Icon))
			O->AddSlot().Padding(10)[SNew(SImage).Image(Ic).ColorAndOpacity(D->Tint)];
		if (S->Count > 1) O->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(5, 3)[TF(FString::FromInt(S->Count), Display(13), Fg, false)];
		if (D->Charges > 0)
			O->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(6, 5)[SNew(SBox).WidthOverride(26).HeightOverride(4)[SNew(SProgressBar).Percent((float)S->Charges / D->Charges).FillColorAndOpacity(C(0xF29A3A))]];
		if (D->Grants.Num()) O->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(5, 4)[SNew(SBox).WidthOverride(7).HeightOverride(7)[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(Moss)]];
	}
	return SNew(SBox).WidthOverride(74).HeightOverride(74)
		[
			SNew(SButton).ButtonStyle(&BtnStyle()).ButtonColorAndOpacity(FLinearColor(1, 1, 1, 0.f)).ContentPadding(FMargin(0))
			.RenderOpacity(bHit ? 1.f : 0.25f)
			.ToolTipText(D ? FText::FromString(D->Name) : FText::GetEmpty())
			.OnClicked_Lambda([G, Bag, Slot] { G->InvClick(Bag, Slot); return FReply::Handled(); })
			[O]
		];
}

TSharedRef<SWidget> SHollowUI::BuildInventory()
{
	AHollowGameMode* G = GM.Get();
	const FInventory& Inv = G->PC.Inv;

	// filters and sort, like Nivalis' InventoryItemFilteringUi (it also sorts by name, recency, count, price)
	TSharedRef<SHorizontalBox> Filters = SNew(SHorizontalBox);
	auto Filt = [&](const FString& Label, int32 Kind)
	{
		const bool bOn = G->InvFilter == Kind;
		Filters->AddSlot().AutoWidth().Padding(0, 0, 4, 0)[Btn(Label, [G, Kind] { G->InvFilter = Kind; G->MarkUI(); }, true, bOn ? C(0x5A4220) : Panel2, bOn ? Gold : Dim, 11)];
	};
	Filt(TEXT("All"), -1);
	static const TCHAR* KindLabels[] = { TEXT("Tools"), TEXT("Consumables"), TEXT("Gear"), TEXT("Valuables"), TEXT("Quest") };
	for (int32 k = 0; k <= (int32)EItemKind::Quest; ++k) Filt(KindLabels[k], k);
	Filters->AddSlot().FillWidth(1.f)[SNew(SSpacer)];
	static const TCHAR* Sorts[] = { TEXT("Sort: kind"), TEXT("Sort: name"), TEXT("Sort: recent") };
	Filters->AddSlot().AutoWidth()[Btn(Sorts[G->InvSort], [G] { G->InvSortBy((G->InvSort + 1) % 3); }, true, Panel2, Fg, 11)];

	TSharedRef<SVerticalBox> Grids = SNew(SVerticalBox);
	Grids->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[Filters];
	for (int32 b = 0; b < Inv.Bags.Num(); ++b)
	{
		const FItemContainer& Bag = Inv.Bags[b];
		Grids->AddSlot().AutoHeight().Padding(0, b ? 12 : 0, 0, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Section(Bag.Name.ToUpper())]
			+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)[TF(FString::Printf(TEXT("%d / %d slots"), Bag.Used(), Bag.Slots.Num()), Body(13, true), Dim, false)]
		];
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(3));
		for (int32 i = 0; i < Bag.Slots.Num(); ++i) Grid->AddSlot(i % 8, i / 8)[BuildSlot(b, i)];
		Grids->AddSlot().AutoHeight()[Grid];
	}
	if (Inv.Bags.Num() < 2) Grids->AddSlot().AutoHeight().Padding(0, 10, 0, 0)[TF(TEXT("Carry a Bag of Holding to open a second container."), Body(13, true), Dim)];
	Grids->AddSlot().AutoHeight().Padding(0, 14, 0, 0)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[Chip(FString::Printf(TEXT("%d silver"), G->PC.Gold), C(0x6A4E1E))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(12, 0, 0, 0).VAlign(VAlign_Center)[TF(TEXT("Click an item to inspect it. With one selected, click an empty slot to move it."), Body(12, true), Dim, false)]
	];

	// details pane (Nivalis: ItemDetailsDisplayUI with a separate nothing-selected panel)
	TSharedRef<SVerticalBox> Det = SNew(SVerticalBox);
	const FItemStack* S = Inv.At(G->InvSel.X, G->InvSel.Y);
	const FItemDef* D = S && !S->IsEmpty() ? HollowItems::Find(S->Id) : nullptr;
	if (D)
	{
		Det->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 8)
		[
			SNew(SBox).WidthOverride(96).HeightOverride(96)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()[SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(HollowItems::RarityColor(D->Rarity) * 0.5f)]
				+ SOverlay::Slot().Padding(14)[Sprite(D->Icon) ? StaticCastSharedRef<SWidget>(SNew(SImage).Image(Sprite(D->Icon)).ColorAndOpacity(D->Tint)) : StaticCastSharedRef<SWidget>(SNew(SSpacer))]
			]
		];
		Det->AddSlot().AutoHeight()[TF(D->Name, Display(22), HollowItems::RarityColor(D->Rarity))];
		Det->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[TF(FString::Printf(TEXT("%s  -  %s%s"), *HollowItems::RarityName(D->Rarity), *HollowItems::KindName(D->Kind), S->Count > 1 ? *FString::Printf(TEXT("  -  x%d"), S->Count) : TEXT("")), Body(13, true), Dim)];
		Det->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[TF(D->Desc, Prose(16, true), Fg)];
		if (D->Grants.Num())
		{
			Det->AddSlot().AutoHeight().Padding(0, 0, 0, 4)[Section(TEXT("FIELD ABILITIES"))];
			TSharedRef<SWrapBox> Gr = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
			for (const FString& A : D->Grants) Gr->AddSlot()[Chip(HollowItems::AbilityName(A), C(0x3F5A2E))];
			Det->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[Gr];
		}
		if (D->Charges > 0) Det->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[TF(FString::Printf(TEXT("%d of %d uses left on this one."), S->Charges, D->Charges), Body(13), C(0xF29A3A))];
		if (D->BagSlots) Det->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[TF(FString::Printf(TEXT("Opens a %d-slot container while carried."), D->BagSlots), Body(13), C(0xB79CDB))];
		const int32 Sp = G->SellPrice(D->Id);
		Det->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[TF(Sp ? FString::Printf(TEXT("Traders pay about %d silver."), Sp) : FString(TEXT("No trader will buy this.")), Body(13, true), Dim)];
		TSharedRef<SHorizontalBox> Acts = SNew(SHorizontalBox);
		const bool bUsable = !D->Use.IsEmpty() || D->Grants.Num() > 0;
		Acts->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Btn(D->Grants.Num() ? FString::Printf(TEXT("Use here: %s"), *HollowItems::AbilityName(D->Grants[0])) : FString(TEXT("Use")), [G] { G->InvUse(); }, bUsable, Gold, C(0x1B1206))];
		if (Inv.Bags.Num() > 1 && D->Id != TEXT("bag")) Acts->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Btn(G->InvSel.X == 0 ? TEXT("To bag") : TEXT("To pack"), [G] { G->InvStow(); })];
		Acts->AddSlot().AutoWidth()[Btn(TEXT("Drop one"), [G] { G->InvDrop(); }, D->Kind != EItemKind::Quest, C(0x4A2622))];
		Det->AddSlot().AutoHeight()[Acts];
	}
	else
	{
		Det->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[TF(TEXT("Nothing selected"), Display(18), Dim)];
		Det->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[TF(TEXT("What you carry decides what you can do out there. A shovel digs, a torch lights and burns, a rope climbs."), Prose(15, true), Dim)];
		Det->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[Section(TEXT("YOUR FIELD ABILITIES"))];
		for (const FString& A : HollowItems::AbilityList())
		{
			const FItemDef* By = HollowItems::Find(Inv.GrantingItem(A));
			Det->AddSlot().AutoHeight().Padding(0, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)[Chip(HollowItems::AbilityName(A), By ? C(0x3F5A2E) : C(0x2A2430), By ? FLinearColor::White : Dim)]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[TF(By ? TEXT("from your ") + By->Name : FString(TEXT("no tool for this yet")), Body(13, !By), By ? Fg : Dim, false)]
			];
		}
	}

	return SNew(SBox).WidthOverride(1060)
		[
			Hide(SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 20, 0)[Grids]
				+ SHorizontalBox::Slot().FillWidth(1.f)[Panel(Det, C(0x120E16, 0.85f), FMargin(16, 14), false)],
			Leather(), C(0x5A4A40), FMargin(20, 18))
		];
}

TSharedRef<SWidget> SHollowUI::BuildJournal()
{
	AHollowGameMode* G = GM.Get();
	TSharedRef<SVerticalBox> J = SNew(SVerticalBox);
	for (const FString& E : G->Journal) J->AddSlot().AutoHeight().Padding(0, 2)[TF(E, Prose(16), ParchInk)];
	if (!G->Journal.Num()) J->AddSlot().AutoHeight()[TF(TEXT("Nothing written yet."), Prose(16, true), ParchDim)];
	TSharedRef<SVerticalBox> Side = SNew(SVerticalBox);
	Side->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[TF(TEXT("ACTIVE EFFECTS"), Display(13), Wine)];
	for (const FBuff& B : G->Buffs)
	{
		Side->AddSlot().AutoHeight()[TF(B.Name + (B.Fights > 0 ? FString::Printf(TEXT("  (%d fight%s)"), B.Fights, B.Fights > 1 ? TEXT("s") : TEXT("")) : FString(TEXT("  (today)"))), Display(13), B.Col * 0.6f)];
		Side->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[TF(B.Desc, Body(13, true), ParchDim)];
	}
	if (!G->Buffs.Num()) Side->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[TF(TEXT("None. Pray at a shrine, or level up."), Body(13, true), ParchDim)];
	Side->AddSlot().AutoHeight().Padding(0, 10, 0, 6)[TF(TEXT("ROADS YOU'VE LEARNED"), Display(13), Wine)];
	for (const FString& L : G->LeadsGiven)
		if (const FLeadDef* Ld = HollowData::FindLead(L)) Side->AddSlot().AutoHeight()[TF(Ld->Name, Body(14), ParchInk)];
	Side->AddSlot().AutoHeight().Padding(0, 10, 0, 0)[TF(FString::Printf(TEXT("%d topics you can raise in conversation."), G->KnownTopics.Num()), Body(13, true), ParchDim)];
	return SNew(SBox).WidthOverride(1000).MaxDesiredHeight(620)
		[
			Hide(SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 20, 0)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[TF(TEXT("Journal"), Display(26), Wine)]
					+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[J]]
				]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(300)[Side]],
			Paper(), C(0xEADBBD), FMargin(26, 22))
		];
}

TSharedRef<SWidget> SHollowUI::BuildSystem()
{
	AHollowGameMode* G = GM.Get();
	auto Row = [](const FString& K, const FString& V)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(150)[TF(K, Display(12), Gold, false)]]
			+ SHorizontalBox::Slot().FillWidth(1.f)[TF(V, Body(14), Fg)];
	};
	return SNew(SBox).WidthOverride(620)
		[
			Hide(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)[TF(TEXT("The world holds its breath."), Prose(20, true), Fg)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Row(TEXT("WASD / CLICK"), TEXT("walk"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Row(TEXT("E / SPACE"), TEXT("talk, examine, use a spot, skip a line"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Row(TEXT("1 - 9"), TEXT("pick a response in conversation"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Row(TEXT("I  C  J"), TEXT("inventory, character, journal"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Row(TEXT("M"), TEXT("the map of the March"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 16)[Row(TEXT("WHEEL"), TEXT("zoom"))]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)[Btn(TEXT("Resume"), [G] { G->SetPaused(false); }, true, Gold, C(0x1B1206), 15)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)[Btn(TEXT("Restart"), [G] { G->SetPaused(false); G->Restart(); }, true, Panel2, Fg, 15)]
					+ SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("Quit to desktop"), [G] { G->QuitToDesktop(); }, true, C(0x4A2622), Fg, 15)]
				],
			Leather(), C(0x5A4A40), FMargin(26, 22))
		];
}

/* ---------- Trade ---------- */

TSharedRef<SWidget> SHollowUI::BuildTrade()
{
	AHollowGameMode* G = GM.Get();
	const FTradeView& V = G->Trade.GetValue();
	const FNpcDef* N = HollowData::FindNpc(V.NpcId);
	if (!N) return SNew(SSpacer);
	auto ItemRow = [&](const FItemDef& D, const FString& Right, const FString& Btn_, bool bOk, TFunction<void()> Fn)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
			[SNew(SBox).WidthOverride(34).HeightOverride(34)[Sprite(D.Icon) ? StaticCastSharedRef<SWidget>(SNew(SImage).Image(Sprite(D.Icon)).ColorAndOpacity(D.Tint)) : StaticCastSharedRef<SWidget>(SNew(SSpacer))]]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[TF(D.Name, Display(13), HollowItems::RarityColor(D.Rarity), false)]
				+ SVerticalBox::Slot().AutoHeight()[TF(D.Grants.Num() ? TEXT("Grants: ") + HollowItems::AbilityName(D.Grants[0]) : HollowItems::KindName(D.Kind), Body(12, true), Dim, false)]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)[TF(Right, Display(13), Gold, false)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Btn(Btn_, Fn, bOk, bOk ? C(0x5A4220) : Panel2, Fg, 12)];
	};
	TSharedRef<SVerticalBox> Wares = SNew(SVerticalBox);
	for (const FString& Id : N->Stock)
		if (const FItemDef* D = HollowItems::Find(Id))
			Wares->AddSlot().AutoHeight().Padding(0, 3)[ItemRow(*D, FString::Printf(TEXT("%d"), D->Price), TEXT("Buy"), G->PC.Gold >= D->Price, [G, Id] { G->Buy(Id); })];
	TSharedRef<SVerticalBox> Mine = SNew(SVerticalBox);
	TArray<FString> Seen;
	for (const FItemContainer& B : G->PC.Inv.Bags)
		for (const FItemStack& S : B.Slots)
		{
			if (S.IsEmpty() || Seen.Contains(S.Id)) continue;
			Seen.Add(S.Id);
			const FItemDef* D = HollowItems::Find(S.Id);
			const int32 P = G->SellPrice(S.Id);
			if (!D || P <= 0) continue;
			const FString Id = S.Id;
			Mine->AddSlot().AutoHeight().Padding(0, 3)[ItemRow(*D, FString::Printf(TEXT("x%d   %d"), G->PC.Count(Id), P), TEXT("Sell"), true, [G, Id] { G->Sell(Id); })];
		}
	if (!Seen.Num()) Mine->AddSlot().AutoHeight()[TF(TEXT("Nothing to sell."), Body(13, true), Dim)];
	return SNew(SBox).WidthOverride(940)
		[
			Hide(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)[TF(FString::Printf(TEXT("Trading with %s"), *N->Name), Display(22), Gold, false)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)[Chip(FString::Printf(TEXT("%d silver"), G->PC.Gold), C(0x6A4E1E))]
					+ SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("Done  [Esc]"), [G] { G->CloseTrade(); })]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 12)[TF(TEXT("“") + V.Line + TEXT("”"), Prose(18, true), Fg)]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 16, 0)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[Section(TEXT("WARES"))]
						+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).MaxDesiredHeight(420)[SNew(SScrollBox) + SScrollBox::Slot()[Wares]]]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[Section(TEXT("YOUR GOODS"))]
						+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).MaxDesiredHeight(420)[SNew(SScrollBox) + SScrollBox::Slot()[Mine]]]
					]
				],
			Leather(), C(0x5A4A40), FMargin(22, 18))
		];
}

/* ---------- Level up ---------- */

TSharedRef<SWidget> SHollowUI::BuildLevelUp()
{
	AHollowGameMode* G = GM.Get();
	const FLevelUpView& V = G->LevelUp.GetValue();
	const FPC& P = G->PC;
	TSharedRef<SVerticalBox> Page = SNew(SVerticalBox);
	Page->AddSlot().AutoHeight().HAlign(HAlign_Center)[TF(FString::Printf(TEXT("LEVEL %d"), V.Level), Display(44), Gold, false)];
	Page->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 12)[TF(FString::Printf(TEXT("%s the %s grows. +%d HP."), *P.Name, *P.ClassName, P.HpPer), Prose(18, true), Fg, false)];

	// a selectable row used by every picker
	auto Pick = [](const FString& Title, const FString& Sub, bool bOn, bool bEnabled, TFunction<void()> Fn, float W = 0.f) -> TSharedRef<SWidget>
	{
		TSharedRef<SWidget> B = SNew(SButton).ButtonStyle(&BtnStyle()).ButtonColorAndOpacity(bOn ? Gold : Panel2).ContentPadding(FMargin(8, 5)).IsEnabled(bEnabled)
			.ToolTipText(FText::FromString(Sub))
			.OnClicked_Lambda([Fn] { if (GUiGM.IsValid()) GUiGM->PlaySfx(TEXT("click"), 0.5f); Fn(); return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[TF(Title, Display(13), bOn ? C(0x1B1206) : Fg, false)]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Sub.Left(110))).Font(Body(12, true)).ColorAndOpacity(FSlateColor(bOn ? C(0x1B1206) : Dim)).AutoWrapText(true)]
			];
		return W > 0.f ? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(W)[B]) : B;
	};
	auto Heading = [&](const FString& S) { Page->AddSlot().AutoHeight().Padding(0, 8, 0, 6)[Section(S)]; };

	if (V.BoostsNeeded)
	{
		Heading(FString::Printf(TEXT("ABILITY BOOSTS  %d / %d  -  four different abilities; at +4 or more a boost is partial, two make +1"), V.Boosts.Num(), V.BoostsNeeded));
		TSharedRef<SHorizontalBox> Ab = SNew(SHorizontalBox);
		for (const TCHAR* K : { TEXT("str"), TEXT("dex"), TEXT("con"), TEXT("int"), TEXT("wis"), TEXT("cha") })
		{
			const FString Id = K;
			const int32 Cur = P.Abil.FindRef(Id);
			const bool bOn = V.Boosts.Contains(Id);
			Ab->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Pick(Id.ToUpper(), Cur >= 4 ? FString::Printf(TEXT("%s (partial)"), *Sgn(Cur)) : FString::Printf(TEXT("%s > %s"), *Sgn(Cur), *Sgn(Cur + 1)),
				bOn, bOn || V.Boosts.Num() < V.BoostsNeeded, [G, Id] { G->PickBoost(Id); }, 118.f)];
		}
		Page->AddSlot().AutoHeight()[Ab];
	}
	if (V.bSkill)
	{
		Heading(TEXT("SKILL INCREASE  -  expert from level 3, master from 7, legendary from 15"));
		static const TCHAR* Rk[] = { TEXT("Untrained"), TEXT("Trained"), TEXT("Expert"), TEXT("Master"), TEXT("Legendary") };
		TSharedRef<SWrapBox> Sk = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6, 6));
		for (const FString& S : SkillList())
		{
			const int32 R = P.Ranks.FindRef(S);
			const bool bCan = R < G->MaxRankAt(V.Level);
			const FString Id = S;
			Sk->AddSlot()[Pick(SkillName(S), bCan ? FString::Printf(TEXT("%s > %s"), Rk[R], Rk[FMath::Min(4, R + 1)]) : FString::Printf(TEXT("%s (max for now)"), Rk[R]),
				V.Skill == S, bCan, [G, Id] { G->PickSkillUp(Id); }, 176.f)];
		}
		Page->AddSlot().AutoHeight()[Sk];
	}
	// feats from the rules library, filtered to what this character can take at this level
	auto Feats = [&](const FString& Slot, const FString& Title, const FString& Chosen)
	{
		Heading(FString::Printf(TEXT("%s%s"), *Title, Chosen.IsEmpty() ? TEXT("") : *(TEXT("  -  ") + Chosen)));
		TSharedRef<SWrapBox> W = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6, 6));
		const TArray<const FLibFeat*> Opts = G->FeatOptions(Slot);
		for (const FLibFeat* F : Opts)
		{
			const FString Name = F->Name;
			const FString Act = F->Action == TEXT("passive") ? FString() : F->Action == TEXT("action") ? FString::Printf(TEXT("[%d] "), FMath::Max(1, F->Actions)) : FString::Printf(TEXT("[%s] "), *F->Action);
			W->AddSlot()[Pick(FString::Printf(TEXT("%s  %d"), *Name, F->Level), Act + F->Text, Chosen == Name, true, [G, Slot, Name] { G->PickFeat(Slot, Name); }, 270.f)];
		}
		if (!Opts.Num()) W->AddSlot()[TF(TEXT("No matching feats in the rules library."), Body(13, true), Dim)];
		Page->AddSlot().AutoHeight()[SNew(SBox).MaxDesiredHeight(190)[SNew(SScrollBox) + SScrollBox::Slot()[W]]];
	};
	if (V.bClassFeat) Feats(TEXT("class"), FString::Printf(TEXT("%s FEAT"), *P.ClassName.ToUpper()), V.ClassFeat);
	if (V.bAncestryFeat) Feats(TEXT("ancestry"), FString::Printf(TEXT("%s FEAT"), *P.Ancestry.ToUpper()), V.AncestryFeat);
	if (V.bSkillFeat) Feats(TEXT("skill"), TEXT("SKILL FEAT"), V.SkillFeat);
	if (V.bGeneralFeat) Feats(TEXT("general"), TEXT("GENERAL FEAT"), V.GeneralFeat);
	if (V.SpellsNeeded)
	{
		Heading(FString::Printf(TEXT("NEW SPELLS  %d / %d  -  %s, they join your deck as cards"), V.NewSpells.Num(), V.SpellsNeeded, *G->CasterTradition()));
		TSharedRef<SWrapBox> W = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6, 6));
		for (const FLibSpell* S : G->SpellOptions())
		{
			const FString Name = S->Name;
			const bool bOn = V.NewSpells.Contains(Name);
			W->AddSlot()[Pick(FString::Printf(TEXT("%s  (rank %d)"), *Name, S->Rank), HollowLib::SpellSummary(*S, S->Rank) + TEXT(". ") + S->Text, bOn, bOn || V.NewSpells.Num() < V.SpellsNeeded, [G, Name] { G->PickSpell(Name); }, 270.f)];
		}
		Page->AddSlot().AutoHeight()[SNew(SBox).MaxDesiredHeight(190)[SNew(SScrollBox) + SScrollBox::Slot()[W]]];
	}
	if (!V.BoostsNeeded && !V.bSkill && !V.bClassFeat && !V.bAncestryFeat && !V.bSkillFeat && !V.bGeneralFeat && !V.SpellsNeeded)
		Page->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8)[TF(TEXT("Nothing to choose at this level: more hit points and sharper proficiency."), Body(14, true), Dim)];
	Page->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 0)[Btn(TEXT("Confirm"), [G] { G->ConfirmLevelUp(); }, V.IsComplete(), Wine, C(0xF6E7C8), 17)];

	return SNew(SBox).WidthOverride(1180).MaxDesiredHeight(860)
		[
			Hide(SNew(SScrollBox) + SScrollBox::Slot()[Page], Leather(), C(0x5A4A40), FMargin(24, 20))
		];
}

/* ---------- HUD: field abilities and buffs ---------- */

TSharedRef<SWidget> SHollowUI::BuildAbilities()
{
	AHollowGameMode* G = GM.Get();
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox).Visibility(EVisibility::SelfHitTestInvisible);
	for (const FString& A : HollowItems::AbilityList())
	{
		const bool bHas = G->HasAbility(A);
		Row->AddSlot().AutoWidth().Padding(0, 0, 4, 0)[Chip(HollowItems::AbilityName(A), bHas ? C(0x3F5A2E, 0.9f) : C(0x1D1724, 0.6f), bHas ? FLinearColor::White : C(0x6A5E66))];
	}
	for (const FBuff& B : G->Buffs)
		Row->AddSlot().AutoWidth().Padding(8, 0, 0, 0)[Chip(B.Name, B.Col * 0.45f)];
	// stealth readout: light where you stand, whether anything sees you, and your cover from it
	const FLinearColor VisCol = G->HeroVis == AHollowGameMode::EVis::Observed ? C(0x7A2620) : G->HeroVis == AHollowGameMode::EVis::Concealed ? C(0x6A4E1E) : C(0x2E4A2A);
	Row->AddSlot().AutoWidth().Padding(12, 0, 0, 0)[Chip(G->VisLabel() + (G->bNoticed ? TEXT("  (spotted)") : TEXT("")) + (G->bLanternLit ? TEXT("") : TEXT("  lantern hooded")), VisCol)];
	return Row;
}

/* ---------- Library: the whole rules compendium and the campaign's own journals ---------- */

TSharedRef<SWidget> SHollowUI::BuildLibrary()
{
	AHollowGameMode* G = GM.Get();
	if (!HollowLib::Load())
		return SNew(SBox).WidthOverride(700)[Hide(TF(TEXT("No rules library yet. Export your Foundry VTT packs (ExternalAssets/FoundryVTT/unpack.mjs), then run:\n\n    node Tools/BuildLibrary.mjs\n\nIt writes ExternalAssets/Library/library.json, which stays on this machine."), Body(15), Fg), Leather(), C(0x5A4A40), FMargin(24, 20))];

	struct FRow { FString Name, Sub, Text; };
	TArray<FRow> Rows;
	const FString Q = G->LibQuery.ToLower();
	auto Keep = [&](const FString& Name) { return Q.IsEmpty() || Name.ToLower().Contains(Q); };
	auto Add = [&](const FString& Name, const FString& Sub, const FString& Text) { if (Keep(Name) && Rows.Num() < 300) Rows.Add({ Name, Sub, Text }); };
	const FString& Cat = G->LibCat;
	if (Cat == TEXT("Spells")) for (const FLibSpell& S : HollowLib::Spells()) Add(S.Name, FString::Printf(TEXT("%s %d  -  %s"), S.bCantrip ? TEXT("Cantrip") : S.bFocus ? TEXT("Focus") : TEXT("Rank"), S.Rank, *FString::Join(S.Traditions, TEXT(", "))), HollowLib::SpellSummary(S, S.Rank) + TEXT("\n\n") + S.Text);
	else if (Cat == TEXT("Feats")) for (const FLibFeat& F : HollowLib::Feats()) Add(F.Name, FString::Printf(TEXT("%s feat %d"), *F.Category, F.Level), (F.Prereq.Num() ? TEXT("Prerequisites: ") + FString::Join(F.Prereq, TEXT("; ")) + TEXT("\n\n") : FString()) + F.Text);
	else if (Cat == TEXT("Items")) for (const FLibItem& I : HollowLib::Items()) Add(I.Name, FString::Printf(TEXT("%s, level %d, %g silver"), *I.Type, I.Level, I.Price), (I.Type == TEXT("weapon") ? FString::Printf(TEXT("%d%s %s, %s %s.\n\n"), I.Dice, *I.Die, *I.DmgType, *I.Category, *I.Group) : I.Type == TEXT("armor") ? FString::Printf(TEXT("+%d AC, Dex cap %d, Str %d.\n\n"), I.Ac, I.DexCap, I.Str) : FString()) + I.Text);
	else if (Cat == TEXT("Classes")) for (const FLibClass& Cl : HollowLib::Classes()) { FString F; for (const auto& Ft : Cl.Features) F += FString::Printf(TEXT("%d  %s\n"), Ft.Key, *Ft.Value); Add(Cl.Name, FString::Printf(TEXT("%d HP, key %s"), Cl.Hp, *FString::Join(Cl.KeyAbility, TEXT("/"))), Cl.Text + TEXT("\n\n") + F); }
	else if (Cat == TEXT("Ancestries")) for (const FLibAncestry& A : HollowLib::Ancestries()) Add(A.Name, FString::Printf(TEXT("%d HP, %d ft, %s"), A.Hp, A.Speed, *A.Vision), A.Text);
	else if (Cat == TEXT("Conditions")) for (const FLibEntry& E : HollowLib::Conditions()) Add(E.Name, TEXT("condition"), E.Text);
	else if (Cat == TEXT("Actions")) for (const FLibEntry& E : HollowLib::Actions()) Add(E.Name, TEXT("action"), E.Text);
	else if (Cat == TEXT("Deities")) for (const FLibEntry& E : HollowLib::Deities()) Add(E.Name, TEXT("deity"), E.Text);
	else for (const FLibJournal& J : HollowLib::Journals()) { FString T; for (const auto& P : J.Pages) T += (P.Key.IsEmpty() ? FString() : P.Key.ToUpper() + TEXT("\n")) + P.Value + TEXT("\n\n"); Add(J.Name, FString::Printf(TEXT("%d pages"), J.Pages.Num()), T); }

	TSharedRef<SVerticalBox> Cats = SNew(SVerticalBox);
	for (const TCHAR* Cn : { TEXT("Journals"), TEXT("Spells"), TEXT("Feats"), TEXT("Items"), TEXT("Classes"), TEXT("Ancestries"), TEXT("Conditions"), TEXT("Actions"), TEXT("Deities") })
	{
		const FString Name = Cn;
		const bool bOn = Cat == Name;
		Cats->AddSlot().AutoHeight().Padding(0, 2)[Btn(Name, [G, Name] { G->LibCat = Name; G->LibSel.Reset(); G->MarkUI(); }, true, bOn ? Gold : Panel2, bOn ? C(0x1B1206) : Fg, 13)];
	}
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	const FRow* Sel = nullptr;
	for (const FRow& R : Rows)
	{
		const bool bOn = R.Name == G->LibSel;
		if (bOn) Sel = &R;
		const FString Name = R.Name;
		List->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SButton).ButtonStyle(&BtnStyle()).ButtonColorAndOpacity(bOn ? C(0x5A4220) : C(0x1A1520, 0.7f)).ContentPadding(FMargin(8, 3))
			.OnClicked_Lambda([G, Name] { G->LibSel = Name; G->MarkUI(); return FReply::Handled(); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)[TF(R.Name, Body(14), bOn ? Gold : Fg, false)]
				+ SHorizontalBox::Slot().AutoWidth()[TF(R.Sub, Body(12, true), Dim, false)]
			]
		];
	}
	if (!Rows.Num()) List->AddSlot().AutoHeight()[TF(TEXT("Nothing matches."), Body(13, true), Dim)];

	return SNew(SBox).WidthOverride(1240).HeightOverride(700)
		[
			Hide(SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 14, 0)[SNew(SBox).WidthOverride(150)[Cats]]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 14, 0)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
					[
						SNew(SEditableTextBox).Text(FText::FromString(G->LibQuery)).Font(Body(14)).HintText(FText::FromString(TEXT("Search by name...")))
						.OnTextCommitted_Lambda([G](const FText& Tx, ETextCommit::Type) { G->LibQuery = Tx.ToString(); G->MarkUI(); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)[TF(FString::Printf(TEXT("%d shown%s"), Rows.Num(), Rows.Num() >= 300 ? TEXT(" (search to narrow)") : TEXT("")), Body(12, true), Dim, false)]
					+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[List]]
				]
				+ SHorizontalBox::Slot().FillWidth(1.2f)
				[
					Panel(SNew(SScrollBox) + SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[TF(Sel ? Sel->Name : FString(TEXT("Pick an entry")), Display(22), Gold)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[TF(Sel ? Sel->Sub : FString(), Body(13, true), Dim)]
						+ SVerticalBox::Slot().AutoHeight()[TF(Sel ? Sel->Text : FString(TEXT("Everything your Foundry world knows: the pf2e rules compendium and your own campaign journals, handouts and GM notes.")), Body(15), Fg)]
					], C(0x120E16, 0.85f), FMargin(16, 14), false)
				],
			Leather(), C(0x5A4A40), FMargin(18, 16))
		];
}
