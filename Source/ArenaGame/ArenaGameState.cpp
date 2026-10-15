// Fill out your copyright notice in the Description page of Project Settings.

#include "ArenaGameState.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


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
}