#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoopPIETestLibrary.generated.h"
class ALootPickup;
class APlayerController;
class AActor;
class AArenaAIController;
class AArenaGameCharacter;
class APawn;
class UBehaviorTree;

/** Python's editor execution guard forces RPCs local. Queue native timer actions
 * so PIE integration tests exercise the real network path outside that guard. */
UCLASS()
class UCoopPIETestLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly), Category="Development|PIE")
    static void QueueAction(UObject* Target, FName Action, ALootPickup* Pickup = nullptr);
    UFUNCTION(BlueprintPure, meta=(DevelopmentOnly), Category="Development|PIE")
    static bool IsGameInputReady(APlayerController* Controller);
    UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly), Category="Development|PIE")
    static AActor* SpawnTestActor(UObject* Context, TSubclassOf<AActor> ActorClass, FTransform Transform);
    UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly), Category="Development|PIE")
    static void QueueSightEvent(AArenaAIController* Controller, AActor* Player, bool bVisible);
    UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly), Category="Development|PIE")
    static bool CheckPlayerVisibility(AArenaGameCharacter* Player, APawn* Observer, FVector& SeenLocation);
    UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly), Category="Development|PIE")
    static bool ConfigureHearingTree(UBehaviorTree* Tree);
    UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly), Category="Development|PIE")
    static bool ConfigurePursuitTree(UBehaviorTree* Tree);
};
