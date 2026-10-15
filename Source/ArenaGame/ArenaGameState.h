#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ArenaGameState.generated.h"


UCLASS()
class ARENAGAME_API AArenaGameState : public AGameStateBase
{
	GENERATED_BODY()

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