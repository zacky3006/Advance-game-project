#include "LootCollectorComponent.h"
#include "LootPickup.h"
#include "LootHUDWidget.h"
#include "LootInventoryWidget.h"
#include "LootUITheme.h"
#include "EscapePoint.h"
#include "EscapeSequenceWidget.h"
#include "EngineUtils.h"
#include "HealthComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Styling/CoreStyle.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ULootCollectorComponent::ULootCollectorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // Track the item smoothly as the third-person camera moves.
    PrimaryComponentTick.TickInterval = 0.f;
    SetIsReplicatedByDefault(true);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/UI/Materials/M_LootOutline.M_LootOutline"));
    OutlineMaterial = Material.Object;
}

int64 ULootCollectorComponent::GetTotalValue() const
{
    int64 Total = 0;
    for (const FLootEntry& Entry : Inventory)
    {
        Total += Entry.Value;
    }
    return Total;
}

bool ULootCollectorComponent::CanCollect(const ALootPickup* Pickup) const
{
    if (bEndingGame) return false;
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn || !Pawn->IsPlayerControlled() || !IsValid(Pickup) || Pickup->IsActorBeingDestroyed())
    {
        return false;
    }
    const UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
    if ((Health && Health->IsDead()) || FVector::DistSquared(Pawn->GetActorLocation(), Pickup->GetActorLocation()) > FMath::Square(FMath::Max(1.f, PickupRange)))
    {
        return false;
    }
    // Trace from the character, so a third-person camera cannot reach through walls.
    FCollisionQueryParams Params(SCENE_QUERY_STAT(LootVisibility), false, Pawn);
    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Pawn->GetPawnViewLocation(), Pickup->GetActorLocation(), ECC_Visibility, Params);
    return !bHit || Hit.GetActor() == Pickup;
}

ALootPickup* ULootCollectorComponent::FindPickup() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!PC) return nullptr;
    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
    const FVector CameraForward = CameraRotation.Vector();
    const double MinDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(FocusHalfAngle, 5.f, 89.f)));
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    GetWorld()->OverlapMultiByObjectType(Overlaps, GetOwner()->GetActorLocation(), FQuat::Identity, Objects,
        FCollisionShape::MakeSphere(FMath::Max(1.f, PickupRange)), FCollisionQueryParams(SCENE_QUERY_STAT(LootSearch), false, GetOwner()));
    ALootPickup* Best = nullptr;
    double BestScore = TNumericLimits<double>::Max();
    for (const FOverlapResult& Overlap : Overlaps)
    {
        ALootPickup* Pickup = Cast<ALootPickup>(Overlap.GetActor());
        if (CanCollect(Pickup))
        {
            const FVector AimPoint = Pickup->Mesh->Bounds.Origin;
            const double AimDot = FVector::DotProduct(CameraForward, (AimPoint - CameraLocation).GetSafeNormal());
            if (AimDot < MinDot) continue;
            // A reachable item may still be behind a wall from the camera's perspective.
            FHitResult CameraHit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(LootCameraVisibility), false, Pawn);
            if (GetWorld()->LineTraceSingleByChannel(CameraHit, CameraLocation, AimPoint, ECC_Visibility, Params)
                && CameraHit.GetActor() != Pickup) continue;
            const double Distance = FVector::Distance(GetOwner()->GetActorLocation(), Pickup->GetActorLocation());
            double Score = (1.0 - AimDot) + 0.12 * Distance / FMath::Max(1.f, PickupRange);
            // Keep the current item for tiny camera movements instead of flickering between neighbors.
            if (Pickup == FocusedPickup.Get()) Score -= 0.025;
            if (Score < BestScore)
            {
                Best = Pickup;
                BestScore = Score;
            }
        }
    }
    return Best;
}

