#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LootCollectorComponent.generated.h"

class ALootPickup;
class UUserWidget;
class UTextBlock;
class UBorder;
class UStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class UTexture2D;
class ULootInventoryWidget;
class APlayerController;
class AEscapePoint;

USTRUCT(BlueprintType)
struct FLootEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Loot")
    FText ItemName;

    UPROPERTY(BlueprintReadOnly, Category="Loot")
    int32 Value = 0;

    UPROPERTY(BlueprintReadOnly, Category="Loot")
    TObjectPtr<UTexture2D> Icon;

    UPROPERTY()
    TObjectPtr<UStaticMesh> PreviewMesh;

    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> PreviewMaterials;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLootInventoryChanged);

UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class ARENAGAME_API ULootCollectorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULootCollectorComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot", meta=(ClampMin="1", Units="cm"))
    float PickupRange = 200.f;

    /** Maximum angle from the camera for a visible, collectable target. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot", meta=(ClampMin="5", ClampMax="89", Units="deg"))
    float FocusHalfAngle = 65.f;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Inventory, Category="Loot")
    TArray<FLootEntry> Inventory;

    UPROPERTY(BlueprintAssignable, Category="Loot")
    FLootInventoryChanged OnInventoryChanged;

    UFUNCTION(BlueprintPure, Category="Loot")
    int64 GetTotalValue() const;

    UFUNCTION(BlueprintCallable, Category="Loot")
    void TryInteract();

    UFUNCTION(BlueprintCallable, Category="Escape")
    void TryEscape();
    bool IsEndingGame() const { return bEndingGame || bEscaped; }
    UFUNCTION(BlueprintPure, Category="Team")
    bool HasEscaped() const { return bEscaped; }
    void MarkExtracted();
    void PrepareTeamEnding();

    UFUNCTION(BlueprintCallable, Category="Loot")
    void ToggleInventory();

    UFUNCTION(BlueprintPure, Category="Loot")
    bool IsInventoryOpen() const;

    void CloseInventory();
    void RecordPickup(const ALootPickup* Pickup);

    bool CanCollect(const ALootPickup* Pickup) const;
    void RecordLoot(const FText& Name, int32 Value);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class UCoopPIETestLibrary;
    UPROPERTY(ReplicatedUsing=OnRep_Escaped)
    bool bEscaped = false;
    UFUNCTION()
    void OnRep_Escaped();
    void SyncTeamInventory();
    UFUNCTION(Server, Reliable)
    void ServerEscape(AEscapePoint* Point);
    UFUNCTION(Client, Reliable)
    void ClientBeginEscape(const FString& Movie);
    bool bEndingGame = false;

    UFUNCTION(Server, Reliable)
    void ServerCollect(ALootPickup* Pickup);

    UFUNCTION()
    void OnRep_Inventory();

    ALootPickup* FindPickup() const;
    void UpdateFocusedPickup();
    void UpdateHUD();

    TWeakObjectPtr<ALootPickup> FocusedPickup;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> OutlineMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> OutlineMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> HUD;

    UPROPERTY(Transient)
    TObjectPtr<ULootInventoryWidget> InventoryWidget;

    bool bPreviousMouseCursor = false;
    TWeakObjectPtr<APlayerController> InventoryController;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HUDText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HUDValue;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PickupPrompt;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PickupValue;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> LootPanel;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> PromptPanel;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> GainText;

    int32 LastInventoryCount = 0;
    int64 LastTotalValue = 0;
    int32 PendingGainCount = 0;
    int64 PendingGainValue = 0;
    double FeedbackStartTime = -100.0;
    TWeakObjectPtr<ALootPickup> LastPromptPickup;
    double PromptStartTime = -100.0;
};
