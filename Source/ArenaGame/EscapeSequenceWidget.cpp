#include "EscapeSequenceWidget.h"
#include "RunResultsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "MediaSoundComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/ConstructorHelpers.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"

UEscapeSequenceWidget::UEscapeSequenceWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Video(TEXT("/Game/UI/Materials/M_IntroVideo.M_IntroVideo"));
    VideoMaterial = Video.Object;
}

void UEscapeSequenceWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    Screen = WidgetTree->ConstructWidget<UBorder>();
    Screen->SetBrushColor(FLinearColor::Black);
    Screen->SetPadding(FMargin(0.f));
    WidgetTree->RootWidget = Screen;
    UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
    Fit->SetStretch(EStretch::ScaleToFit);
    Screen->SetContent(Fit);
    Image = WidgetTree->ConstructWidget<UImage>();
    Image->SetVisibility(ESlateVisibility::Hidden);
    Fit->SetContent(Image);
    SetRenderOpacity(0.f);
}

void UEscapeSequenceWidget::BeginEnding(const FString& Movie)
{
    MovieName = Movie;
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);
        bLockedInput = true;
        PC->bShowMouseCursor = false;
        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(TakeWidget());
        PC->SetInputMode(Mode);
        SetKeyboardFocus();
    }
}

void UEscapeSequenceWidget::SetRunResult(const TArray<FLootEntry>& Entries, double RunSeconds)
{
    ResultInventory = Entries;
    ResultSeconds = FMath::Max(0.0, RunSeconds);
}

void UEscapeSequenceWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (bFinished) return;
    Elapsed += DeltaTime;
    const float Alpha = FMath::Clamp(Elapsed, 0.f, 1.f);
    SetRenderOpacity(Alpha * Alpha * (3.f - 2.f * Alpha));
    if (Elapsed >= 1.15f && !bOpened)
    {
        bOpened = true;
        OpenMovie();
    }
    if (Player)
    {
        OpeningTime += DeltaTime;
        if (Sound) Sound->UpdatePlayer();
        if (OpeningTime > 10.f && !Player->IsPlaying() && Player->GetTime().GetTotalSeconds() <= 0.01) Finish();
    }
}

void UEscapeSequenceWidget::OpenMovie()
{
    // Only a file name is accepted: ending videos belong in packaged Movies.
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Movies") / FPaths::GetCleanFilename(MovieName));
    if (MovieName.IsEmpty() || !IFileManager::Get().FileExists(*Path) || !VideoMaterial)
    {
        UE_LOG(LogTemp, Display, TEXT("Ending: no movie supplied; showing run summary after fade."));
        Finish();
        return;
    }
    Player = NewObject<UMediaPlayer>(this);
    Player->SetDesiredPlayerName(TEXT("WmfMedia"));
    Player->PlayOnOpen = true;
    Player->SetLooping(false);
    Player->OnEndReached.AddDynamic(this, &UEscapeSequenceWidget::Finish);
    Player->OnMediaOpened.AddDynamic(this, &UEscapeSequenceWidget::MovieOpened);
    Player->OnMediaOpenFailed.AddDynamic(this, &UEscapeSequenceWidget::MovieFailed);
    Texture = NewObject<UMediaTexture>(this);
    Texture->NewStyleOutput = true;
    Texture->SetMediaPlayer(Player);
    Texture->UpdateResource();
    Material = UMaterialInstanceDynamic::Create(VideoMaterial, this);
    Material->SetTextureParameterValue(TEXT("Video"), Texture);
    Image->SetBrushFromMaterial(Material);
    Image->SetDesiredSizeOverride(FVector2D(1920.f, 1080.f));
    Image->SetVisibility(ESlateVisibility::HitTestInvisible);
    AudioActor = GetWorld()->SpawnActor<AActor>();
    if (!AudioActor) { Finish(); return; }
    Sound = NewObject<UMediaSoundComponent>(AudioActor);
    AudioActor->AddInstanceComponent(Sound);
    Sound->bIsUISound = true;
    Sound->bAllowSpatialization = false;
    Sound->SetMediaPlayer(Player);
    Sound->RegisterComponent();
    Sound->Start();
    if (!Player->OpenFile(Path)) MovieFailed(Path);
}

void UEscapeSequenceWidget::MovieOpened(FString Url)
{
    if (!Player || bFinished) return;
    if (Player->GetNumTracks(EMediaPlayerTrack::Audio) > 0) Player->SelectTrack(EMediaPlayerTrack::Audio, 0);
    if (Player->GetNumTracks(EMediaPlayerTrack::Video) > 0)
    {
        Player->SelectTrack(EMediaPlayerTrack::Video, 0);
        const FIntPoint Size = Player->GetVideoTrackDimensions(0, 0);
        if (Size.X > 0 && Size.Y > 0) Image->SetDesiredSizeOverride(FVector2D(Size.X, Size.Y));
    }
    if (Sound) { Sound->UpdatePlayer(); Sound->Start(); }
    Player->Play();
}

void UEscapeSequenceWidget::MovieFailed(FString Url)
{
    UE_LOG(LogTemp, Warning, TEXT("Ending movie unavailable: %s"), *Url);
    Finish();
}

void UEscapeSequenceWidget::CleanupMedia()
{
    if (Sound) { Sound->Stop(); Sound->SetMediaPlayer(nullptr); }
    if (Player)
    {
        Player->OnEndReached.RemoveAll(this);
        Player->OnMediaOpened.RemoveAll(this);
        Player->OnMediaOpenFailed.RemoveAll(this);
        Player->Close();
    }
    if (IsValid(AudioActor)) AudioActor->Destroy();
    if (Texture) Texture->SetMediaPlayer(nullptr);
    Sound = nullptr;
    AudioActor = nullptr;
    Player = nullptr;
}

void UEscapeSequenceWidget::Finish()
{
    if (bFinished) return;
    bFinished = true;
    CleanupMedia();
    SetRenderOpacity(1.f);
    if (URunResultsWidget* Results = CreateWidget<URunResultsWidget>(GetOwningPlayer(), URunResultsWidget::StaticClass()))
    {
        Screen->SetContent(Results);
        Results->Populate(ResultInventory, ResultSeconds);
        Results->FocusFinish();
    }
}

FReply UEscapeSequenceWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (bFinished) return Super::NativeOnPreviewKeyDown(Geometry, Event);
    if (bOpened && Event.GetKey() == EKeys::SpaceBar && !Event.IsRepeat()) Finish();
    return FReply::Handled();
}

void UEscapeSequenceWidget::NativeDestruct()
{
    CleanupMedia();
    if (bLockedInput)
        if (APlayerController* PC = GetOwningPlayer())
        {
            PC->SetIgnoreMoveInput(false);
            PC->SetIgnoreLookInput(false);
        }
    bLockedInput = false;
    Super::NativeDestruct();
}
