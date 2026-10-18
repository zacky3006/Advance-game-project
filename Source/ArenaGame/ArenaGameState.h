#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LootCollectorComponent.h"
#include "ArenaGameState.generated.h"


UCLASS()
class ARENAGAME_API AArenaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
    AArenaGameState();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Team")
    TArray<FLootEntry> TeamInventory;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Team")
    TArray<TObjectPtr<APlayerState>> EscapedPlayers;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Team")
    int32 TeamSize = 1;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Team")
    int32 ConnectedPlayers = 0;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Team")
    bool bTeamComplete = false;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Team")
    int32 ResultsReady = 0;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Team")
    double TeamRunSeconds = 0.0;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Team")
    bool bTeamFailed = false;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Team")
    FText TeamFailureReason;

    UFUNCTION(BlueprintPure, Category="Team")
    int64 GetTeamTotalValue() const;
    void AddTeamLoot(const FLootEntry& Entry);
    void ExtractPlayer(ULootCollectorComponent* Collector, const FString& Movie);
    void PlayerDied(APlayerController* Controller);
    void LockRoster();
    void CheckTeamResolved();
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Team")
    TArray<TObjectPtr<APlayerState>> DeadPlayers;
    void ConfirmResults(APlayerController* Controller);
    void PlayerDisconnected(APlayerController* Controller);
    void AbortTeam(const FText& Reason);
private:
    void RefreshTeamSize();
    UPROPERTY() TArray<TObjectPtr<APlayerState>> ExtractionRoster;
    UPROPERTY() TArray<TObjectPtr<APlayerState>> ReadyPlayers;
    FString TeamEndingMovie;
    bool bReturningToMenu = false;
public:

	// ==========================================
	// Round State
	// ==========================================

	UPROPERTY(
		ReplicatedUsing = OnRep_RoundOver,
		BlueprintReadOnly
	)
	bool bRoundOver = false;

	UPROPERTY(
		Replicated,
		BlueprintReadOnly
	)
	int32 WinnerId = -1;


	// ==========================================
	// Replication
	// ==========================================

	UFUNCTION()
	void OnRep_RoundOver();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;
};
