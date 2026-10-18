// Copyright Epic Games, Inc. All Rights Reserved.

#include "ArenaGamePlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "ArenaGame.h"
#include "Widgets/Input/SVirtualJoystick.h"

#include "PlayerAlertHUD.h"
#include "GameFramework/Actor.h"
#include "ArenaGameState.h"
#include "EscapeSequenceWidget.h"
#include "TeamStatusWidget.h"
#include "HealthComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Sound/AmbientSound.h"
#include "Components/AudioComponent.h"
#include "MissionFailedWidget.h"

namespace
{
    void FadeOutRoundMusic(UWorld* World)
    {
        for (TActorIterator<AAmbientSound> It(World); It; ++It)
        {
            if (!It->ActorHasTag(TEXT("OfficeBGM"))) continue;
            UAudioComponent* Audio = It->GetAudioComponent();
            if (!Audio || Audio->ComponentHasTag(TEXT("RoundMusicStopped"))) continue;
            Audio->ComponentTags.AddUnique(TEXT("RoundMusicStopped"));
            Audio->FadeOut(1.f, 0.f);
        }
    }
}

// ============================================================
// LAB 11 STEP 1
// Player Controller Team ID
// ============================================================

FGenericTeamId AArenaGamePlayerController::GetGenericTeamId() const
{
    return FGenericTeamId(0);
}

// ============================================================
// Begin Play
// ============================================================

void AArenaGamePlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (IsLocalController())
    {
        bShowMouseCursor = false;
        ResetIgnoreMoveInput();
        ResetIgnoreLookInput();
        SetInputMode(FInputModeGameOnly());
        TeamHUD = CreateWidget<UTeamStatusWidget>(this, UTeamStatusWidget::StaticClass());
        if (TeamHUD) TeamHUD->AddToPlayerScreen(15);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("PLAYER TEAM: %d"),
        static_cast<int32>(GetGenericTeamId().GetId())
    );

    // Create the Alert HUD only for this local player.
    if (IsLocalPlayerController() && PlayerAlertHUDClass)
    {
        PlayerAlertHUDInstance = CreateWidget<UPlayerAlertHUD>(
            this,
            PlayerAlertHUDClass
        );

        if (PlayerAlertHUDInstance)
        {
            PlayerAlertHUDInstance->AddToPlayerScreen(10);
        }
        else
        {
            UE_LOG(
                LogArenaGame,
                Error,
                TEXT("Could not create Player Alert HUD.")
            );
        }
    }

    // Mobile Touch Controls
    if (ShouldUseTouchControls() && IsLocalPlayerController())
    {
        if (MobileControlsWidgetClass)
        {
            MobileControlsWidget = CreateWidget<UUserWidget>(
                this,
                MobileControlsWidgetClass
            );

            if (MobileControlsWidget)
            {
                MobileControlsWidget->AddToPlayerScreen(0);
            }
            else
            {
                UE_LOG(
                    LogArenaGame,
                    Error,
                    TEXT("Could not spawn mobile controls widget.")
                );
            }
        }
    }
}

// ============================================================
// AI ALERT HUD
// Local HUD update functions
// ============================================================

void AArenaGamePlayerController::ShowAIAlert(
    AActor* AIActor,
    float AlertPercent)
{
    if (bTeamSpectating || bTeamEndingStarted) return;
    if (!IsLocalPlayerController() ||
        !IsValid(AIActor) ||
        !PlayerAlertHUDInstance)
    {
        return;
    }

    PlayerAlertHUDInstance->AddOrUpdateIndicator(
        AIActor,
        AlertPercent
    );
}

void AArenaGamePlayerController::HideAIAlert(AActor* AIActor)
{
    if (!IsLocalPlayerController() ||
        !IsValid(AIActor) ||
        !PlayerAlertHUDInstance)
    {
        return;
    }

    PlayerAlertHUDInstance->RemoveIndicator(AIActor);
}

// ============================================================
// AI ALERT HUD
// Client RPC Implementations
// ============================================================

void AArenaGamePlayerController::ClientShowAIAlert_Implementation(
    AActor* AIActor,
    float AlertPercent)
{
    ShowAIAlert(AIActor, AlertPercent);
}

