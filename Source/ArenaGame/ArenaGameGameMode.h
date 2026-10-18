// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "ArenaGameGameMode.generated.h"


class AActor;
class APawn;
class AController;
class AArenaPlayerState;


UCLASS(abstract)
class ARENAGAME_API AArenaGameGameMode
    : public AGameModeBase
{
    GENERATED_BODY()


public:

    // =====================================================
    // Constructor
    // =====================================================

    AArenaGameGameMode();
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void Logout(AController* Exiting) override;


    // =====================================================
    // Game Rules
    // =====================================================

    UPROPERTY(
        EditDefaultsOnly,
        Category = "Rules"
    )
    int32 WinningScore = 3;


    // =====================================================
    // Character Death / Score
    // =====================================================

    void OnCharacterKilled(
        AController* Killer,
        AController* Victim
    );


    void CheckWinner(
        AArenaPlayerState* Scorer
    );


    // =====================================================
    // Travel
    // =====================================================

    UFUNCTION(
        BlueprintCallable,
        Category = "Rules"
    )
    void TravelToArena();


    // =====================================================
    // AI
    // =====================================================

    UPROPERTY(
        EditDefaultsOnly,
        Category = "AI"
    )
    TSubclassOf<APawn> AICharacterClass;


    UFUNCTION(
        BlueprintCallable,
        Category = "AI"
    )
    void SpawnAICharacter(
        const FTransform& SpawnXform
    );


    // =====================================================
    // LAB 12 - WAVE
    // =====================================================

    UFUNCTION(
        BlueprintCallable,
        Category = "Wave"
    )
    void StartWave(
        const TArray<AActor*>& Points
    );


    /**
     * Number of enemies in each wave.
     *
     * Example:
     * Wave 1 = 2 enemies
     * Wave 2 = 4 enemies
     * Wave 3 = 6 enemies
     */
    UPROPERTY(
        EditDefaultsOnly,
        Category = "Wave"
    )
    TArray<int32> WaveSizes = { 2, 4, 6 };


    /**
     * Time between each enemy spawn.
     */
    UPROPERTY(
        EditDefaultsOnly,
        Category = "Wave"
    )
    float SpawnInterval = 3.0f;


protected:

    // =====================================================
    // LAB 12 - STEP 1
    // Spawn Points
    // =====================================================

    /**
     * Spawn locations used by the current wave.
     */
    TArray<AActor*> SpawnPoints;


    /**
     * Select a random spawn transform.
     */
    FTransform PickSpawnTransform() const;


    // =====================================================
    // LAB 12 - STEP 2
    // Wave Spawning
    // =====================================================

    /**
     * Called repeatedly by WaveTimerHandle.
     *
     * Spawns one enemy at a time.
     */
    void SpawnWave();


    /**
     * Returns true if there is at least
     * one AI enemy currently alive.
     */
    bool AnyEnemiesLeft() const;


    /**
     * Timer used to spawn enemies.
     */
    FTimerHandle WaveTimerHandle;


    /**
     * Current wave index.
     *
     * 0 = Wave 1
     * 1 = Wave 2
     * 2 = Wave 3
     */
    int32 CurrentWave = 0;


    /**
     * Number of enemies already spawned
     * in the current wave.
     */
    int32 SpawnedSoFar = 0;


    // =====================================================
    // LAB 12 - STEP 3
    // Stop AI
    // =====================================================

    /**
     * Stop all AI Behavior Trees when the round ends.
     */
    void StopAllAI();
};
