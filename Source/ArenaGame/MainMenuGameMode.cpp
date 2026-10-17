#include "MainMenuGameMode.h"
#include "MainMenuWidget.h"

AMainMenuGameMode::AMainMenuGameMode()
{
    PlayerControllerClass = AMainMenuPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
    HUDClass = nullptr;
}

void AMainMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;
    Menu = CreateWidget<UMainMenuWidget>(this, UMainMenuWidget::StaticClass());
    if (!Menu) return;
    Menu->AddToPlayerScreen(100);
    bShowMouseCursor = true;
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(Menu->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    Menu->FocusPlayButton();
}
