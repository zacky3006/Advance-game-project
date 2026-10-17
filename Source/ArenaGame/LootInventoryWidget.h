#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PreviewScene.h"
#include "LootInventoryWidget.generated.h"

class ULootCollectorComponent;
class UVerticalBox;
class UTextBlock;
class UStaticMeshComponent;
class USceneCaptureComponent2D;
class UTexture;
class UTextureCube;
class UStaticMesh;
class UMaterialInterface;
struct FLootEntry;

UCLASS()
class ARENAGAME_API ULootInventoryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    ULootInventoryWidget(const FObjectInitializer& ObjectInitializer);
    UPROPERTY(Transient)
    TObjectPtr<ULootCollectorComponent> Collector;

    void RefreshInventory();
    void ReleasePreviews();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
    friend class FLootThumbnailPreviewTest;
    UFUNCTION()
    void CloseClicked();

    UFUNCTION()
    void SortClicked();

    UTexture* MakeThumbnail(const FLootEntry& Entry);

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> Rows;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CountText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ValueText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SortText;

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<UTexture>> ThumbnailCache;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> PreviewMesh;

    UPROPERTY(Transient)
    TObjectPtr<USceneCaptureComponent2D> PreviewCapture;

    UPROPERTY()
    TObjectPtr<UTextureCube> PreviewEnvironment;

    UPROPERTY()
    TObjectPtr<UStaticMesh> BackdropMeshAsset;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> BackdropMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> PreviewBackdrop;

    TUniquePtr<FPreviewScene> ThumbnailScene;

    TWeakObjectPtr<AActor> PreviewActor;
    bool bSortByValue = true;
    float OpenTime = 0.f;
};
