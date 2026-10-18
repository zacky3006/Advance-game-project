#include "BTTask_PursuePlayer.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"

UBTTask_PursuePlayer::UBTTask_PursuePlayer()
{
    NodeName = TEXT("Pursue player / wait at reachable edge");
    bCreateNodeInstance = true;
    bNotifyTick = true;
    BlackboardKey.SelectedKeyName = TEXT("TargetActor");
    BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PursuePlayer, BlackboardKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_PursuePlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    NextPathAttempt = 0.0;
    MoveTarget.Reset();
    if (AAIController* AI = OwnerComp.GetAIOwner()) AI->StopMovement();
    return EBTNodeResult::InProgress;
}

void UBTTask_PursuePlayer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AAIController* AI = OwnerComp.GetAIOwner();
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(GetSelectedBlackboardKey())) : nullptr;
    if (!AI || !AI->GetPawn() || !IsValid(Target))
    {
        if (AI) { AI->StopMovement(); AI->ClearFocus(EAIFocusPriority::Gameplay); }
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    if (MoveTarget.Get() != Target)
    {
        AI->StopMovement();
        MoveTarget = Target;
        NextPathAttempt = 0.0;
    }
    AI->SetFocus(Target, EAIFocusPriority::Gameplay);
    // Finish only when the real player is in melee reach, never merely because
    // navigation reached the end of a partial path at the mouth of a gap.
    if (FVector::Dist(AI->GetPawn()->GetActorLocation(), Target->GetActorLocation()) <= 140.f && AI->LineOfSightTo(Target))
    {
        AI->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    const double Now = AI->GetWorld()->GetTimeSeconds();
    if (Now < NextPathAttempt) return;
    NextPathAttempt = Now + .75;
    // Partial paths end at the closest reachable boundary. Do not require the
    // player's endpoint to lie on the guard's navmesh (e.g. a narrow sofa gap).
    // An active actor path already tracks movement; do not restart it every tick.
    if (AI->GetMoveStatus() != EPathFollowingStatus::Idle) return;
    FAIMoveRequest Request(Target);
    Request.SetUsePathfinding(true);
    Request.SetNavigationFilter(AI->GetDefaultNavigationFilterClass());
    Request.SetAllowPartialPath(true);
    Request.SetRequireNavigableEndLocation(false);
    Request.SetProjectGoalLocation(false);
    Request.SetAcceptanceRadius(30.f);
    Request.SetReachTestIncludesAgentRadius(false);
    Request.SetReachTestIncludesGoalRadius(false);
    AI->MoveTo(Request);
    // Failed/partial-complete moves deliberately leave this task running.
    // Blackboard target removal still aborts the branch after sight memory ends.
}

EBTNodeResult::Type UBTTask_PursuePlayer::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (AAIController* AI = OwnerComp.GetAIOwner())
    {
        AI->StopMovement();
        AI->ClearFocus(EAIFocusPriority::Gameplay);
    }
    return EBTNodeResult::Aborted;
}
