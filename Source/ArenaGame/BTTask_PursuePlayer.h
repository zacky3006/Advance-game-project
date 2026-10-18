#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_PursuePlayer.generated.h"

/** Keep the chase branch active even when only a partial path is available. */
UCLASS()
class ARENAGAME_API UBTTask_PursuePlayer : public UBTTask_BlackboardBase
{
    GENERATED_BODY()
public:
    UBTTask_PursuePlayer();
protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
private:
    // Each AI owns its task instance; retries never leak between guards.
    double NextPathAttempt = 0.0;
    TWeakObjectPtr<AActor> MoveTarget;
};
