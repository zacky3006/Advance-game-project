#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TimerHandle.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UBorder;
class UMediaPlayer;
class UMediaSoundComponent;
class AActor;
class UImage;
class UMediaTexture;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class ARENAGAME_API UMainMenuBackdropWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
        const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
        const FWidgetStyle& Style, bool bParentEnabled) const override;
};

UCLASS()
class ARENAGAME_API UMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UMainMenuWidget(const FObjectInitializer& ObjectInitializer);
    void FocusPlayButton();
    void BeginTeamIntro();
    void UpdateIntroVotes(int32 Votes, int32 Members, bool bVoted);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
        const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
        const FWidgetStyle& Style, bool bParentEnabled) const override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    UFUNCTION()
    void PlayClicked();

    UFUNCTION()
    void QuitClicked();

    void OpenDemoMap();
    void StartIntro();
    void StopIntro();
    UFUNCTION()
    void FinishIntro();
    UFUNCTION()
    void IntroFailed(FString FailedUrl);
    UFUNCTION()
    void IntroOpened(FString OpenedUrl);

    UPROPERTY()
    TObjectPtr<UMaterialInterface> IntroMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> IntroDynamicMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMediaTexture> IntroTexture;
    UPROPERTY(Transient)
    TObjectPtr<UImage> IntroImage;

    UPROPERTY(Transient)
    TObjectPtr<UMediaPlayer> IntroPlayer;
    UPROPERTY(Transient)
    TObjectPtr<UMediaSoundComponent> IntroSound;
    UPROPERTY(Transient)
    TObjectPtr<AActor> IntroAudioActor;
    UPROPERTY(Transient)
    TObjectPtr<UBorder> IntroScreen;
    UPROPERTY(Transient)
    TObjectPtr<UBorder> IntroFade;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SkipLabel;
    bool bHasSkipVote = false;
    bool bIntroActive = false;
    bool bIntroFinished = false;
    float IntroWait = 0.f;
    bool bFadingToIntro = false;
    float IntroFadeElapsed = 0.f;

    UPROPERTY(Transient)
    TObjectPtr<UButton> PlayButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> QuitButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PlayLabel;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Status;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ContentPanel;

    FTimerHandle TravelTimer;
    float Elapsed = 0.f;
    bool bStartingGame = false;
};
