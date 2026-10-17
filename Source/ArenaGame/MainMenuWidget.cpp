#include "MainMenuWidget.h"
#include "LootUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Rendering/DrawElementTypes.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "Misc/Paths.h"
#include "GameFramework/Actor.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "MediaTexture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const FName DemoMap(TEXT("/Game/Office_Pack_Vol_1/Maps/DemoMap"));

    UTextBlock* MenuText(UWidgetTree* Tree, const FText& Text, int32 Size, FLinearColor Color)
    {
        UTextBlock* Label = Tree->ConstructWidget<UTextBlock>();
        Label->SetText(Text);
        Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Size));
        Label->SetColorAndOpacity(FSlateColor(Color));
        return Label;
    }
}

UMainMenuWidget::UMainMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> VideoMaterial(
        TEXT("/Game/UI/Materials/M_IntroVideo.M_IntroVideo"));
    IntroMaterial = VideoMaterial.Object;
}

void UMainMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;

    // NativePaint runs after a widget's children. Keep artwork in its own first
    // canvas child so it cannot cover the menu's text and interactive controls.
    UMainMenuBackdropWidget* Backdrop = CreateWidget<UMainMenuBackdropWidget>(GetOwningPlayer(), UMainMenuBackdropWidget::StaticClass());
    Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
    Backdrop->ForceVolatile(true);
    UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop);
    BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    BackdropSlot->SetOffsets(FMargin(0.f));
    BackdropSlot->SetZOrder(-10);

    ContentPanel = WidgetTree->ConstructWidget<UBorder>();
    ContentPanel->SetPadding(FMargin(0.f));
    FSlateBrush NoFrame;
    NoFrame.DrawAs = ESlateBrushDrawType::NoDrawType;
    ContentPanel->SetBrush(NoFrame);
    ContentPanel->SetBrushColor(FLinearColor::Transparent);
    ContentPanel->SetVerticalAlignment(VAlign_Center);
    UCanvasPanelSlot* ContentSlot = Canvas->AddChildToCanvas(ContentPanel);
    ContentSlot->SetAnchors(FAnchors(0.08f, 0.45f));
    ContentSlot->SetAlignment(FVector2D(0.f, 0.5f));
    ContentSlot->SetPosition(FVector2D::ZeroVector);
    ContentSlot->SetSize(FVector2D(860.f, 480.f));
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    ContentPanel->SetContent(Content);
    UTextBlock* Title = MenuText(WidgetTree, NSLOCTEXT("Menu", "Title", "PLACEHOLDER"), 76, FLinearColor::White);
    FSlateFontInfo TitleFont = Title->GetFont();
    TitleFont.LetterSpacing = 100;
    Title->SetFont(TitleFont);
    Title->SetJustification(ETextJustify::Left);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 0.f, 0.f, 48.f));

    FButtonStyle PlayStyle;
    FSlateBrush Normal = LootUITheme::CardBrush();
    Normal.TintColor = FSlateColor(LootUITheme::Gold);
    FSlateBrush Hover = Normal;
    Hover.TintColor = FSlateColor(FLinearColor(1.f, 0.9f, 0.52f));
    FSlateBrush Pressed = Normal;
    Pressed.TintColor = FSlateColor(FLinearColor(0.75f, 0.49f, 0.08f));
    PlayStyle.SetNormal(Normal).SetHovered(Hover).SetPressed(Pressed).SetDisabled(Normal);
    PlayStyle.SetNormalPadding(FMargin(22.f, 17.f)).SetPressedPadding(FMargin(22.f, 18.f, 22.f, 16.f));
    PlayButton = WidgetTree->ConstructWidget<UButton>();
    PlayButton->SetStyle(PlayStyle);
    PlayLabel = MenuText(WidgetTree, NSLOCTEXT("Menu", "PlayLeft", "PLAY GAME"), 28, LootUITheme::Panel);
    PlayLabel->SetJustification(ETextJustify::Left);
    PlayButton->SetContent(PlayLabel);
    PlayButton->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
    PlayButton->OnClicked.AddDynamic(this, &UMainMenuWidget::PlayClicked);
    USizeBox* PlaySize = WidgetTree->ConstructWidget<USizeBox>();
    PlaySize->SetWidthOverride(400.f);
    PlaySize->SetHeightOverride(82.f);
    PlaySize->SetContent(PlayButton);
    UVerticalBoxSlot* PlaySlot = Content->AddChildToVerticalBox(PlaySize);
    PlaySlot->SetHorizontalAlignment(HAlign_Left);
    PlaySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));

    FButtonStyle QuitStyle = PlayStyle;
    Normal.TintColor = FSlateColor(LootUITheme::Surface);
    Hover.TintColor = FSlateColor(FLinearColor(0.09f, 0.11f, 0.15f));
    QuitStyle.SetNormal(Normal).SetHovered(Hover).SetPressed(Normal).SetDisabled(Normal);
    QuitButton = WidgetTree->ConstructWidget<UButton>();
    QuitButton->SetStyle(QuitStyle);
    UTextBlock* QuitLabel = MenuText(WidgetTree, NSLOCTEXT("Menu", "QuitLeft", "QUIT"), 28, LootUITheme::Muted);
    QuitLabel->SetJustification(ETextJustify::Left);
    QuitButton->SetContent(QuitLabel);
    QuitButton->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
    QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::QuitClicked);
    USizeBox* QuitSize = WidgetTree->ConstructWidget<USizeBox>();
    QuitSize->SetWidthOverride(280.f);
    QuitSize->SetHeightOverride(72.f);
    QuitSize->SetContent(QuitButton);
    Content->AddChildToVerticalBox(QuitSize)->SetHorizontalAlignment(HAlign_Left);

    Status = MenuText(WidgetTree, FText::GetEmpty(), 10, LootUITheme::Muted);
    Status->SetVisibility(ESlateVisibility::Collapsed);
    Status->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Status)->SetPadding(FMargin(2.f, 18.f, 0.f, 0.f));

    IntroScreen = WidgetTree->ConstructWidget<UBorder>();
    IntroScreen->SetBrushColor(FLinearColor::Black);
    IntroScreen->SetPadding(FMargin(0.f));
    UCanvasPanelSlot* IntroSlot = Canvas->AddChildToCanvas(IntroScreen);
    IntroSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    IntroSlot->SetOffsets(FMargin(0.f));
    IntroSlot->SetZOrder(100);
    UCanvasPanel* IntroCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    IntroScreen->SetContent(IntroCanvas);
    UScaleBox* VideoFit = WidgetTree->ConstructWidget<UScaleBox>();
    VideoFit->SetStretch(EStretch::ScaleToFit);
    UCanvasPanelSlot* VideoSlot = IntroCanvas->AddChildToCanvas(VideoFit);
    VideoSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    VideoSlot->SetOffsets(FMargin(0.f));
    IntroImage = WidgetTree->ConstructWidget<UImage>();
    VideoFit->SetContent(IntroImage);
    UTextBlock* Skip = MenuText(WidgetTree, NSLOCTEXT("Menu", "SkipIntro", "SPACE  /  SKIP"), 12, FLinearColor(0.4f, 0.4f, 0.4f));
    Skip->SetJustification(ETextJustify::Right);
    UCanvasPanelSlot* SkipSlot = IntroCanvas->AddChildToCanvas(Skip);
    SkipSlot->SetAnchors(FAnchors(1.f, 1.f));
    SkipSlot->SetAlignment(FVector2D(1.f, 1.f));
    SkipSlot->SetPosition(FVector2D(-40.f, -24.f));
    SkipSlot->SetSize(FVector2D(180.f, 24.f));
    IntroScreen->SetVisibility(ESlateVisibility::Collapsed);

    IntroFade = WidgetTree->ConstructWidget<UBorder>();
    IntroFade->SetBrushColor(FLinearColor::Black);
    IntroFade->SetRenderOpacity(0.f);
    UCanvasPanelSlot* FadeSlot = Canvas->AddChildToCanvas(IntroFade);
    FadeSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    FadeSlot->SetOffsets(FMargin(0.f));
    FadeSlot->SetZOrder(110);
    IntroFade->SetVisibility(ESlateVisibility::Collapsed);

}

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Elapsed = 0.f;
}

