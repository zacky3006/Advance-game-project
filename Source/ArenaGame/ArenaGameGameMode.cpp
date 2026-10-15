#include "ArenaGameGameMode.h"

#include "ArenaPlayerState.h"
#include "ArenaGameState.h"
#include "ArenaAIController.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "BrainComponent.h"
#include "TimerManager.h"


AArenaGameGameMode::AArenaGameGameMode()
{
}


void AArenaGameGameMode::OnCharacterKilled(
    AController* Killer,
    AController* Victim)
{
    if (!HasAuthority())
    {
        return;
    }

    // ==========================================
    // Add kill to the player who made the kill
    // ==========================================
    if (Killer)
    {
        if (AArenaPlayerState* KillerPlayerState =
            Killer->GetPlayerState<AArenaPlayerState>())
        {
            KillerPlayerState->AddKill();

            CheckWinner(KillerPlayerState);
        }
    }

    // ==========================================
    // Destroy the dead character
    // ==========================================
    if (Victim)
    {
        if (APawn* VictimPawn = Victim->GetPawn())
        {
            VictimPawn->Destroy();
        }
    }

    // ==========================================
    // LAB 12 - Player death ends the round
    // ==========================================
    AArenaGameState* GS = GetGameState<AArenaGameState>();

    const bool bRoundStillRunning =
        (GS == nullptr || !GS->bRoundOver);

    if (bRoundStillRunning && Victim)
    {
        // If the victim is a player, the AI wins.
        if (Victim->IsPlayerController())
        {
            if (GS)
            {
                GS->WinnerId = -1;
                GS->bRoundOver = true;
                GS->OnRep_RoundOver();
            }

            // Stop all AI immediately.
            StopAllAI();

            // Stop the wave timer.
            GetWorldTimerManager().ClearTimer(WaveTimerHandle);
        }
    }
}


void AArenaGameGameMode::CheckWinner(AArenaPlayerState* Scorer)
{
    if (!HasAuthority() || !Scorer)
    {
        return;
    }

    if (Scorer->Kills < WinningScore)
    {
        return;
    }

    AArenaGameState* GS = GetGameState<AArenaGameState>();

    if (!GS || GS->bRoundOver)
    {
        return;
    }

    GS->WinnerId = Scorer->GetPlayerId();
    GS->bRoundOver = true;
    GS->OnRep_RoundOver();

    // Stop all AI when the player wins.
    StopAllAI();

    // Stop spawning more AI.
    GetWorldTimerManager().ClearTimer(WaveTimerHandle);
}


void AArenaGameGameMode::TravelToArena()
{
    if (!HasAuthority())
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    World->ServerTravel(TEXT("/Game/Maps/Arena?listen"));
}


void AArenaGameGameMode::SpawnAICharacter(
    const FTransform& SpawnXform)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!AICharacterClass)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("SpawnAICharacter: AICharacterClass is not assigned.")
        );

        return;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    SpawnParams.ObjectFlags |= RF_Transient;

    APawn* SpawnedPawn = World->SpawnActor<APawn>(
        AICharacterClass,
        SpawnXform,
        SpawnParams
    );

    if (!SpawnedPawn)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("SpawnAICharacter: Failed to spawn AI pawn.")
        );

        return;
    }

    // Make sure the spawned AI has a controller.
    if (!SpawnedPawn->GetController())
    {
        SpawnedPawn->SpawnDefaultController();
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("SpawnAICharacter: AI spawned successfully at %s"),
        *SpawnXform.GetLocation().ToString()
    );
}


