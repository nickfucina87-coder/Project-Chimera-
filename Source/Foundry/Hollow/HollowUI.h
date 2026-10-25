#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AHollowGameMode;
class SEditableTextBox;
struct FCardDef;

// The whole game UI in Slate, rebuilt from AHollowGameMode state whenever it marks itself dirty.
class SHollowUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHollowUI) {}
		SLATE_ARGUMENT(AHollowGameMode*, GM)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Rebuild();

private:
	TSharedRef<SWidget> BuildTitle();
	TSharedRef<SWidget> BuildCreate();
	TSharedRef<SWidget> BuildHud();
	TSharedRef<SWidget> BuildExplore();
	TSharedRef<SWidget> BuildMap();
	TSharedRef<SWidget> BuildCombat();
	TSharedRef<SWidget> BuildEnd();
	TSharedRef<SWidget> BuildEvent();
	TSharedRef<SWidget> BuildTalk();
	TSharedRef<SWidget> BuildReward();
	TSharedRef<SWidget> BuildSheet();
	TSharedRef<SWidget> BuildDeck();
	TSharedRef<SWidget> BuildCard(const FCardDef& Card, bool bEnabled, TFunction<void()> OnClick, float Scale = 1.f, int32 Serial = -1, int32 Index = 0);
	TSharedRef<SWidget> BuildNote();

	TWeakObjectPtr<AHollowGameMode> GM;
	TSharedPtr<SEditableTextBox> AskBox;
};
