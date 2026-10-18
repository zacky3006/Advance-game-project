#include "CoopPIETestLibrary.h"
#include "LootCollectorComponent.h"
#include "LootPickup.h"
#include "RunResultsWidget.h"
#include "MainMenuGameMode.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "TimerManager.h"
#include "ArenaGameCharacter.h"
#include "ArenaAIController.h"
#include "Perception/AISense_Sight.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BehaviorTree.h"
#include "UObject/UnrealType.h"
#include "BTTask_PursuePlayer.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#if WITH_EDITOR
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#endif

bool UCoopPIETestLibrary::ConfigurePursuitTree(UBehaviorTree* Tree)
{
#if WITH_EDITOR
    if (!Tree || Tree->GetPathName() != TEXT("/Game/_Student/AI/BT_ArenaAI.BT_ArenaAI") || !Tree->RootNode) return false;
    Tree->Modify();
    int32 Count = 0;
    TFunction<void(UBTCompositeNode*)> Visit = [&](UBTCompositeNode* Composite)
    {
        for (FBTCompositeChild& Child : Composite->Children)
        {
            if (Child.ChildComposite) Visit(Child.ChildComposite);
            if (Cast<UBTTask_PursuePlayer>(Child.ChildTask)) { ++Count; continue; }
            UBTTask_MoveTo* Old = Cast<UBTTask_MoveTo>(Child.ChildTask);
            if (!Old || Old->GetSelectedBlackboardKey() != TEXT("TargetActor")) continue;
            auto* Task = NewObject<UBTTask_PursuePlayer>(Tree, NAME_None, RF_Transactional);
            Task->Services = Old->Services;
            Composite->Modify();
            Child.ChildTask = Task;
            // Keep the editor graph in sync so later edits/compilation retain it.
            if (UEdGraph* Graph = Cast<UEdGraph>(Tree->BTGraph))
                for (UEdGraphNode* Node : Graph->Nodes)
                    if (FObjectPropertyBase* Instance = FindFProperty<FObjectPropertyBase>(Node->GetClass(), TEXT("NodeInstance"));
                        Instance && Instance->GetObjectPropertyValue_InContainer(Node) == Old)
                    {
                        Node->Modify();
                        Instance->SetObjectPropertyValue_InContainer(Node, Task);
                    }
            ++Count;
        }
    };
    Visit(Tree->RootNode);
    Tree->MarkPackageDirty();
    return Count == 1;
#else
    return false;
#endif
}

bool UCoopPIETestLibrary::ConfigureHearingTree(UBehaviorTree* Tree)
{
#if WITH_EDITOR
    if (!Tree || Tree->GetPathName() != TEXT("/Game/_Student/AI/BT_ArenaAI.BT_ArenaAI")) return false;
    UObject* Move = StaticFindObject(UObject::StaticClass(), Tree, TEXT("BTTask_MoveTo_3"));
    UObject* Decorator = StaticFindObject(UObject::StaticClass(), Tree, TEXT("BTDecorator_Blackboard_2"));
    if (!Move || !Decorator) return false;
    FBoolProperty* Observe = FindFProperty<FBoolProperty>(Move->GetClass(), TEXT("bObserveBlackboardValue"));
    FByteProperty* Notify = FindFProperty<FByteProperty>(Decorator->GetClass(), TEXT("NotifyObserver"));
    if (!Observe || !Notify) return false;
    Tree->Modify(); Move->Modify(); Decorator->Modify();
    Observe->SetPropertyValue_InContainer(Move, true);
    // EBTBlackboardRestart::ValueChange = 0; UE's enum is not Python-exported.
    Notify->SetPropertyValue_InContainer(Decorator, 0);
    Tree->MarkPackageDirty();
    return Observe->GetPropertyValue_InContainer(Move) && Notify->GetPropertyValue_InContainer(Decorator) == 0;
#else
    return false;
#endif
}

bool UCoopPIETestLibrary::CheckPlayerVisibility(AArenaGameCharacter* Player, APawn* Observer, FVector& SeenLocation)
{
    SeenLocation = FVector::ZeroVector;
#if WITH_EDITOR
    if (!Player || !Observer || Player->GetWorld()->WorldType != EWorldType::PIE) return false;
    FCanBeSeenFromContext Context;
    Context.ObserverLocation = Observer->GetPawnViewLocation();
    Context.IgnoreActor = Observer;
    int32 Checks = 0, AsyncChecks = 0;
    float Strength = 0.f;
    return Player->CanBeSeenFrom(Context, SeenLocation, Checks, AsyncChecks, Strength) == UAISense_Sight::EVisibilityResult::Visible;
#else
    return false;
#endif
}