void UMainMenuWidget::FocusPlayButton()
{
    if (PlayButton) PlayButton->SetKeyboardFocus();
}

void UMainMenuWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (bFadingToIntro)
    {
        IntroFadeElapsed += DeltaTime;
        const float Alpha = FMath::Clamp(IntroFadeElapsed / 0.9f, 0.f, 1.f);
        IntroFade->SetRenderOpacity(Alpha * Alpha * (3.f - 2.f * Alpha));
        if (Alpha >= 1.f)
        {
            bFadingToIntro = false;
            // Present a fully black frame before opening the media player.
            GetWorld()->GetTimerManager().SetTimer(TravelTimer, this,
                &UMainMenuWidget::StartIntro, 0.08f, false);
        }
        return;
    }
    if (bIntroActive && IntroPlayer)
    {
        IntroWait += DeltaTime;
        if (IntroSound) IntroSound->UpdatePlayer();
        const double Time = IntroPlayer->GetTime().GetTotalSeconds();
        // A failed media backend must never trap the player on a black screen.
        if (IntroWait > 10.f && !IntroPlayer->IsPlaying() && Time <= 0.01)
            FinishIntro();
        return;
    }
    Elapsed += DeltaTime;
    const float Fade = FMath::Clamp(Elapsed / 0.65f, 0.f, 1.f);
    ContentPanel->SetRenderOpacity(Fade);
    ContentPanel->SetRenderTranslation(FVector2D((1.f - Fade) * -24.f, 0.f));
    for (UButton* Button : {PlayButton.Get(), QuitButton.Get()})
    {
        const bool bHighlighted = Button->GetIsEnabled() && (Button->IsHovered() || Button->HasKeyboardFocus());
        const float Scale = FMath::FInterpTo(Button->GetRenderTransform().Scale.X,
            bHighlighted ? 1.025f : 1.f, DeltaTime, 12.f);
        Button->SetRenderScale(FVector2D(Scale));
    }
}

