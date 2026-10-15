#include "ArenaAIController.h"

#include "ArenaGamePlayerController.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"

#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Team.h"

#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Team.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "GenericTeamAgentInterface.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "Engine/World.h"


// ============================================================
// Constructor
// ============================================================

AArenaAIController::AArenaAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    SetReplicates(true);

    UAIPerceptionComponent* Perception =
        CreateDefaultSubobject<UAIPerceptionComponent>(
            TEXT("AI Perception")
        );

    SetPerceptionComponent(*Perception);

    // Sight
    SightConfig =
        CreateDefaultSubobject<UAISenseConfig_Sight>(
            TEXT("Sight")
        );

    SightConfig->SightRadius = 1200.f;
    SightConfig->LoseSightRadius = 1500.f;
    SightConfig->PeripheralVisionAngleDegrees = 30.f;

    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

    Perception->ConfigureSense(*SightConfig);

    // Hearing
    HearingConfig =
        CreateDefaultSubobject<UAISenseConfig_Hearing>(
            TEXT("Hearing")
        );

    HearingConfig->HearingRange = 3000.f;
    HearingConfig->SetMaxAge(8.f);

    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = false;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;

    Perception->ConfigureSense(*HearingConfig);

    // Damage
    DamageConfig =
        CreateDefaultSubobject<UAISenseConfig_Damage>(
            TEXT("Damage")
        );

    DamageConfig->SetMaxAge(8.f);
    Perception->ConfigureSense(*DamageConfig);

    // Team Sense - Lab 11 Step 7
    TeamConfig =
        CreateDefaultSubobject<UAISenseConfig_Team>(
            TEXT("Team")
        );

    TeamConfig->SetMaxAge(8.f);
    Perception->ConfigureSense(*TeamConfig);

    Perception->SetDominantSense(
        SightConfig->GetSenseImplementation()
    );
}


// ============================================================
// Team ID
// ============================================================

FGenericTeamId AArenaAIController::GetGenericTeamId() const
{
    return FGenericTeamId(1);
}


// ============================================================
// Begin Play
// ============================================================

void AArenaAIController::BeginPlay()
{
    Super::BeginPlay();

    if (UAIPerceptionComponent* Perception =
        GetPerceptionComponent())
    {
        Perception->OnTargetPerceptionUpdated.AddDynamic(
            this,
            &AArenaAIController::OnTargetSensed
        );
    }

    // Lab 11 Checkpoint A
    const APawn* PlayerPawn =
        UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    if (PlayerPawn)
    {
        const ETeamAttitude::Type Attitude =
            GetTeamAttitudeTowards(*PlayerPawn);

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("AI TEAM: %d | ATTITUDE TO PLAYER: %s"),
            static_cast<int32>(GetGenericTeamId().GetId()),
            Attitude == ETeamAttitude::Hostile
                ? TEXT("Hostile")
                : Attitude == ETeamAttitude::Friendly
                    ? TEXT("Friendly")
                    : TEXT("Neutral")
        );
    }
}


// ============================================================
// Tick
// ============================================================

void AArenaAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Alert is calculated on the server only.
    if (HasAuthority())
    {
        UpdateAlert(DeltaTime);
    }
}


// ============================================================
// On Possess
// ============================================================

void AArenaAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (ACharacter* ControlledCharacter =
        Cast<ACharacter>(InPawn))
    {
        if (UCharacterMovementComponent* Movement =
            ControlledCharacter->GetCharacterMovement())
        {
            Movement->MaxWalkSpeed = 250.f;
        }
    }

    if (BehaviorTreeAsset)
    {
        RunBehaviorTree(BehaviorTreeAsset);
    }
}


// ============================================================
// Find Player Alert State
// ============================================================

FAIPlayerAlertState* AArenaAIController::FindPlayerAlertState(
    AActor* PlayerActor)
{
    if (!IsValid(PlayerActor))
    {
        return nullptr;
    }

    for (FAIPlayerAlertState& State : PlayerAlertStates)
    {
        if (State.PlayerActor == PlayerActor)
        {
            return &State;
        }
    }

    return nullptr;
}


// ============================================================
// Find Or Add Player Alert State
// ============================================================