void AArenaGameGameMode::StartWave(
    const TArray<AActor*>& Points)
{
    if (!HasAuthority())
    {
        return;
    }

    if (Points.Num() == 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("StartWave: No spawn points were provided.")
        );

        return;
    }

    if (WaveSizes.Num() == 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("StartWave: WaveSizes is empty.")
        );

        return;
    }

    // Save spawn points.
    SpawnPoints = Points;

    // Reset wave state.
    CurrentWave = 0;
    SpawnedSoFar = 0;

    // Stop an existing wave timer before starting a new one.
    GetWorldTimerManager().ClearTimer(WaveTimerHandle);

    // Start spawning.
    GetWorldTimerManager().SetTimer(
        WaveTimerHandle,
        this,
        &AArenaGameGameMode::SpawnWave,
        SpawnInterval,
        true
    );

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Wave system started. First wave size: %d"),
        WaveSizes[CurrentWave]
    );
}


FTransform AArenaGameGameMode::PickSpawnTransform() const
{
    if (SpawnPoints.Num() == 0)
    {
        return FTransform::Identity;
    }

    const int32 RandomIndex =
        FMath::RandRange(0, SpawnPoints.Num() - 1);

    AActor* SpawnPoint = SpawnPoints[RandomIndex];

    if (!SpawnPoint)
    {
        return FTransform::Identity;
    }

    return SpawnPoint->GetActorTransform();
}


void AArenaGameGameMode::SpawnWave()
{
    if (!HasAuthority())
    {
        return;
    }

    AArenaGameState* GS = GetGameState<AArenaGameState>();

    // Stop spawning if the round is already over.
    if (GS && GS->bRoundOver)
    {
        GetWorldTimerManager().ClearTimer(WaveTimerHandle);
        StopAllAI();
        return;
    }

    // Safety check.
    if (WaveSizes.Num() == 0)
    {
        GetWorldTimerManager().ClearTimer(WaveTimerHandle);
        return;
    }

    if (CurrentWave >= WaveSizes.Num())
    {
        GetWorldTimerManager().ClearTimer(WaveTimerHandle);
        return;
    }

    const int32 ThisWaveSize =
        WaveSizes[CurrentWave];

    // ==========================================
    // Current wave has finished spawning
    // ==========================================
    if (SpawnedSoFar >= ThisWaveSize)
    {
        // Wait until every AI from this wave is dead.
        if (AnyEnemiesLeft())
        {
            return;
        }

        // Move to next wave.
        CurrentWave++;
        SpawnedSoFar = 0;

        // ==========================================
        // All waves completed
        // ==========================================
        if (CurrentWave >= WaveSizes.Num())
        {
            GetWorldTimerManager().ClearTimer(WaveTimerHandle);

            UE_LOG(
                LogTemp,
                Log,
                TEXT("All waves completed. Player wins.")
            );

            if (GS && !GS->bRoundOver)
            {
                GS->WinnerId = 0;
                GS->bRoundOver = true;
                GS->OnRep_RoundOver();
            }

            StopAllAI();

            return;
        }

        UE_LOG(
            LogTemp,
            Log,
            TEXT("Starting Wave %d. Size: %d"),
            CurrentWave + 1,
            WaveSizes[CurrentWave]
        );

        return;
    }

    // ==========================================
    // Spawn one AI
    // ==========================================
    const FTransform SpawnTransform =
        PickSpawnTransform();

    SpawnAICharacter(SpawnTransform);

    SpawnedSoFar++;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Wave %d: Spawned AI %d / %d"),
        CurrentWave + 1,
        SpawnedSoFar,
        ThisWaveSize
    );
}


bool AArenaGameGameMode::AnyEnemiesLeft() const
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return false;
    }

    for (TActorIterator<AArenaAIController> It(World);
         It;
         ++It)
    {
        AArenaAIController* AIController = *It;

        if (!AIController)
        {
            continue;
        }

        if (AIController->GetPawn())
        {
            return true;
        }
    }

    return false;
}


void AArenaGameGameMode::StopAllAI()
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    for (TActorIterator<AArenaAIController> It(World);
         It;
         ++It)
    {
        AArenaAIController* AIController = *It;

        if (!AIController)
        {
            continue;
        }

        if (UBrainComponent* Brain =
            AIController->GetBrainComponent())
        {
            Brain->StopLogic(TEXT("Round over"));
        }
    }
}