int32 UMainMenuWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
    const FWidgetStyle& Style, bool bParentEnabled) const
{
    return Super::NativePaint(Args, Geometry, CullingRect, Elements, Layer, Style, bParentEnabled);
}

int32 UMainMenuBackdropWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
    const FWidgetStyle& Style, bool bParentEnabled) const
{
    const FVector2D Size = Geometry.GetLocalSize();
    FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(),
        FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None,
        FLinearColor(0.009f, 0.014f, 0.025f));
    auto Line = [&](FVector2D A, FVector2D B, FLinearColor Color, float Thickness = 1.f)
    {
        FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(),
            TArray<FVector2D>{A, B}, ESlateDrawEffect::None, Color, true, Thickness);
    };
    constexpr float Grid = 40.f;
    for (int32 Index = 0; Index * Grid < Size.X; ++Index)
    {
        const bool bMajor = Index % 5 == 0;
        Line(FVector2D(Index * Grid, 0.f), FVector2D(Index * Grid, Size.Y),
            FLinearColor(0.18f, 0.3f, 0.4f, bMajor ? 0.24f : 0.09f));
    }
    for (int32 Index = 0; Index * Grid < Size.Y; ++Index)
    {
        const bool bMajor = Index % 5 == 0;
        Line(FVector2D(0.f, Index * Grid), FVector2D(Size.X, Index * Grid),
            FLinearColor(0.18f, 0.3f, 0.4f, bMajor ? 0.24f : 0.09f));
    }
    // Fine registration marks retain the architectural blueprint treatment.
    for (float X = 200.f; X < Size.X; X += 200.f)
        for (float Y = 200.f; Y < Size.Y; Y += 200.f)
        {
            Line(FVector2D(X - 3.f, Y), FVector2D(X + 3.f, Y), FLinearColor(0.55f, 0.64f, 0.7f, 0.25f));
            Line(FVector2D(X, Y - 3.f), FVector2D(X, Y + 3.f), FLinearColor(0.55f, 0.64f, 0.7f, 0.25f));
        }
    const float Time = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
    const float ScanY = FMath::Fmod(Time * 18.f, FMath::Max(1.f, static_cast<float>(Size.Y)));
    Line(FVector2D(0.f, ScanY), FVector2D(Size.X, ScanY), FLinearColor(0.3f, 0.5f, 0.65f, 0.08f));

    // Reserve the title's full width so decorative artwork never overlaps the menu.
    const float ArtLeft = Size.X * 0.08f + 900.f;
    const float ArtRight = Size.X * 0.96f;
    const float ArtWidth = ArtRight - ArtLeft;
    if (ArtWidth > 220.f)
    {
        const FVector2D Center((ArtLeft + ArtRight) * 0.5f, Size.Y * 0.52f);
        const float Radius = FMath::Min(ArtWidth * 0.38f, static_cast<float>(Size.Y) * 0.24f);
        const float Fade = FMath::Clamp(Time / 1.2f, 0.f, 1.f);
        const FLinearColor Structure(0.3f, 0.48f, 0.58f, 0.38f * Fade);
        const FLinearColor Detail(0.3f, 0.48f, 0.58f, 0.2f * Fade);
        const FLinearColor Gold(1.f, 0.76f, 0.22f, 0.52f * Fade);
        auto Point = [&](float X, float Y) { return Center + FVector2D(X, Y) * Radius; };
        auto Rectangle = [&](float X, float Y, float W, float H, FLinearColor Color)
        {
            Line(Point(X,Y), Point(X+W,Y), Color);
            Line(Point(X+W,Y), Point(X+W,Y+H), Color);
            Line(Point(X+W,Y+H), Point(X,Y+H), Color);
            Line(Point(X,Y+H), Point(X,Y), Color);
        };
        auto Arc = [&](float Scale, float Start, float Sweep, FLinearColor Color, float Thickness = 1.f)
        {
            TArray<FVector2D> Points;
            constexpr int32 Segments = 80;
            Points.Reserve(Segments + 1);
            for (int32 Index = 0; Index <= Segments; ++Index)
            {
                const float Angle = Start + Sweep * Index / Segments;
                Points.Add(Point(FMath::Cos(Angle) * Scale, FMath::Sin(Angle) * Scale));
            }
            FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(),
                Points, ESlateDrawEffect::None, Color, true, Thickness);
        };

        Rectangle(-1.16f, -1.2f, 2.32f, 2.4f, Structure);
        Rectangle(-1.04f, -1.08f, 2.08f, 2.16f, Detail);
        for (float Y : {-0.75f, 0.57f})
        {
            Rectangle(-1.22f, Y, 0.19f, 0.18f, Gold);
            Rectangle(1.03f, Y, 0.19f, 0.18f, Structure);
        }
        Arc(0.83f, 0.f, 2.f * PI, Structure);
        Arc(0.7f, 0.f, 2.f * PI, Detail);
        Arc(0.52f, 0.f, 2.f * PI, Gold, 1.5f);
        Arc(0.12f, 0.f, 2.f * PI, Gold, 2.f);
        const float Rotation = Time * 0.12f;
        for (int32 Index = 0; Index < 48; ++Index)
        {
            const float Angle = Rotation + Index * 2.f * PI / 48.f;
            const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
            const float Inner = Index % 4 == 0 ? 0.73f : 0.78f;
            Line(Center + Direction * Radius * Inner, Center + Direction * Radius * 0.83f,
                Index % 4 == 0 ? Gold : Structure);
        }
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const float Angle = -Rotation * 0.65f + Index * 2.f * PI / 3.f;
            const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
            Line(Center + Direction * Radius * 0.12f, Center + Direction * Radius * 0.48f, Gold, 3.f);
        }
        Arc(0.92f, Time * 0.3f, PI * 0.35f, FLinearColor(1.f, 0.76f, 0.22f, 0.2f * Fade), 2.f);
        const float VaultScanY = -1.08f + FMath::Fmod(Time * 0.16f, 2.16f);
        Line(Point(-1.04f, VaultScanY), Point(1.04f, VaultScanY),
            FLinearColor(0.5f, 0.72f, 0.8f, 0.18f * Fade));
    }
    return Super::NativePaint(Args, Geometry, CullingRect, Elements, Layer + 1, Style, bParentEnabled);
}
FReply UMainMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (bIntroActive)
    {
        if (Event.GetKey() == EKeys::SpaceBar && !Event.IsRepeat() && IntroWait > 0.4f)
            FinishIntro();
        return FReply::Handled();
    }
    if (bStartingGame) return FReply::Handled();
    const FKey Key = Event.GetKey();
    if (Key == EKeys::Down || Key == EKeys::Up)
    {
        if (PlayButton->HasKeyboardFocus()) QuitButton->SetKeyboardFocus();
        else PlayButton->SetKeyboardFocus();
        return FReply::Handled();
    }
    if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
    {
        if (!Event.IsRepeat())
        {
            if (QuitButton->HasKeyboardFocus()) QuitClicked();
            else PlayClicked();
        }
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UMainMenuWidget::PlayClicked()
{
    if (bStartingGame) return;
    if (!FPackageName::DoesPackageExist(DemoMap.ToString()))
    {
        Status->SetText(NSLOCTEXT("Menu", "MissingMap", "DemoMap could not be found. Please check the game installation."));
        Status->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.35f, 0.25f)));
        Status->SetVisibility(ESlateVisibility::HitTestInvisible);
        return;
    }
    bStartingGame = true;
    PlayButton->SetIsEnabled(false);
    QuitButton->SetIsEnabled(false);
    IntroFadeElapsed = 0.f;
    bFadingToIntro = true;
    IntroFade->SetRenderOpacity(0.f);
    IntroFade->SetVisibility(ESlateVisibility::Visible);
    SetKeyboardFocus();
    if (APlayerController* PC = GetOwningPlayer()) PC->bShowMouseCursor = false;
}