FAIPlayerAlertState&
AArenaAIController::FindOrAddPlayerAlertState(
    AActor* PlayerActor)
{
    if (FAIPlayerAlertState* ExistingState =
        FindPlayerAlertState(PlayerActor))
    {
        return *ExistingState;
    }

    FAIPlayerAlertState NewState;
    NewState.PlayerActor = PlayerActor;

    return PlayerAlertStates.Add_GetRef(NewState);
}


// ============================================================
// Get Alert Progress
// Returns the highest progress among tracked players.
// ============================================================

float AArenaAIController::GetAlertProgress() const
{
    float HighestProgress = 0.f;

    for (const FAIPlayerAlertState& State : PlayerAlertStates)
    {
        HighestProgress = FMath::Max(
            HighestProgress,
            State.Progress
        );
    }

    return HighestProgress;
}


// ============================================================
// Notify Player: Show / Update Alert
// ============================================================

void AArenaAIController::NotifyPlayerAlert(
    AActor* PlayerActor,
    float AlertPercent)
{
    if (!HasAuthority() || !IsValid(PlayerActor))
    {
        return;
    }

    for (FConstPlayerControllerIterator It =
        GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AArenaGamePlayerController* PC =
            Cast<AArenaGamePlayerController>(It->Get());

        if (PC && PC->GetPawn() == PlayerActor)
        {
            PC->ClientShowAIAlert(
                GetPawn(),
                FMath::Clamp(AlertPercent, 0.f, 1.f)
            );

            return;
        }
    }
}


// ============================================================
// Notify Player: Hide Alert
// ============================================================

void AArenaAIController::NotifyPlayerAlertHidden(
    AActor* PlayerActor)
{
    if (!HasAuthority() || !IsValid(PlayerActor))
    {
        return;
    }

    for (FConstPlayerControllerIterator It =
        GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AArenaGamePlayerController* PC =
            Cast<AArenaGamePlayerController>(It->Get());

        if (PC && PC->GetPawn() == PlayerActor)
        {
            PC->ClientHideAIAlert(GetPawn());
            return;
        }
    }
}


// ============================================================
// Update Alert
// ============================================================

void AArenaAIController::UpdateAlert(float DeltaTime)
{
    if (!HasAuthority())
    {
        return;
    }

    UBlackboardComponent* BB = GetBlackboardComponent();

    // Update alert separately for every player.
    for (int32 Index = PlayerAlertStates.Num() - 1;
         Index >= 0;
         --Index)
    {
        FAIPlayerAlertState& State = PlayerAlertStates[Index];

        if (!IsValid(State.PlayerActor))
        {
            PlayerAlertStates.RemoveAt(Index);
            continue;
        }

        // Confirmed alerts remain confirmed.
        if (State.bIsConfirmed)
        {
            State.Progress = 1.f;
            continue;
        }

        if (State.bIsInSight)
        {
            State.Progress = FMath::Min(
                1.f,
                State.Progress +
                    DeltaTime /
                    FMath::Max(AlertFillDuration, 0.1f)
            );
        }
        else
        {
            State.Progress = FMath::Max(
                0.f,
                State.Progress -
                    DeltaTime /
                    FMath::Max(AlertDecayDuration, 0.1f)
            );
        }

        // Update the affected player's own HUD.
        if (State.Progress > 0.f)
        {
            NotifyPlayerAlert(
                State.PlayerActor,
                State.Progress
            );
        }
        else
        {
            NotifyPlayerAlertHidden(State.PlayerActor);
        }

        // Confirm alert after reaching 100%.
        if (State.Progress >= 1.f && State.bIsInSight)
        {
            State.bIsConfirmed = true;

            UE_LOG(
                LogTemp,
                Log,
                TEXT("AI Alert confirmed: AI=%s, Player=%s"),
                *GetName(),
                *GetNameSafe(State.PlayerActor)
            );
        }
    }

    // Select a confirmed target for the existing Behavior Tree.
    // Blackboard has one TargetActor key, so only one target can
    // occupy this key at a time.
    if (BB)
    {
        AActor* ConfirmedTarget = nullptr;

        for (const FAIPlayerAlertState& State : PlayerAlertStates)
        {
            if (State.bIsConfirmed &&
                IsValid(State.PlayerActor))
            {
                ConfirmedTarget = State.PlayerActor;
                break;
            }
        }

        if (ConfirmedTarget)
        {
            BB->SetValueAsBool(AlertConfirmedKey, true);
            BB->SetValueAsObject(
                AlertTargetBlackboardKey,
                ConfirmedTarget
            );
        }
        else
        {
            BB->SetValueAsBool(AlertConfirmedKey, false);
            BB->ClearValue(AlertTargetBlackboardKey);
        }
    }
}


