#include "MainMenuGameMode.h"
#include "MainMenuWidget.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AMainMenuGameMode::AMainMenuGameMode()
{
    PlayerControllerClass = AMainMenuPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
    HUDClass = nullptr;
}

void AMainMenuPlayerController::ServerRequestTeamGame_Implementation()
{
    if (AMainMenuGameMode* MenuMode = GetWorld()->GetAuthGameMode<AMainMenuGameMode>()) MenuMode->BeginTeamIntro();
}

void AMainMenuGameMode::StartTeamGame()
{
    if (!HasAuthority() || bStartingTeamGame) return;
    bStartingTeamGame = true;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (AMainMenuPlayerController* PC = Cast<AMainMenuPlayerController>(It->Get())) PC->ClientPrepareGameTravel();
    FString Destination = TEXT("/Game/Office_Pack_Vol_1/Maps/DemoMap?game=/Game/_AGD/BP_ArenaGameMode.BP_ArenaGameMode_C");
    if (GetNetMode() == NM_ListenServer) Destination += TEXT("?listen");
    if (GetNetMode() == NM_Standalone)
        UGameplayStatics::OpenLevel(this, TEXT("/Game/Office_Pack_Vol_1/Maps/DemoMap"), true,
            TEXT("listen?game=/Game/_AGD/BP_ArenaGameMode.BP_ArenaGameMode_C"));
    else GetWorld()->ServerTravel(Destination, false);
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
    if (bPendingIntro)
    {
        Menu->BeginTeamIntro();
        Menu->UpdateIntroVotes(IntroVotes, IntroMemberCount, bIntroVoted);
    }
}

void AMainMenuPlayerController::ServerVoteSkipIntro_Implementation()
{
    if (AMainMenuGameMode* Mode = GetWorld()->GetAuthGameMode<AMainMenuGameMode>()) Mode->VoteSkipIntro(this);
}

void AMainMenuPlayerController::ServerIntroFinished_Implementation()
{
    if (AMainMenuGameMode* Mode = GetWorld()->GetAuthGameMode<AMainMenuGameMode>()) Mode->IntroFinished(this);
}

void AMainMenuPlayerController::ClientBeginTeamIntro_Implementation()
{
    bPendingIntro = true;
    if (Menu) Menu->BeginTeamIntro();
}

void AMainMenuPlayerController::ClientIntroVoteStatus_Implementation(int32 Votes, int32 Members, bool bVoted)
{
    IntroVotes = Votes;
    IntroMemberCount = Members;
    bIntroVoted = bVoted;
    if (Menu) Menu->UpdateIntroVotes(Votes, Members, bVoted);
}

void AMainMenuPlayerController::ClientPrepareGameTravel_Implementation()
{
    if (Menu) Menu->RemoveFromParent();
    bShowMouseCursor = false;
    ResetIgnoreMoveInput();
    ResetIgnoreLookInput();
    SetInputMode(FInputModeGameOnly());
}

void AMainMenuGameMode::BeginTeamIntro()
{
    if (!HasAuthority() || bIntroRunning || bStartingTeamGame) return;
    bIntroRunning = true;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (AMainMenuPlayerController* PC = Cast<AMainMenuPlayerController>(It->Get()))
        {
            IntroMembers.Add(PC);
            PC->ClientBeginTeamIntro();
        }
    UpdateIntro();
}

void AMainMenuGameMode::VoteSkipIntro(AMainMenuPlayerController* Member)
{
    if (!bIntroRunning || bStartingTeamGame || !IntroMembers.Contains(Member)) return;
    SkipVotes.Add(Member);
    UpdateIntro();
}

void AMainMenuGameMode::IntroFinished(AMainMenuPlayerController* Member)
{
    if (!bIntroRunning || bStartingTeamGame || !IntroMembers.Contains(Member)) return;
    FinishedMembers.Add(Member);
    UpdateIntro();
}

void AMainMenuGameMode::UpdateIntro()
{
    if (!bIntroRunning || bStartingTeamGame || IntroMembers.IsEmpty()) return;
    for (const TWeakObjectPtr<AMainMenuPlayerController>& Member : IntroMembers)
        if (Member.IsValid()) Member->ClientIntroVoteStatus(SkipVotes.Num(), IntroMembers.Num(), SkipVotes.Contains(Member));
    if (SkipVotes.Num() == IntroMembers.Num() || FinishedMembers.Num() == IntroMembers.Num()) StartTeamGame();
}

void AMainMenuGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (bIntroRunning && !bStartingTeamGame)
        if (AMainMenuPlayerController* PC = Cast<AMainMenuPlayerController>(NewPlayer))
        {
            IntroMembers.Add(PC);
            PC->ClientBeginTeamIntro();
            UpdateIntro();
        }
}

void AMainMenuGameMode::Logout(AController* Exiting)
{
    if (AMainMenuPlayerController* PC = Cast<AMainMenuPlayerController>(Exiting))
    {
        IntroMembers.Remove(PC);
        SkipVotes.Remove(PC);
        FinishedMembers.Remove(PC);
    }
    Super::Logout(Exiting);
    UpdateIntro();
}