void UCoopPIETestLibrary::QueueAction(UObject* Target, FName Action, ALootPickup* Pickup)
{
#if WITH_EDITOR
    if (!IsValid(Target) || !Target->GetWorld() || Target->GetWorld()->WorldType != EWorldType::PIE) return;
    Target->GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(Target,
        [WeakTarget = TWeakObjectPtr<UObject>(Target), WeakPickup = TWeakObjectPtr<ALootPickup>(Pickup), Action]()
    {
        UObject* Object = WeakTarget.Get();
        if (Action == TEXT("ReturnMenu") && Object && Object->GetWorld()->GetAuthGameMode())
        {
            Object->GetWorld()->ServerTravel(TEXT("/Game/Maps/MainMenu?game=/Script/ArenaGame.MainMenuGameMode?listen"), false);
            return;
        }
        if (AArenaGameCharacter* Pawn = Cast<AArenaGameCharacter>(Object); Pawn && Action == TEXT("TogglePerspective")) Pawn->ToggleCameraPerspective();
        if (AArenaGameCharacter* Pawn = Cast<AArenaGameCharacter>(Object); Pawn && Action == TEXT("AIHit"))
        {
            for (TActorIterator<AArenaAIController> It(Pawn->GetWorld()); It; ++It)
            {
                Pawn->SetCanBeDamaged(true);
                UGameplayStatics::ApplyDamage(Pawn, 1.f, *It, It->GetPawn(), nullptr);
                break;
            }
        }
        if (ULootCollectorComponent* Loot = Cast<ULootCollectorComponent>(Object))
        {
            if (Action == TEXT("Collect") && WeakPickup.IsValid()) Loot->ServerCollect(WeakPickup.Get());
            if (Action == TEXT("Escape")) Loot->TryEscape();
        }
        if (URunResultsWidget* Results = Cast<URunResultsWidget>(Object); Results && Action == TEXT("Finish")) Results->FinishClicked();
        if (AMainMenuPlayerController* PC = Cast<AMainMenuPlayerController>(Object))
        {
            if (Action == TEXT("VoteSkip")) PC->ServerVoteSkipIntro();
            if (Action == TEXT("IntroFinished")) PC->ServerIntroFinished();
        }
        if (AMainMenuPlayerController* Menu = Cast<AMainMenuPlayerController>(Object); Menu && Action == TEXT("Start")) Menu->ServerRequestTeamGame();
    }));
#endif
}

bool UCoopPIETestLibrary::IsGameInputReady(APlayerController* Controller)
{
#if WITH_EDITOR
    if (!Controller || !Controller->GetWorld() || Controller->GetWorld()->WorldType != EWorldType::PIE) return false;
    ULocalPlayer* Player = Controller->GetLocalPlayer();
    return Player && Player->ViewportClient && !Player->ViewportClient->IgnoreInput()
        && !Controller->IsMoveInputIgnored() && !Controller->IsLookInputIgnored();
#else
    return false;
#endif
}

AActor* UCoopPIETestLibrary::SpawnTestActor(UObject* Context, TSubclassOf<AActor> ActorClass, FTransform Transform)
{
#if WITH_EDITOR
    UWorld* World = Context ? Context->GetWorld() : nullptr;
    if (!World || World->WorldType != EWorldType::PIE || !World->GetAuthGameMode() || !ActorClass) return nullptr;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return World->SpawnActor<AActor>(ActorClass, Transform, Params);
#else
    return nullptr;
#endif
}

void UCoopPIETestLibrary::QueueSightEvent(AArenaAIController* Controller, AActor* Player, bool bVisible)
{
#if WITH_EDITOR
    if (!IsValid(Controller) || !IsValid(Player) || !Controller->GetWorld()
        || Controller->GetWorld()->WorldType != EWorldType::PIE || !Controller->HasAuthority()) return;
    Controller->GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(Controller,
        [Controller, Target = TWeakObjectPtr<AActor>(Player), bVisible]()
    {
        if (!Target.IsValid()) return;
        const UAISense_Sight* Sight = GetDefault<UAISense_Sight>();
        FAIStimulus Stimulus(*Sight, 1.f, Target->GetActorLocation(), Controller->GetPawn() ?
            Controller->GetPawn()->GetActorLocation() : FVector::ZeroVector,
            bVisible ? FAIStimulus::SensingSucceeded : FAIStimulus::SensingFailed);
        Controller->OnTargetSensed(Target.Get(), Stimulus);
    }));
#endif
}