void ULootCollectorComponent::UpdateFocusedPickup()
{
    ALootPickup* Target = FindPickup();
    FocusedPickup = Target;
    if (!Target)
    {
        if (OutlineMesh) OutlineMesh->SetVisibility(false);
        return;
    }
    if (!OutlineMesh && OutlineMaterial)
    {
        // One local-only hull per collector. Never changes the item's original materials.
        OutlineMesh = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("LootFocusOutline"));
        OutlineMesh->SetMobility(EComponentMobility::Movable);
        OutlineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        OutlineMesh->SetGenerateOverlapEvents(false);
        OutlineMesh->SetCanEverAffectNavigation(false);
        OutlineMesh->SetCastShadow(false);
        OutlineMesh->SetOnlyOwnerSee(true);
        OutlineMesh->bReceivesDecals = false;
        OutlineMesh->SetBoundsScale(1.2f);
        OutlineMesh->RegisterComponent();
    }
    if (OutlineMesh)
    {
        if (OutlineMesh->GetStaticMesh() != Target->Mesh->GetStaticMesh())
        {
            OutlineMesh->SetStaticMesh(Target->Mesh->GetStaticMesh());
            for (int32 Index = 0; Index < OutlineMesh->GetNumMaterials(); ++Index)
            {
                OutlineMesh->SetMaterial(Index, OutlineMaterial);
            }
        }
        OutlineMesh->SetWorldTransform(Target->Mesh->GetComponentTransform());
        OutlineMesh->SetVisibility(true);
    }
}

void ULootCollectorComponent::TryInteract()
{
    if (bEndingGame || IsInventoryOpen()) return;
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (Pawn && Pawn->IsLocallyControlled())
    {
        // Collect exactly the item whose prompt and outline the player is seeing.
        if (ALootPickup* Pickup = FocusedPickup.Get(); CanCollect(Pickup))
        {
            ServerCollect(Pickup);
        }
    }
}

void ULootCollectorComponent::TryEscape()
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn || !Pawn->IsLocallyControlled() || bEndingGame || IsInventoryOpen()) return;
    AEscapePoint* Best = nullptr;
    double BestDistance = TNumericLimits<double>::Max();
    for (TActorIterator<AEscapePoint> It(GetWorld()); It; ++It)
    {
        const double Distance = FVector::DistSquared(Pawn->GetActorLocation(), It->GetActorLocation());
        if (It->CanInteract(this) && It->HasEnoughLoot(this) && Distance < BestDistance)
        {
            Best = *It;
            BestDistance = Distance;
        }
    }
    if (Best) ServerEscape(Best);
}

void ULootCollectorComponent::ServerEscape_Implementation(AEscapePoint* Point)
{
    // Recheck the authoritative inventory, reach, health and line of sight.
    if (bEndingGame || !IsValid(Point) || !Point->CanInteract(this) || !Point->HasEnoughLoot(this)) return;
    bEndingGame = true;
    ClientBeginEscape(Point->EndingMovie);
}

void ULootCollectorComponent::ClientBeginEscape_Implementation(const FString& Movie)
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!PC || !PC->IsLocalController()) return;
    bEndingGame = true;
    CloseInventory();
    if (UEscapeSequenceWidget* Ending = CreateWidget<UEscapeSequenceWidget>(PC, UEscapeSequenceWidget::StaticClass()))
    {
        // Snapshot at extraction: exclude fading, the ending movie and result-screen time.
        // DemoMap's world clock begins after the opening movie and stops while paused.
        Ending->SetRunResult(Inventory, GetWorld()->GetTimeSeconds());
        Ending->AddToPlayerScreen(500);
        Ending->BeginEnding(Movie);
    }
}

void ULootCollectorComponent::ServerCollect_Implementation(ALootPickup* Pickup)
{
    if (CanCollect(Pickup))
    {
        Pickup->TryCollect(this);
    }
}

void ULootCollectorComponent::RecordLoot(const FText& Name, int32 Value)
{
    if (!GetOwner()->HasAuthority()) return;
    FLootEntry Entry;
    Entry.ItemName = Name;
    Entry.Value = FMath::Max(0, Value);
    Inventory.Add(Entry);
    OnInventoryChanged.Broadcast();
    GetOwner()->ForceNetUpdate();
}

void ULootCollectorComponent::RecordPickup(const ALootPickup* Pickup)
{
    if (!GetOwner()->HasAuthority() || !IsValid(Pickup)) return;
    FLootEntry Entry;
    Entry.ItemName = Pickup->ItemName;
    Entry.Value = FMath::Max(0, Pickup->Value);
    Entry.Icon = Pickup->ItemIcon;
    Entry.PreviewMesh = Pickup->Mesh->GetStaticMesh();
    Entry.PreviewMaterials = Pickup->Mesh->GetMaterials();
    Inventory.Add(MoveTemp(Entry));
    OnInventoryChanged.Broadcast();
    if (IsInventoryOpen()) InventoryWidget->RefreshInventory();
    GetOwner()->ForceNetUpdate();
}