void UMainMenuWidget::StartIntro()
{
    bIntroActive = true;
    bIntroFinished = false;
    IntroWait = 0.f;
    IntroScreen->SetVisibility(ESlateVisibility::Visible);
    IntroFade->SetVisibility(ESlateVisibility::Collapsed);
    SetKeyboardFocus();
    if (APlayerController* PC = GetOwningPlayer()) PC->bShowMouseCursor = false;
    IntroPlayer = NewObject<UMediaPlayer>(this);
    IntroPlayer->SetDesiredPlayerName(TEXT("WmfMedia"));
    IntroPlayer->PlayOnOpen = true;
    IntroPlayer->SetLooping(false);
    IntroPlayer->OnEndReached.AddDynamic(this, &UMainMenuWidget::FinishIntro);
    IntroPlayer->OnMediaOpenFailed.AddDynamic(this, &UMainMenuWidget::IntroFailed);
    IntroPlayer->OnMediaOpened.AddDynamic(this, &UMainMenuWidget::IntroOpened);
    IntroTexture = NewObject<UMediaTexture>(this);
    IntroTexture->NewStyleOutput = true;
    IntroTexture->AutoClear = true;
    IntroTexture->SetMediaPlayer(IntroPlayer);
    IntroTexture->UpdateResource();
    if (!IntroMaterial) { IntroFailed(TEXT("Missing M_IntroVideo material")); return; }
    IntroDynamicMaterial = UMaterialInstanceDynamic::Create(IntroMaterial, this);
    IntroDynamicMaterial->SetTextureParameterValue(TEXT("Video"), IntroTexture);
    IntroImage->SetBrushFromMaterial(IntroDynamicMaterial);
    IntroImage->SetDesiredSizeOverride(FVector2D(1920.f, 1080.f));
    IntroAudioActor = GetWorld()->SpawnActor<AActor>();
    if (!IntroAudioActor) { FinishIntro(); return; }
    IntroSound = NewObject<UMediaSoundComponent>(IntroAudioActor);
    IntroAudioActor->AddInstanceComponent(IntroSound);
    IntroSound->bIsUISound = true;
    IntroSound->bAllowSpatialization = false;
    IntroSound->SetMediaPlayer(IntroPlayer);
    IntroSound->RegisterComponent();
    IntroSound->Start();
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Movies/OpeningJob.mp4"));
    if (!IntroPlayer->OpenFile(Path)) IntroFailed(Path);
}

