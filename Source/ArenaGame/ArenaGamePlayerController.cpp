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