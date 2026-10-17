#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuGameMode.generated.h"

class UMainMenuWidget;

UCLASS()
class ARENAGAME_API AMainMenuPlayerController : public APlayerController
{
    GENERATED_BODY()
protected:
    virtual void BeginPlay() override;

    UPROPERTY(Transient)
    TObjectPtr<UMainMenuWidget> Menu;
};

UCLASS()
class ARENAGAME_API AMainMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AMainMenuGameMode();
};
