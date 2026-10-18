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
public:
    UFUNCTION(Server, Reliable)
    void ServerRequestTeamGame();
    UFUNCTION(Server, Reliable)
    void ServerVoteSkipIntro();
    UFUNCTION(Server, Reliable)
    void ServerIntroFinished();
    UFUNCTION(Client, Reliable)
    void ClientBeginTeamIntro();
    UFUNCTION(Client, Reliable)
    void ClientIntroVoteStatus(int32 Votes, int32 Members, bool bVoted);
    UFUNCTION(Client, Reliable)
    void ClientPrepareGameTravel();
protected:
    virtual void BeginPlay() override;

    UPROPERTY(Transient)
    TObjectPtr<UMainMenuWidget> Menu;
    bool bPendingIntro = false;
    int32 IntroVotes = 0;
    int32 IntroMemberCount = 1;
    bool bIntroVoted = false;
};

UCLASS()
class ARENAGAME_API AMainMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AMainMenuGameMode();
    void StartTeamGame();
    void BeginTeamIntro();
    void VoteSkipIntro(AMainMenuPlayerController* Member);
    void IntroFinished(AMainMenuPlayerController* Member);
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
private:
    void UpdateIntro();
    bool bIntroRunning = false;
    bool bStartingTeamGame = false;
    TSet<TWeakObjectPtr<AMainMenuPlayerController>> IntroMembers;
    TSet<TWeakObjectPtr<AMainMenuPlayerController>> SkipVotes;
    TSet<TWeakObjectPtr<AMainMenuPlayerController>> FinishedMembers;
};
