#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "LootCollectorComponent.h"
#include "ArenaGamePlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UPlayerAlertHUD;
class AActor;

UCLASS()
class ARENAGAME_API AArenaGamePlayerController
    : public APlayerController,
      public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(Client, Reliable)
    void ClientWatchTeammate();
    UFUNCTION(Client, Reliable)
    void ClientMissionFailed();
    UFUNCTION(Client, Reliable)
    void ClientTeamEnding(const FString& Movie, const TArray<FLootEntry>& Entries, double Seconds);
    UFUNCTION(Server, Reliable)
    void ServerConfirmResults();
    virtual void PlayerTick(float DeltaTime) override;
private:
    bool bTeamSpectating = false;
    bool bTeamEndingStarted = false;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> TeamHUD;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> TeamEnding;
public:
    // LAB 11: Player is on Team 0
    virtual FGenericTeamId GetGenericTeamId() const override;

    // AI ALERT HUD: Local HUD update functions
    UFUNCTION(BlueprintCallable, Category = "AI Alert")
    void ShowAIAlert(AActor* AIActor, float AlertPercent);

    UFUNCTION(BlueprintCallable, Category = "AI Alert")
    void HideAIAlert(AActor* AIActor);

    // AI ALERT HUD: Server sends updates to the owning client
    UFUNCTION(Client, Reliable)
    void ClientShowAIAlert(AActor* AIActor, float AlertPercent);

    UFUNCTION(Client, Reliable)
    void ClientHideAIAlert(AActor* AIActor);

protected:
    // GAMEPLAY
    virtual void BeginPlay() override;

    // AI ALERT HUD
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Alert")
    TSubclassOf<UPlayerAlertHUD> PlayerAlertHUDClass;

    UPROPERTY()
    TObjectPtr<UPlayerAlertHUD> PlayerAlertHUDInstance = nullptr;

    // INPUT MAPPING
    UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
    TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

    UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
    TArray<TObjectPtr<UInputMappingContext>> MobileExcludedMappingContexts;

    // TOUCH CONTROLS
    UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
    TSubclassOf<UUserWidget> MobileControlsWidgetClass;

    UPROPERTY()
    TObjectPtr<UUserWidget> MobileControlsWidget = nullptr;

    UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
    bool bForceTouchControls = false;

    // INPUT
    virtual void SetupInputComponent() override;

    bool ShouldUseTouchControls() const;
};
