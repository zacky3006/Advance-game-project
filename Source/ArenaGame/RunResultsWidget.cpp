#include "RunResultsWidget.h"
#include "LootCollectorComponent.h"
#include "LootUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

namespace RunResultsUI
{
    UTextBlock* Text(UWidgetTree* Tree, const FText& Content, int32 Size, FLinearColor Color = FLinearColor::White)
    {
        UTextBlock* Label = Tree->ConstructWidget<UTextBlock>();
        Label->SetText(Content);
        Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Size));
        Label->SetColorAndOpacity(FSlateColor(Color));
        return Label;
    }
    UBorder* Card(UWidgetTree* Tree, FLinearColor Color, float Padding)
    {
        UBorder* Box = Tree->ConstructWidget<UBorder>();
        Box->SetBrush(LootUITheme::CardBrush());
        Box->SetBrushColor(Color);
        Box->SetPadding(FMargin(Padding));
        return Box;
    }
    void Column(UWidgetTree* Tree, UHorizontalBox* Row, UTextBlock* Label, float Width)
    {
        USizeBox* Box = Tree->ConstructWidget<USizeBox>();
        if (Width > 0.f)
        {
            Box->SetWidthOverride(Width);
            Label->SetJustification(ETextJustify::Right);
        }
        else Label->SetAutoWrapText(true);
        Box->SetContent(Label);
        UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Box);
        Slot->SetPadding(FMargin(12.f, 0.f));
        Slot->SetVerticalAlignment(VAlign_Center);
        if (Width <= 0.f) Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
}

void URunResultsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    using namespace RunResultsUI;
    SetIsFocusable(true);
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    UBorder* Background = Card(WidgetTree, FLinearColor(0.008f, 0.012f, 0.022f, 1.f), 0.f);
    UCanvasPanelSlot* BackgroundSlot = Canvas->AddChildToCanvas(Background);
    BackgroundSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    BackgroundSlot->SetOffsets(FMargin(0.f));
    UBorder* Panel = Card(WidgetTree, LootUITheme::Panel, 28.f);
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
    PanelSlot->SetAnchors(FAnchors(0.06f, 0.06f, 0.94f, 0.94f));
    PanelSlot->SetOffsets(FMargin(0.f));
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Panel->SetContent(Content);
    Content->AddChildToVerticalBox(Text(WidgetTree, NSLOCTEXT("Results", "Complete", "ESCAPE SUCCESSFUL"), 13, LootUITheme::Success));
    Content->AddChildToVerticalBox(Text(WidgetTree, NSLOCTEXT("Results", "Title", "RUN SUMMARY"), 34, LootUITheme::Gold))
        ->SetPadding(FMargin(0.f, 6.f, 0.f, 22.f));

    UHorizontalBox* Stats = WidgetTree->ConstructWidget<UHorizontalBox>();
    Content->AddChildToVerticalBox(Stats)->SetPadding(FMargin(0.f, 0.f, 0.f, 26.f));
    auto Stat = [&](const FText& Label, FLinearColor Color, UTextBlock*& Output)
    {
        UBorder* Box = Card(WidgetTree, LootUITheme::Surface, 18.f);
        UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
        Box->SetContent(Stack);
        Stack->AddChildToVerticalBox(Text(WidgetTree, Label, 12, LootUITheme::Muted));
        Output = Text(WidgetTree, FText::GetEmpty(), 28, Color);
        Stack->AddChildToVerticalBox(Output)->SetPadding(FMargin(0.f, 9.f, 0.f, 0.f));
        UHorizontalBoxSlot* Slot = Stats->AddChildToHorizontalBox(Box);
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Slot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
    };
    UTextBlock* CountLabel = nullptr;
    UTextBlock* ValueLabel = nullptr;
    UTextBlock* DurationLabel = nullptr;
    Stat(NSLOCTEXT("Results", "CountLabel", "TOTAL ITEMS"), FLinearColor::White, CountLabel);
    Stat(NSLOCTEXT("Results", "ValueLabel", "TOTAL VALUE"), LootUITheme::Success, ValueLabel);
    Stat(NSLOCTEXT("Results", "TimeLabel", "TIME ELAPSED"), LootUITheme::Gold, DurationLabel);
    Count = CountLabel;
    Value = ValueLabel;
    Duration = DurationLabel;

    UHorizontalBox* Labels = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column(WidgetTree, Labels, Text(WidgetTree, NSLOCTEXT("Results", "Item", "ITEM"), 12, LootUITheme::Muted), 0.f);
    Column(WidgetTree, Labels, Text(WidgetTree, NSLOCTEXT("Results", "Unit", "UNIT VALUE"), 12, LootUITheme::Muted), 140.f);
    Column(WidgetTree, Labels, Text(WidgetTree, NSLOCTEXT("Results", "Qty", "QTY"), 12, LootUITheme::Muted), 75.f);
    Column(WidgetTree, Labels, Text(WidgetTree, NSLOCTEXT("Results", "Subtotal", "SUBTOTAL"), 12, LootUITheme::Muted), 160.f);
    Content->AddChildToVerticalBox(Labels)->SetPadding(FMargin(12.f, 0.f, 12.f, 12.f));
    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Scroll->SetAnimateWheelScrolling(true);
    Scroll->SetScrollbarPadding(FMargin(8.f, 0.f));
    Content->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Rows = WidgetTree->ConstructWidget<UVerticalBox>();
    Scroll->AddChild(Rows);

    FinishButton = WidgetTree->ConstructWidget<UButton>();
    FSlateBrush Normal = LootUITheme::CardBrush();
    Normal.TintColor = FSlateColor(LootUITheme::Gold);
    FSlateBrush Hover = Normal;
    Hover.TintColor = FSlateColor(FLinearColor(1.f, 0.9f, 0.52f));
    FSlateBrush Pressed = Normal;
    Pressed.TintColor = FSlateColor(FLinearColor(0.8f, 0.55f, 0.12f));
    FButtonStyle Style;
    Style.SetNormal(Normal).SetHovered(Hover).SetPressed(Pressed).SetDisabled(Normal);
    FinishButton->SetStyle(Style);
    FinishButton->SetContent(Text(WidgetTree, NSLOCTEXT("Results", "Finish", "FINISH"), 20, LootUITheme::Panel));
    FinishButton->OnClicked.AddDynamic(this, &URunResultsWidget::FinishClicked);
    USizeBox* FinishSize = WidgetTree->ConstructWidget<USizeBox>();
    FinishSize->SetWidthOverride(240.f);
    FinishSize->SetHeightOverride(64.f);
    FinishSize->SetContent(FinishButton);
    UVerticalBoxSlot* FooterSlot = Content->AddChildToVerticalBox(FinishSize);
    FooterSlot->SetHorizontalAlignment(HAlign_Right);
    FooterSlot->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
}

