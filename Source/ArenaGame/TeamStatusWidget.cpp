#include "TeamStatusWidget.h"
#include "ArenaGameState.h"
#include "LootCollectorComponent.h"
#include "LootUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"
#include "Engine/World.h"

void UTeamStatusWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetVisibility(ESlateVisibility::HitTestInvisible);
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
    Card->SetBrush(LootUITheme::CardBrush());
    Card->SetBrushColor(LootUITheme::Panel);
    Card->SetPadding(FMargin(22.f, 12.f));
    UCanvasPanelSlot* CardSlot = Canvas->AddChildToCanvas(Card);
    CardSlot->SetAnchors(FAnchors(0.5f, 0.f));
    CardSlot->SetAlignment(FVector2D(0.5f, 0.f));
    CardSlot->SetPosition(FVector2D(0.f, 24.f));
    CardSlot->SetAutoSize(true);
    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
    Card->SetContent(Stack);
    Progress = WidgetTree->ConstructWidget<UTextBlock>();
    Progress->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 18));
    Progress->SetColorAndOpacity(FSlateColor(LootUITheme::Gold));
    Progress->SetJustification(ETextJustify::Center);
    Stack->AddChild(Progress);
    Status = WidgetTree->ConstructWidget<UTextBlock>();
    Status->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
    Status->SetColorAndOpacity(FSlateColor(LootUITheme::Muted));
    Status->SetJustification(ETextJustify::Center);
    Stack->AddChild(Status);
}

void UTeamStatusWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    const AArenaGameState* Team = GetWorld()->GetGameState<AArenaGameState>();
    APlayerController* PC = GetOwningPlayer();
    if (!Team || !PC) return;
    SetRenderOpacity(Team->bTeamComplete ? 0.f : 1.f);
    Progress->SetText(FText::FromString(FString::Printf(TEXT("ESCAPED %d/%d   |   DEAD %d   |   PLAYING %d"),
        Team->EscapedPlayers.Num(), Team->TeamSize, Team->DeadPlayers.Num(), FMath::Max(0, Team->ConnectedPlayers - Team->EscapedPlayers.Num() - Team->DeadPlayers.Num()))));
    const ULootCollectorComponent* Loot = PC->GetPawn() ? PC->GetPawn()->FindComponentByClass<ULootCollectorComponent>() : nullptr;
    FString Label = Team->ConnectedPlayers < Team->TeamSize ? TEXT("WAITING FOR TEAMMATE") : TEXT("TEAM EXTRACTION");
    if ((Loot && Loot->HasEscaped()) || Team->DeadPlayers.Contains(PC->PlayerState))
    {
        const APawn* Target = Cast<APawn>(PC->GetViewTarget());
        const APlayerState* State = Target ? Target->GetPlayerState() : nullptr;
        Label = FString::Printf(TEXT("SPECTATING: %s   |   TEAM $%lld"), State ? *State->GetPlayerName() : TEXT("Waiting"), Team->GetTeamTotalValue());
    }
    Status->SetText(FText::FromString(Label));
    if (Team->bTeamFailed)
    {
        Progress->SetText(NSLOCTEXT("Team", "Failed", "TEAM FAILED"));
        Status->SetText(Team->TeamFailureReason);
    }
}