void UMainMenuWidget::IntroOpened(FString OpenedUrl)
{
    if (!bIntroActive || !IntroPlayer) return;
    const int32 AudioTracks = IntroPlayer->GetNumTracks(EMediaPlayerTrack::Audio);
    if (AudioTracks > 0) IntroPlayer->SelectTrack(EMediaPlayerTrack::Audio, 0);
    const int32 VideoTracks = IntroPlayer->GetNumTracks(EMediaPlayerTrack::Video);
    if (VideoTracks > 0)
    {
        IntroPlayer->SelectTrack(EMediaPlayerTrack::Video, 0);
        const FIntPoint Dimensions = IntroPlayer->GetVideoTrackDimensions(0, 0);
        if (Dimensions.X > 0 && Dimensions.Y > 0)
            IntroImage->SetDesiredSizeOverride(FVector2D(Dimensions.X, Dimensions.Y));
    }
    if (IntroSound) { IntroSound->UpdatePlayer(); IntroSound->Start(); }
    const bool bPlaying = IntroPlayer->Play();
    UE_LOG(LogTemp, Display, TEXT("Intro opened: audio=%d video=%d play=%d duration=%.3f"),
        AudioTracks, VideoTracks, bPlaying, IntroPlayer->GetDuration().GetTotalSeconds());
}