void URunResultsWidget::Populate(const TArray<FLootEntry>& Entries, double RunSeconds)
{
    using namespace RunResultsUI;
    Rows->ClearChildren();
    struct FGroup { FLootEntry Item; int32 Quantity = 1; };
    TArray<FGroup> Groups;
    int64 Total = 0;
    for (const FLootEntry& Entry : Entries)
    {
        Total += Entry.Value;
        FGroup* Existing = Groups.FindByPredicate([&](const FGroup& Group)
        {
            return Group.Item.ItemName.EqualTo(Entry.ItemName) && Group.Item.Value == Entry.Value
                && Group.Item.PreviewMesh == Entry.PreviewMesh && Group.Item.PreviewMaterials == Entry.PreviewMaterials;
        });
        if (Existing) ++Existing->Quantity;
        else { FGroup Group; Group.Item = Entry; Groups.Add(MoveTemp(Group)); }
    }
    Groups.Sort([](const FGroup& A, const FGroup& B)
    {
        const int64 AValue = static_cast<int64>(A.Item.Value) * A.Quantity;
        const int64 BValue = static_cast<int64>(B.Item.Value) * B.Quantity;
        if (AValue != BValue) return AValue > BValue;
        return A.Item.ItemName.ToString() < B.Item.ItemName.ToString();
    });
    Count->SetText(FText::AsNumber(Entries.Num()));
    Value->SetText(FText::Format(NSLOCTEXT("Results", "Money", "${0}"), FText::AsNumber(Total)));
    const int64 Seconds = FMath::Max<int64>(0, static_cast<int64>(RunSeconds));
    Duration->SetText(FText::FromString(Seconds >= 3600
        ? FString::Printf(TEXT("%02lld:%02lld:%02lld"), Seconds / 3600, (Seconds / 60) % 60, Seconds % 60)
        : FString::Printf(TEXT("%02lld:%02lld"), Seconds / 60, Seconds % 60)));
    for (const FGroup& Group : Groups)
    {
        UBorder* Box = Card(WidgetTree, LootUITheme::Surface, 12.f);
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Box->SetContent(Row);
        Column(WidgetTree, Row, Text(WidgetTree, Group.Item.ItemName, 17), 0.f);
        Column(WidgetTree, Row, Text(WidgetTree, FText::Format(NSLOCTEXT("Results", "Money", "${0}"), FText::AsNumber(Group.Item.Value)), 16, LootUITheme::Muted), 140.f);
        Column(WidgetTree, Row, Text(WidgetTree, FText::AsNumber(Group.Quantity), 17), 75.f);
        Column(WidgetTree, Row, Text(WidgetTree, FText::Format(NSLOCTEXT("Results", "Money", "${0}"),
            FText::AsNumber(static_cast<int64>(Group.Item.Value) * Group.Quantity)), 18, LootUITheme::Success), 160.f);
        Rows->AddChildToVerticalBox(Box)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }
    if (Groups.IsEmpty())
        Rows->AddChildToVerticalBox(Text(WidgetTree, NSLOCTEXT("Results", "Empty", "No valuables collected."), 18, LootUITheme::Muted));
}

void URunResultsWidget::FocusFinish()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->bShowMouseCursor = true;
        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(FinishButton->TakeWidget());
        PC->SetInputMode(Mode);
        FinishButton->SetKeyboardFocus();
    }
}

void URunResultsWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    IntroElapsed += DeltaTime;
    SetRenderOpacity(FMath::Clamp(IntroElapsed / 0.35f, 0.f, 1.f));
}

void URunResultsWidget::FinishClicked()
{
    if (bLeaving) return;
    bLeaving = true;
    FinishButton->SetIsEnabled(false);
    UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/MainMenu"), true);
}
