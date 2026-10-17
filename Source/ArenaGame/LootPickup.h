#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootPickup.generated.h"

class UStaticMeshComponent;
class ULootCollectorComponent;
class UTexture2D;

UCLASS(Blueprintable)
class ARENAGAME_API ALootPickup : public AActor
{
    GENERATED_BODY()
public:
    ALootPickup();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loot")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot")
    FText ItemName = NSLOCTEXT("Loot", "DefaultItem", "Valuable");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot", meta=(ClampMin="0"))
    int32 Value = 100;

    /** Fallback artwork only when no static mesh is available; mesh previews take priority. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot")
    TObjectPtr<UTexture2D> ItemIcon;

    bool TryCollect(ULootCollectorComponent* Collector);

private:
    bool bCollected = false;
};
