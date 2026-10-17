#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LootCollectorComponent.h"
#include "EscapeSequenceWidget.generated.h"

class UBorder;
class UImage;
class UMediaPlayer;
class UMediaTexture;
class UMediaSoundComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class ARENAGAME_API UEscapeSequenceWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UEscapeSequenceWidget(const FObjectInitializer& Initializer);
    void BeginEnding(const FString& Movie);
    void SetRunResult(const TArray<FLootEntry>& Entries, double RunSeconds);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    void OpenMovie();
    void CleanupMedia();
    UFUNCTION()
    void MovieOpened(FString Url);
    UFUNCTION()
    void MovieFailed(FString Url);
    UFUNCTION()
    void Finish();
    UPROPERTY() TObjectPtr<UMaterialInterface> VideoMaterial;
    UPROPERTY(Transient) TObjectPtr<UBorder> Screen;
    UPROPERTY(Transient) TObjectPtr<UImage> Image;
    UPROPERTY(Transient) TObjectPtr<UMediaPlayer> Player;
    UPROPERTY(Transient) TObjectPtr<UMediaTexture> Texture;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
    UPROPERTY(Transient) TObjectPtr<UMediaSoundComponent> Sound;
    UPROPERTY(Transient) TObjectPtr<AActor> AudioActor;
    UPROPERTY(Transient) TArray<FLootEntry> ResultInventory;
    double ResultSeconds = 0.0;
    FString MovieName;
    float Elapsed = 0.f;
    float OpeningTime = 0.f;
    bool bOpened = false;
    bool bFinished = false;
    bool bLockedInput = false;
};