void UMainMenuWidget::IntroFailed(FString FailedUrl)
{
    UE_LOG(LogTemp, Warning, TEXT("Opening video could not be played: %s"), *FailedUrl);
    FinishIntro();
}

void UMainMenuWidget::StopIntro()
{
    bFadingToIntro = false;
    bIntroActive = false;
    if (IntroSound) { IntroSound->Stop(); IntroSound->SetMediaPlayer(nullptr); }
    if (IntroPlayer)
    {
        IntroPlayer->OnEndReached.RemoveAll(this);
        IntroPlayer->OnMediaOpenFailed.RemoveAll(this);
        IntroPlayer->OnMediaOpened.RemoveAll(this);
        IntroPlayer->Close();
    }
    if (IsValid(IntroAudioActor)) IntroAudioActor->Destroy();
    IntroSound = nullptr;
    IntroAudioActor = nullptr;
    IntroPlayer = nullptr;
    if (IntroTexture) IntroTexture->SetMediaPlayer(nullptr);
    IntroTexture = nullptr;
    if (IntroImage) IntroImage->SetBrushFromMaterial(nullptr);
    IntroDynamicMaterial = nullptr;
}

void UMainMenuWidget::FinishIntro()
{
    if (bIntroFinished) return;
    bIntroFinished = true;
    StopIntro();
    GetWorld()->GetTimerManager().SetTimer(TravelTimer, this, &UMainMenuWidget::OpenDemoMap, 0.15f, false);
}

void UMainMenuWidget::OpenDemoMap()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
    UGameplayStatics::OpenLevel(this, DemoMap, true,
        TEXT("game=/Game/_AGD/BP_ArenaGameMode.BP_ArenaGameMode_C"));
}

void UMainMenuWidget::QuitClicked()
{
    if (!bStartingGame)
        UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UMainMenuWidget::NativeDestruct()
{
    StopIntro();
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TravelTimer);
    Super::NativeDestruct();
}
