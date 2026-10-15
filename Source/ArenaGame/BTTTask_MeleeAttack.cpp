#include "BTTTask_MeleeAttack.h"

#include "AIController.h"
#include "ArenaGameCharacter.h"

UBTTTask_MeleeAttack::UBTTTask_MeleeAttack()
{
	NodeName = TEXT("Melee Attack");
}

EBTNodeResult::Type UBTTTask_MeleeAttack::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	// =====================================================
	// BLANK 5
	// Who is running this Task?
	// =====================================================

	AAIController* AICon = OwnerComp.GetAIOwner();

	if (AICon == nullptr)
	{
		return EBTNodeResult::Failed;
	}


	// =====================================================
	// BLANK 6
	// What character is this AI Controller driving?
	// =====================================================

	AArenaGameCharacter* Me =
		Cast<AArenaGameCharacter>(AICon->GetPawn());

	if (Me == nullptr)
	{
		return EBTNodeResult::Failed;
	}


	// =====================================================
	// BLANK 7
	// Tell the character to swing
	// =====================================================

	Me->ServerMeleeAttack();


	return EBTNodeResult::Succeeded;
}