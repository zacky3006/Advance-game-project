#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTTask_MeleeAttack.generated.h"

UCLASS()
class ARENAGAME_API UBTTTask_MeleeAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTTask_MeleeAttack();

	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;
};