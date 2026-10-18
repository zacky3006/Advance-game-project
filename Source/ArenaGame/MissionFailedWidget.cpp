#include "MissionFailedWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UMissionFailedWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrush(*FCoreStyle::Get().GetBrush("WhiteBrush"));
    Background->SetBrushColor(FLinearColor::Black);
    auto* BackgroundSlot = Canvas->AddChildToCanvas(Background);
    BackgroundSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    BackgroundSlot->SetOffsets(FMargin(0.f));
    Message = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MissionFailedText"));
    Message->SetText(NSLOCTEXT("Mission", "Failed", "MISSION FAILED"));
    Message->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 64));
    Message->SetColorAndOpacity(FSlateColor(FLinearColor(.85f, .015f, .025f, 1.f)));
    Message->SetJustification(ETextJustify::Center);
    Message->SetRenderOpacity(0.f);
    auto* TextSlot = Canvas->AddChildToCanvas(Message);
    TextSlot->SetAnchors(FAnchors(.5f, .5f));
    TextSlot->SetAlignment(FVector2D(.5f, .5f));
    TextSlot->SetAutoSize(true);
    SetRenderOpacity(0.f);
}

void UMissionFailedWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    Elapsed += DeltaTime;
    SetRenderOpacity(FMath::SmoothStep(0.f, .7f, Elapsed));
    if (Message) Message->SetRenderOpacity(FMath::SmoothStep(.7f, 1.9f, Elapsed));
}