bool ULootCollectorComponent::IsInventoryOpen() const
{
    return InventoryWidget && InventoryWidget->IsInViewport();
}

void ULootCollectorComponent::ToggleInventory()
{
    if (bEndingGame) return;
    APawn* Pawn = Cast<APawn>(GetOwner());
    APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!PC || !PC->IsLocalPlayerController()) return;
    if (IsInventoryOpen())
    {
        CloseInventory();
        return;
    }
    if (!InventoryWidget)
    {
        InventoryWidget = CreateWidget<ULootInventoryWidget>(PC, ULootInventoryWidget::StaticClass());
        if (!InventoryWidget) return;
        InventoryWidget->Collector = this;
    }
    InventoryWidget->RefreshInventory();
    InventoryWidget->AddToPlayerScreen(50);
    InventoryController = PC;
    bPreviousMouseCursor = PC->bShowMouseCursor;
    PC->bShowMouseCursor = true;
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(InventoryWidget->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(Mode);
    InventoryWidget->SetKeyboardFocus();
    FocusedPickup.Reset();
    if (OutlineMesh) OutlineMesh->SetVisibility(false);
    if (HUD) HUD->SetVisibility(ESlateVisibility::Collapsed);
}

void ULootCollectorComponent::CloseInventory()
{
    if (!IsInventoryOpen()) return;
    InventoryWidget->RemoveFromParent();
    APlayerController* PC = InventoryController.Get();
    if (PC)
    {
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
        PC->bShowMouseCursor = bPreviousMouseCursor;
        PC->SetInputMode(FInputModeGameOnly());
    }
    InventoryController.Reset();
}

void ULootCollectorComponent::OnRep_Inventory()
{
    OnInventoryChanged.Broadcast();
    if (IsInventoryOpen()) InventoryWidget->RefreshInventory();
}

void ULootCollectorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ULootCollectorComponent, Inventory, COND_OwnerOnly);
}

void ULootCollectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (Pawn && Pawn->IsLocallyControlled())
    {
        if (IsInventoryOpen()) return;
        UpdateFocusedPickup();
        UpdateHUD();
    }
    else
    {
        CloseInventory();
        FocusedPickup.Reset();
        if (OutlineMesh) OutlineMesh->SetVisibility(false);
        if (HUD) HUD->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ULootCollectorComponent::UpdateHUD()
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!PC || !PC->IsLocalPlayerController()) return;
    if (!HUD)
    {
        HUD = CreateWidget<ULootHUDWidget>(PC, ULootHUDWidget::StaticClass());
        if (!HUD) return;
        UCanvasPanel* Canvas = HUD->WidgetTree->ConstructWidget<UCanvasPanel>();
        HUD->WidgetTree->RootWidget = Canvas;
        const FSlateBrush CardBrush = LootUITheme::CardBrush();

        LootPanel = HUD->WidgetTree->ConstructWidget<UBorder>();
        LootPanel->SetBrush(CardBrush);
        LootPanel->SetPadding(FMargin(22.f, 16.f));
        UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(LootPanel);
        Slot->SetAnchors(FAnchors(1.f, 0.f));
        Slot->SetAlignment(FVector2D(1.f, 0.f));
        Slot->SetPosition(FVector2D(-32.f, 32.f));
        Slot->SetAutoSize(true);
        UVerticalBox* PanelContents = HUD->WidgetTree->ConstructWidget<UVerticalBox>();
        LootPanel->SetContent(PanelContents);
        UTextBlock* Heading = HUD->WidgetTree->ConstructWidget<UTextBlock>();
        Heading->SetText(NSLOCTEXT("Loot", "InventoryTitle", "YOUR HAUL"));
        Heading->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
        Heading->SetColorAndOpacity(FSlateColor(LootUITheme::Gold));
        PanelContents->AddChildToVerticalBox(Heading)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
        HUDText = HUD->WidgetTree->ConstructWidget<UTextBlock>();
        HUDText->SetMinDesiredWidth(240.f);
        HUDText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 20));
        HUDText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        HUDText->SetShadowColorAndOpacity(FLinearColor::Black);
        HUDText->SetShadowOffset(FVector2D(1.f, 1.f));
        PanelContents->AddChildToVerticalBox(HUDText);
        HUDValue = HUD->WidgetTree->ConstructWidget<UTextBlock>();
        HUDValue->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 24));
        HUDValue->SetColorAndOpacity(FSlateColor(LootUITheme::Success));
        PanelContents->AddChildToVerticalBox(HUDValue)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
        GainText = HUD->WidgetTree->ConstructWidget<UTextBlock>();
        GainText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
        GainText->SetColorAndOpacity(FSlateColor(LootUITheme::Success));
        GainText->SetText(FText::FromString(TEXT(" ")));
        PanelContents->AddChildToVerticalBox(GainText)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
        UTextBlock* InventoryHint = HUD->WidgetTree->ConstructWidget<UTextBlock>();
        InventoryHint->SetText(NSLOCTEXT("Loot", "ViewHaulHint", "[TAB]  View haul"));
        InventoryHint->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
        InventoryHint->SetColorAndOpacity(FSlateColor(LootUITheme::Muted));
        PanelContents->AddChildToVerticalBox(InventoryHint)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

        PromptPanel = HUD->WidgetTree->ConstructWidget<UBorder>();
        PromptPanel->SetBrush(CardBrush);
        PromptPanel->SetBrushColor(LootUITheme::Panel);
        PromptPanel->SetPadding(FMargin(14.f, 12.f));
        UHorizontalBox* PromptRow = HUD->WidgetTree->ConstructWidget<UHorizontalBox>();
        PromptPanel->SetContent(PromptRow);
        UBorder* KeyBadge = HUD->WidgetTree->ConstructWidget<UBorder>();
        KeyBadge->SetBrush(CardBrush);
        KeyBadge->SetBrushColor(LootUITheme::Gold);
        KeyBadge->SetPadding(FMargin(14.f, 7.f));
        UTextBlock* KeyText = HUD->WidgetTree->ConstructWidget<UTextBlock>();
        KeyText->SetText(FText::FromString(TEXT("F")));
        KeyText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 24));
        KeyText->SetColorAndOpacity(FSlateColor(LootUITheme::Panel));
        KeyBadge->SetContent(KeyText);
        PromptRow->AddChildToHorizontalBox(KeyBadge)->SetVerticalAlignment(VAlign_Center);
        UVerticalBox* PromptDetails = HUD->WidgetTree->ConstructWidget<UVerticalBox>();
        UHorizontalBoxSlot* DetailsSlot = PromptRow->AddChildToHorizontalBox(PromptDetails);
        DetailsSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
        DetailsSlot->SetVerticalAlignment(VAlign_Center);
        UTextBlock* ActionLabel = HUD->WidgetTree->ConstructWidget<UTextBlock>();
        ActionLabel->SetText(NSLOCTEXT("Loot", "StealLabel", "STEAL VALUABLE"));
        ActionLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
        ActionLabel->SetColorAndOpacity(FSlateColor(LootUITheme::Gold));
        PromptDetails->AddChildToVerticalBox(ActionLabel);
        PickupPrompt = HUD->WidgetTree->ConstructWidget<UTextBlock>();
        PickupPrompt->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        PickupPrompt->SetShadowColorAndOpacity(FLinearColor::Black);
        PickupPrompt->SetShadowOffset(FVector2D(2.f, 2.f));
        PickupPrompt->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 17));
        PickupPrompt->SetWrapTextAt(260.f);
        PromptDetails->AddChildToVerticalBox(PickupPrompt)->SetPadding(FMargin(0.f, 3.f, 0.f, 3.f));
        PickupValue = HUD->WidgetTree->ConstructWidget<UTextBlock>();
        PickupValue->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
        PickupValue->SetColorAndOpacity(FSlateColor(LootUITheme::Success));
        PromptDetails->AddChildToVerticalBox(PickupValue);
        PromptPanel->SetVisibility(ESlateVisibility::Collapsed);
        UCanvasPanelSlot* PromptSlot = Canvas->AddChildToCanvas(PromptPanel);
        PromptSlot->SetAlignment(FVector2D(0.5f, 1.f));
        PromptSlot->SetAutoSize(true);
        HUD->AddToPlayerScreen(5);
    }
    HUD->SetVisibility(ESlateVisibility::HitTestInvisible);
    HUDText->SetText(FText::Format(NSLOCTEXT("Loot", "BagCount", "Loot: {0} items"), FText::AsNumber(Inventory.Num())));
    HUDValue->SetText(FText::Format(NSLOCTEXT("Loot", "BagTotal", "Total: ${0}"), FText::AsNumber(GetTotalValue())));

    const double Now = GetWorld()->GetTimeSeconds();
    const int32 CurrentCount = Inventory.Num();
    const int64 CurrentTotal = GetTotalValue();
    if (CurrentCount > LastInventoryCount)
    {
        // Accumulate quick pickups, including multiple entries arriving in one network update.
        if (Now - FeedbackStartTime >= 1.8)
        {
            PendingGainCount = 0;
            PendingGainValue = 0;
        }
        PendingGainCount += CurrentCount - LastInventoryCount;
        PendingGainValue += CurrentTotal - LastTotalValue;
        FeedbackStartTime = Now;
        GainText->SetText(FText::Format(NSLOCTEXT("Loot", "PickupGain", "+{0} items   +${1}"),
            FText::AsNumber(PendingGainCount), FText::AsNumber(PendingGainValue)));
    }
    LastInventoryCount = CurrentCount;
    LastTotalValue = CurrentTotal;
    const float FeedbackAge = static_cast<float>(Now - FeedbackStartTime);
    const float Fade = 1.f - FMath::Clamp((FeedbackAge - 1.f) / 0.8f, 0.f, 1.f);
    const float Pulse = FeedbackAge < 1.8f
        ? Fade * (0.5f + 0.5f * FMath::Cos(FeedbackAge * 15.f)) : 0.f;
    const FLinearColor Green = LootUITheme::Success;
    HUDText->SetColorAndOpacity(FSlateColor(FMath::Lerp(FLinearColor::White, Green, Pulse)));
    HUDValue->SetColorAndOpacity(FSlateColor(FMath::Lerp(Green, FLinearColor::White, Pulse * 0.35f)));
    LootPanel->SetBrushColor(FMath::Lerp(LootUITheme::Panel,
        FLinearColor(0.025f, 0.24f, 0.1f, 0.96f), Pulse));
    LootPanel->SetRenderTransformPivot(FVector2D(1.f, 0.f));
    const float Pop = FeedbackAge < 0.3f ? FMath::Sin(FeedbackAge / 0.3f * PI) * 0.04f : 0.f;
    LootPanel->SetRenderScale(FVector2D(1.f + Pop));
    GainText->SetRenderOpacity(Fade);
    GainText->SetRenderTranslation(FVector2D(0.f, -8.f * (1.f - Fade)));

    PromptPanel->SetVisibility(ESlateVisibility::Collapsed);
    if (LastPromptPickup != FocusedPickup)
    {
        LastPromptPickup = FocusedPickup;
        PromptStartTime = Now;
    }
    if (ALootPickup* Pickup = FocusedPickup.Get())
    {
        // Project the top of the mesh into this player's UI coordinates.
        // This stays readable at any camera angle and respects viewport DPI.
        const FVector PromptLocation = Pickup->Mesh->Bounds.Origin
            + FVector(0.f, 0.f, Pickup->Mesh->Bounds.BoxExtent.Z + 20.f);
        FVector2D ScreenPosition;
        if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, PromptLocation, ScreenPosition, true))
        {
            PickupPrompt->SetText(Pickup->ItemName);
            PickupValue->SetText(FText::Format(NSLOCTEXT("Loot", "Currency", "${0}"), FText::AsNumber(FMath::Max(0, Pickup->Value))));
            CastChecked<UCanvasPanelSlot>(PromptPanel->Slot)->SetPosition(ScreenPosition);
            const float PromptProgress = FMath::Clamp(static_cast<float>(Now - PromptStartTime) / 0.12f, 0.f, 1.f);
            PromptPanel->SetRenderOpacity(PromptProgress);
            PromptPanel->SetRenderTranslation(FVector2D(0.f, (1.f - PromptProgress) * 6.f));
            PromptPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
}

void ULootCollectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CloseInventory();
    if (InventoryWidget) InventoryWidget->ReleasePreviews();
    if (OutlineMesh) OutlineMesh->DestroyComponent();
    if (HUD) HUD->RemoveFromParent();
    Super::EndPlay(EndPlayReason);
}
