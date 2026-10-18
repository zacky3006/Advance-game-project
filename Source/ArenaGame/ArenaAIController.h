#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Perception/AIPerceptionTypes.h"
#include "GenericTeamAgentInterface.h"
#include "ArenaAIController.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UAISenseConfig_Team;
class AActor;
class AArenaGamePlayerController;

USTRUCT()
struct FAIPlayerAlertState
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<AActor> PlayerActor = nullptr;

    UPROPERTY()
    float Progress = 0.0f;

    UPROPERTY()
    bool bIsInSight = false;

    bool bNearbyMovementSensed = false;
    double ChaseUntil = -1.0;
    bool bWasChasingWithoutSensing = false;

    UPROPERTY()
    bool bIsConfirmed = false;

    UPROPERTY()
    FVector LastKnownLocation = FVector::ZeroVector;
};

UCLASS()
class ARENAGAME_API AArenaAIController : public AAIController
{
    GENERATED_BODY()
    friend class UCoopPIETestLibrary;

public:
    AArenaAIController();

    virtual void OnPossess(APawn* InPawn) override;
    virtual FGenericTeamId GetGenericTeamId() const override;
    virtual void Tick(float DeltaTime) override;

    // Lab 11: Team Sense
    void ReportTrouble(AActor* Enemy);

    // Alert system
    UFUNCTION(BlueprintCallable, Category = "AI Alert")
    float GetAlertProgress() const;

protected:
    virtual void BeginPlay() override;

    // Behavior Tree
    UPROPERTY(EditAnywhere, Category = "AI")
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset = nullptr;

    // AI Perception callback
    UFUNCTION()
    void OnTargetSensed(AActor* Actor, FAIStimulus Stimulus);

    // Perception configs
    UPROPERTY(VisibleAnywhere, Category = "AI")
    TObjectPtr<UAISenseConfig_Sight> SightConfig = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "AI")
    TObjectPtr<UAISenseConfig_Hearing> HearingConfig = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "AI")
    TObjectPtr<UAISenseConfig_Damage> DamageConfig = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "AI")
    TObjectPtr<UAISenseConfig_Team> TeamConfig = nullptr;

    // Blackboard keys
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    FName TargetActorKey = TEXT("TargetActor");

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    FName DamageLocationKey = TEXT("LastDamageLocation");

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    FName InvestigateLocationKey = TEXT("InvestigateLocation");

    // Alert settings
    UPROPERTY(EditDefaultsOnly, Category = "AI Alert",
        meta = (ClampMin = "0.1"))
    float AlertFillDuration = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category = "AI Alert",
        meta = (ClampMin = "0.1"))
    float AlertDecayDuration = 2.0f;

    // Unreal units are centimeters. Zero disables instant close-range confirmation.
    UPROPERTY(EditDefaultsOnly, Category = "AI Alert", meta = (ClampMin = "0.0", Units = "cm"))
    float InstantDetectionDistance = 400.0f;

    // Moving sprinters are recognized farther away; sight is still required.
    UPROPERTY(EditDefaultsOnly, Category = "AI Alert", meta = (ClampMin = "0.0", Units = "cm"))
    float InstantSprintDetectionDistance = 700.0f;

    UPROPERTY(EditDefaultsOnly, Category = "AI Alert", meta = (ClampMin = "0.0", Units = "cm"))
    float NearbyMovementDetectionDistance = 200.0f;

    UPROPERTY(EditDefaultsOnly, Category = "AI Alert", meta = (ClampMin = "0.0", Units = "s"))
    float LostTargetChaseDuration = 3.0f;

    // Time spent searching after a confirmed player is lost. New sounds refresh the search.
    UPROPERTY(EditDefaultsOnly, Category = "AI Alert", meta = (ClampMin = "0.1"))
    float AlertSearchDuration = 6.0f;

    // Alert state tracked separately for each player
    UPROPERTY()
    TArray<FAIPlayerAlertState> PlayerAlertStates;

    // Blackboard alert keys
    UPROPERTY(EditDefaultsOnly, Category = "AI Alert")
    FName AlertConfirmedKey = TEXT("AlertConfirmed");

    UPROPERTY(EditDefaultsOnly, Category = "AI Alert")
    FName AlertTargetBlackboardKey = TEXT("TargetActor");

private:
    void UpdateAlert(float DeltaTime);
    void InvestigateAt(const FVector& Location);
    double InvestigationExpiresAt = -1.0;
    bool bInvestigationFocusActive = false;

    FAIPlayerAlertState* FindPlayerAlertState(AActor* PlayerActor);

    FAIPlayerAlertState& FindOrAddPlayerAlertState(
        AActor* PlayerActor);

    void UpdateAlertTarget(AActor* NewTarget);

    void ClearAlertTarget();

    void NotifyPlayerAlert(
        AActor* PlayerActor,
        float AlertPercent);

    void NotifyPlayerAlertHidden(AActor* PlayerActor);
};