void AArenaGamePlayerController::ClientHideAIAlert_Implementation(
    AActor* AIActor)
{
    HideAIAlert(AIActor);
}

// ============================================================
// Setup Input Component
// ============================================================

void AArenaGamePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (IsLocalPlayerController())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
                GetLocalPlayer()
            ))
        {
            for (UInputMappingContext* CurrentContext :
                 DefaultMappingContexts)
            {
                if (CurrentContext)
                {
                    Subsystem->AddMappingContext(CurrentContext, 0);
                }
            }

            if (!ShouldUseTouchControls())
            {
                for (UInputMappingContext* CurrentContext :
                     MobileExcludedMappingContexts)
                {
                    if (CurrentContext)
                    {
                        Subsystem->AddMappingContext(CurrentContext, 0);
                    }
                }
            }
        }
    }
}

// ============================================================
// Should Use Touch Controls
// ============================================================

bool AArenaGamePlayerController::ShouldUseTouchControls() const
{
    return SVirtualJoystick::ShouldDisplayTouchInterface()
        || bForceTouchControls;
}

void AArenaGamePlayerController::ClientWatchTeammate_Implementation()
{
    if (bTeamSpectating || bTeamEndingStarted) return;
    bTeamSpectating = true;
    bAutoManageActiveCameraTarget = false;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor = false;
    if (PlayerAlertHUDInstance) PlayerAlertHUDInstance->ClearIndicators();
}

void AArenaGamePlayerController::ClientMissionFailed_Implementation()
{
    if (bTeamEndingStarted) return;
    bTeamEndingStarted = true;
    bTeamSpectating = false;
    FadeOutRoundMusic(GetWorld());
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FlushPressedKeys();
    bShowMouseCursor = false;
    if (PlayerAlertHUDInstance) PlayerAlertHUDInstance->ClearIndicators();
    if (UMissionFailedWidget* Failed = CreateWidget<UMissionFailedWidget>(this, UMissionFailedWidget::StaticClass()))
    {
        TeamEnding = Failed;
        Failed->AddToPlayerScreen(10000);
        SetInputMode(FInputModeUIOnly());
    }
}

void AArenaGamePlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (IsLocalController())
        if (const AArenaGameState* Team = GetWorld()->GetGameState<AArenaGameState>(); Team && (Team->bTeamComplete || Team->bTeamFailed))
            FadeOutRoundMusic(GetWorld());
    if (!IsLocalController() || !bTeamSpectating || bTeamEndingStarted) return;
    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        APawn* Target = *It;
        const ULootCollectorComponent* Loot = Target->FindComponentByClass<ULootCollectorComponent>();
        const UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
        if (Target == GetPawn() || !Target->GetPlayerState() || !Loot || Loot->HasEscaped() || (Health && Health->IsDead())) continue;
        if (GetViewTarget() != Target) SetViewTargetWithBlend(Target, 0.25f);
        break;
    }
}

void AArenaGamePlayerController::ClientTeamEnding_Implementation(const FString& Movie, const TArray<FLootEntry>& Entries, double Seconds)
{
    if (bTeamEndingStarted) return;
    bTeamEndingStarted = true;
    FadeOutRoundMusic(GetWorld());
    if (bTeamSpectating)
    {
        SetIgnoreMoveInput(false);
        SetIgnoreLookInput(false);
        bTeamSpectating = false;
    }
    if (GetPawn())
        if (ULootCollectorComponent* Loot = GetPawn()->FindComponentByClass<ULootCollectorComponent>()) Loot->PrepareTeamEnding();
    if (UEscapeSequenceWidget* Ending = CreateWidget<UEscapeSequenceWidget>(this, UEscapeSequenceWidget::StaticClass()))
    {
        TeamEnding = Ending;
        Ending->SetRunResult(Entries, Seconds);
        Ending->AddToPlayerScreen(500);
        Ending->BeginEnding(Movie);
    }
}

void AArenaGamePlayerController::ServerConfirmResults_Implementation()
{
    if (AArenaGameState* Team = GetWorld()->GetGameState<AArenaGameState>()) Team->ConfirmResults(this);
}