// ============================================================
// Update Alert Target
// ============================================================

void AArenaAIController::UpdateAlertTarget(AActor* NewTarget)
{
    if (!HasAuthority() || !IsValid(NewTarget))
    {
        return;
    }

    FAIPlayerAlertState& State =
        FindOrAddPlayerAlertState(NewTarget);

    State.bIsInSight = true;

    // Update immediately so the player sees the indicator.
    NotifyPlayerAlert(NewTarget, State.Progress);
}


// ============================================================
// Clear Alert Target
// ============================================================

void AArenaAIController::ClearAlertTarget()
{
    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->ClearValue(AlertTargetBlackboardKey);
        BB->SetValueAsBool(AlertConfirmedKey, false);
    }
}


// ============================================================
// Report Trouble - Lab 11 Step 7
// ============================================================

void AArenaAIController::ReportTrouble(AActor* Enemy)
{
    if (!GetPawn() || !HasAuthority())
    {
        return;
    }

    FAITeamStimulusEvent TeamEvent(
        this,
        Enemy,
        GetPawn()->GetActorLocation(),
        3000.f
    );

    UAIPerceptionSystem::OnEvent(
        GetWorld(),
        TeamEvent
    );
}


// ============================================================
// AI Perception Callback
// ============================================================

void AArenaAIController::OnTargetSensed(
    AActor* Actor,
    FAIStimulus Stimulus)
{
    if (!Actor)
    {
        return;
    }

    const TSubclassOf<UAISense> Sense =
        UAIPerceptionSystem::GetSenseClassForStimulus(
            this,
            Stimulus
        );

    UBlackboardComponent* BB = GetBlackboardComponent();

    // Sight
    if (Sense == UAISense_Sight::StaticClass())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            // Only track player-controlled pawns for Alert.
            APlayerController* PlayerPC =
                UGameplayStatics::GetPlayerControllerFromID(
                    this,
                    0
                );

            bool bIsPlayerPawn = false;

            for (FConstPlayerControllerIterator It =
                GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                APlayerController* PC = It->Get();

                if (PC && PC->GetPawn() == Actor)
                {
                    bIsPlayerPawn = true;
                    break;
                }
            }

            if (HasAuthority() && bIsPlayerPawn)
            {
                UpdateAlertTarget(Actor);
            }

            // Keep existing investigation behavior for sight.
        }
        else
        {
            if (FAIPlayerAlertState* State =
                FindPlayerAlertState(Actor))
            {
                State->bIsInSight = false;

                // An alert that is not yet confirmed will decay.
                // A confirmed target remains confirmed.
                if (BB)
                {
                    BB->SetValueAsVector(
                        InvestigateLocationKey,
                        Stimulus.StimulusLocation
                    );
                }
            }
        }

        return;
    }

    // Damage
    if (Sense == UAISense_Damage::StaticClass())
    {
        if (BB)
        {
            BB->SetValueAsVector(
                DamageLocationKey,
                Stimulus.StimulusLocation
            );
        }

        return;
    }

    // Hearing
    if (Sense == UAISense_Hearing::StaticClass())
    {
        if (BB)
        {
            BB->SetValueAsVector(
                InvestigateLocationKey,
                Stimulus.StimulusLocation
            );
        }

        return;
    }

    // Team Sense - Lab 11 Step 7
    if (Sense == UAISense_Team::StaticClass())
    {
        if (BB)
        {
            BB->SetValueAsVector(
                InvestigateLocationKey,
                Stimulus.StimulusLocation
            );
        }

        return;
    }
}