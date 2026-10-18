// Fill out your copyright notice in the Description page of Project Settings.

#include "ArenaGameState.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ArenaGamePlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"

AArenaGameState::AArenaGameState()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.2f;
    bAlwaysRelevant = true;
    SetNetUpdateFrequency(10.f);
}

void AArenaGameState::RefreshTeamSize()
{
    ConnectedPlayers = 0;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (It->IsValid() && It->Get()->PlayerState) ++ConnectedPlayers;
    if (ExtractionRoster.IsEmpty())
        TeamSize = FMath::Max(1, ConnectedPlayers);
}

void AArenaGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority()) RefreshTeamSize();
}

int64 AArenaGameState::GetTeamTotalValue() const
{
    int64 Total = 0;
    for (const FLootEntry& Entry : TeamInventory) Total += Entry.Value;
    return Total;
}

void AArenaGameState::AddTeamLoot(const FLootEntry& Entry)
{
    if (!HasAuthority() || bTeamComplete || bTeamFailed) return;
    TeamInventory.Add(Entry);
    ForceNetUpdate();
}

void AArenaGameState::ExtractPlayer(ULootCollectorComponent* Collector, const FString& Movie)
{
    if (!HasAuthority() || !Collector || bTeamComplete || bTeamFailed) return;
    APawn* Pawn = Cast<APawn>(Collector->GetOwner());
    AArenaGamePlayerController* PC = Pawn ? Cast<AArenaGamePlayerController>(Pawn->GetController()) : nullptr;
    if (!PC || !PC->PlayerState || EscapedPlayers.Contains(PC->PlayerState) || DeadPlayers.Contains(PC->PlayerState)) return;
    RefreshTeamSize();
    if (ConnectedPlayers < TeamSize) return;
    LockRoster();
    TeamEndingMovie = Movie;
    EscapedPlayers.AddUnique(PC->PlayerState);
    Collector->MarkExtracted();
    PC->ClientWatchTeammate();
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("Team extraction: %d/%d, money=%lld"), EscapedPlayers.Num(), TeamSize, GetTeamTotalValue());
    CheckTeamResolved();
}

void AArenaGameState::LockRoster()
{
    if (!ExtractionRoster.IsEmpty()) return;
    RefreshTeamSize();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (It->IsValid() && It->Get()->PlayerState) ExtractionRoster.AddUnique(It->Get()->PlayerState);
    TeamSize = ExtractionRoster.Num();
}

void AArenaGameState::PlayerDied(APlayerController* Controller)
{
    if (!HasAuthority() || !Controller || !Controller->PlayerState || bTeamComplete || bTeamFailed
        || DeadPlayers.Contains(Controller->PlayerState) || EscapedPlayers.Contains(Controller->PlayerState)) return;
    LockRoster();
    DeadPlayers.AddUnique(Controller->PlayerState);
    if (AArenaGamePlayerController* PC = Cast<AArenaGamePlayerController>(Controller)) PC->ClientWatchTeammate();
    ForceNetUpdate();
    CheckTeamResolved();
}

void AArenaGameState::CheckTeamResolved()
{
    if (bTeamComplete || bTeamFailed || EscapedPlayers.Num() + DeadPlayers.Num() != TeamSize) return;
    if (EscapedPlayers.IsEmpty())
    {
        AbortTeam(NSLOCTEXT("Team", "AllDead", "ALL PLAYERS WERE CAUGHT"));
        return;
    }
    bTeamComplete = true;
    TeamRunSeconds = GetWorld()->GetTimeSeconds();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (AArenaGamePlayerController* Member = Cast<AArenaGamePlayerController>(It->Get()))
            Member->ClientTeamEnding(TeamEndingMovie, TeamInventory, TeamRunSeconds);
    ForceNetUpdate();
}

void AArenaGameState::ConfirmResults(APlayerController* Controller)
{
    if (!HasAuthority() || !bTeamComplete || !Controller || !ExtractionRoster.Contains(Controller->PlayerState) || bReturningToMenu) return;
    ReadyPlayers.AddUnique(Controller->PlayerState);
    ResultsReady = ReadyPlayers.Num();
    ForceNetUpdate();
    if (ResultsReady != TeamSize) return;
    bReturningToMenu = true;
    FTimerHandle ReturnTimer;
    GetWorldTimerManager().SetTimer(ReturnTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        FString URL = TEXT("/Game/Maps/MainMenu?game=/Script/ArenaGame.MainMenuGameMode");
        if (GetNetMode() == NM_ListenServer) URL += TEXT("?listen");
        GetWorld()->ServerTravel(URL, false);
    }), 0.25f, false);
}

void AArenaGameState::PlayerDisconnected(APlayerController* Controller)
{
    if (!HasAuthority() || !Controller || !ExtractionRoster.Contains(Controller->PlayerState) || bReturningToMenu) return;
    AbortTeam(NSLOCTEXT("Team", "Disconnected", "TEAMMATE DISCONNECTED"));
}

void AArenaGameState::AbortTeam(const FText& Reason)
{
    if (!HasAuthority() || bReturningToMenu || GetWorld()->bIsTearingDown) return;
    bReturningToMenu = true;
    bTeamFailed = true;
    TeamFailureReason = Reason;
    const bool bAllPlayersDead = TeamSize > 0 && DeadPlayers.Num() == TeamSize && EscapedPlayers.IsEmpty();
    if (bAllPlayersDead)
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (AArenaGamePlayerController* Member = Cast<AArenaGamePlayerController>(It->Get()))
                Member->ClientMissionFailed();
    ForceNetUpdate();
    FTimerHandle ReturnTimer;
    GetWorldTimerManager().SetTimer(ReturnTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        FString URL = TEXT("/Game/Maps/MainMenu?game=/Script/ArenaGame.MainMenuGameMode");
        if (GetNetMode() == NM_ListenServer) URL += TEXT("?listen");
        GetWorld()->ServerTravel(URL, false);
        if (TeamSize == 1) GetWorld()->NextSwitchCountdown = 0.f;
    }), bAllPlayersDead ? 5.f : 2.5f, false);
}


void AArenaGameState::OnRep_RoundOver()
{
	if (!bRoundOver)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("ROUND OVER -- Winner PlayerID %d"),
		WinnerId
	);

	// Stop player input.
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);

		if (APawn* P = PC->GetPawn())
		{
			P->DisableInput(PC);

			if (ACharacter* Character = Cast<ACharacter>(P))
			{
				if (UCharacterMovementComponent* Movement =
					Character->GetCharacterMovement())
				{
					Movement->StopMovementImmediately();
				}
			}
		}
	}
}


void AArenaGameState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArenaGameState, bRoundOver);
	DOREPLIFETIME(AArenaGameState, WinnerId);
    DOREPLIFETIME(AArenaGameState, DeadPlayers);
    DOREPLIFETIME(AArenaGameState, TeamInventory);
    DOREPLIFETIME(AArenaGameState, EscapedPlayers);
    DOREPLIFETIME(AArenaGameState, TeamSize);
    DOREPLIFETIME(AArenaGameState, ConnectedPlayers);
    DOREPLIFETIME(AArenaGameState, bTeamComplete);
    DOREPLIFETIME(AArenaGameState, ResultsReady);
    DOREPLIFETIME(AArenaGameState, TeamRunSeconds);
    DOREPLIFETIME(AArenaGameState, bTeamFailed);
    DOREPLIFETIME(AArenaGameState, TeamFailureReason);
}
