#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapePoint.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ULootCollectorComponent;

UCLASS(Blueprintable)
class ARENAGAME_API AEscapePoint : public AActor
{
    GENERATED_BODY()
public:
    AEscapePoint();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Escape")
    TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Escape", meta=(ClampMin="0"))
    int32 RequiredValue = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Escape", meta=(ClampMin="1", Units="cm"))
    float InteractionRange = 200.f;
    /** Relative to Content/Movies. Missing movie returns to the menu after fading. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Escape")
    FString EndingMovie = TEXT("Ending.mp4");

    bool CanInteract(const ULootCollectorComponent* Collector) const;
    bool HasEnoughLoot(const ULootCollectorComponent* Collector) const;
private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Outline;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Prompt;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OutlineMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> PadMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> PadDynamic;
};
