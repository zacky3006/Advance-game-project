#include "LootInventoryWidget.h"
#include "LootCollectorComponent.h"
#include "LootUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Materials/MaterialInterface.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/TextureCube.h"
#include "UObject/ConstructorHelpers.h"

ULootInventoryWidget::ULootInventoryWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FObjectFinder<UTextureCube> Environment(
        TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap"));
    PreviewEnvironment = Environment.Object;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
    BackdropMeshAsset = Plane.Object;
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Backdrop(TEXT("/Game/UI/Materials/M_LootThumbnailBackdrop.M_LootThumbnailBackdrop"));
    BackdropMaterial = Backdrop.Object;
}

namespace LootInventoryUI
{
    const FLinearColor Gold = LootUITheme::Gold;
    const FLinearColor Muted = LootUITheme::Muted;

    UTextBlock* Text(UWidgetTree* Tree, const FText& Value, int32 Size = 16, FLinearColor Color = FLinearColor::White)
    {
        UTextBlock* Result = Tree->ConstructWidget<UTextBlock>();
        Result->SetText(Value);
        Result->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Size));
        Result->SetColorAndOpacity(FSlateColor(Color));
        return Result;
    }

    UBorder* Card(UWidgetTree* Tree, FLinearColor Color, float Padding = 16.f)
    {
        UBorder* Result = Tree->ConstructWidget<UBorder>();
        FSlateBrush Brush = LootUITheme::CardBrush();
        Result->SetBrush(Brush);
        Result->SetBrushColor(Color);
        Result->SetPadding(FMargin(Padding));
        return Result;
    }

    void Column(UWidgetTree* Tree, UHorizontalBox* Row, UWidget* Widget, float Width, bool Fill = false)
    {
        USizeBox* Box = Tree->ConstructWidget<USizeBox>();
        if (!Fill) Box->SetWidthOverride(Width);
        Box->SetContent(Widget);
        UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Box);
        Slot->SetVerticalAlignment(VAlign_Center);
        Slot->SetPadding(FMargin(8.f, 0.f));
        if (Fill) Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
}

void ULootInventoryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    using namespace LootInventoryUI;
    SetIsFocusable(true);
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    UBorder* Dim = Card(WidgetTree, FLinearColor(0.005f, 0.008f, 0.016f, 0.84f), 0.f);
    UCanvasPanelSlot* DimSlot = Canvas->AddChildToCanvas(Dim);
    DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    DimSlot->SetOffsets(FMargin(0.f));

    UBorder* Panel = Card(WidgetTree, LootUITheme::Panel, 24.f);
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
    PanelSlot->SetAnchors(FAnchors(0.06f, 0.07f, 0.94f, 0.93f));
    PanelSlot->SetOffsets(FMargin(0.f));
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Panel->SetContent(Content);

    UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
    Content->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
    UVerticalBox* Titles = WidgetTree->ConstructWidget<UVerticalBox>();
    Titles->AddChildToVerticalBox(Text(WidgetTree, NSLOCTEXT("Loot", "InventoryTitle", "YOUR HAUL"), 30, Gold));
    Titles->AddChildToVerticalBox(Text(WidgetTree, NSLOCTEXT("Loot", "InventorySubtitle", "Every valuable. Every successful steal."), 13, Muted));
    Header->AddChildToHorizontalBox(Titles)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UButton* Close = WidgetTree->ConstructWidget<UButton>();
    Close->SetBackgroundColor(FLinearColor(0.12f, 0.16f, 0.23f));
    Close->SetContent(Text(WidgetTree, NSLOCTEXT("Loot", "CloseBag", "  CLOSE  [TAB]  "), 13));
    Close->OnClicked.AddDynamic(this, &ULootInventoryWidget::CloseClicked);
    Header->AddChildToHorizontalBox(Close)->SetVerticalAlignment(VAlign_Center);

    UHorizontalBox* Stats = WidgetTree->ConstructWidget<UHorizontalBox>();
    Content->AddChildToVerticalBox(Stats)->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
    UBorder* CountCard = Card(WidgetTree, FLinearColor(0.045f, 0.065f, 0.095f));
    CountText = Text(WidgetTree, FText::GetEmpty(), 24);
    CountCard->SetContent(CountText);
    UHorizontalBoxSlot* CountSlot = Stats->AddChildToHorizontalBox(CountCard);
    CountSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CountSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    UBorder* ValueCard = Card(WidgetTree, FLinearColor(0.055f, 0.085f, 0.065f));
    ValueText = Text(WidgetTree, FText::GetEmpty(), 24, LootUITheme::Success);
    ValueCard->SetContent(ValueText);
    Stats->AddChildToHorizontalBox(ValueCard)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* Sort = WidgetTree->ConstructWidget<UButton>();
    Sort->SetBackgroundColor(FLinearColor(0.07f, 0.095f, 0.14f));
    SortText = Text(WidgetTree, FText::GetEmpty(), 12, Gold);
    Sort->SetContent(SortText);
    Sort->OnClicked.AddDynamic(this, &ULootInventoryWidget::SortClicked);
    UVerticalBoxSlot* SortSlot = Content->AddChildToVerticalBox(Sort);
    SortSlot->SetHorizontalAlignment(HAlign_Right);
    SortSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

    UHorizontalBox* Labels = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column(WidgetTree, Labels, Text(WidgetTree, FText::GetEmpty()), 64.f);
    Column(WidgetTree, Labels, Text(WidgetTree, NSLOCTEXT("Loot", "ItemColumn", "ITEM"), 12, Muted), 0.f, true);
    Column(WidgetTree, Labels, Text(WidgetTree, NSLOCTEXT("Loot", "UnitColumn", "UNIT VALUE"), 12, Muted), 110.f);
    Column(WidgetTree, Labels, Text(WidgetTree, NSLOCTEXT("Loot", "QtyColumn", "QTY"), 12, Muted), 50.f);
    Column(WidgetTree, Labels, Text(WidgetTree, NSLOCTEXT("Loot", "SubtotalColumn", "SUBTOTAL"), 12, Muted), 130.f);
    Content->AddChildToVerticalBox(Labels)->SetPadding(FMargin(12.f, 0.f, 12.f, 8.f));

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Scroll->SetAnimateWheelScrolling(true);
    Scroll->SetScrollbarPadding(FMargin(8.f, 0.f));
    Content->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Rows = WidgetTree->ConstructWidget<UVerticalBox>();
    Scroll->AddChild(Rows);
    Content->AddChildToVerticalBox(Text(WidgetTree,
        NSLOCTEXT("Loot", "InventoryFooter", "MOUSE WHEEL  Scroll     |     TAB / ESC  Return to game"), 12, Muted))
        ->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
}

void ULootInventoryWidget::RefreshInventory()
{
    if (!Rows || !Collector) return;
    using namespace LootInventoryUI;
    Rows->ClearChildren();
    CountText->SetText(FText::Format(NSLOCTEXT("Loot", "BagCount", "Loot: {0} items"), FText::AsNumber(Collector->Inventory.Num())));
    ValueText->SetText(FText::Format(NSLOCTEXT("Loot", "BagTotal", "Total: ${0}"), FText::AsNumber(Collector->GetTotalValue())));
    SortText->SetText(bSortByValue ? NSLOCTEXT("Loot", "SortValue", "  SORT: HIGHEST VALUE  v  ")
        : NSLOCTEXT("Loot", "SortName", "  SORT: NAME A-Z  v  "));

    struct FGroup { FLootEntry Entry; int32 Quantity = 1; };
    TArray<FGroup> Groups;
    for (const FLootEntry& Entry : Collector->Inventory)
    {
        FGroup* Existing = Groups.FindByPredicate([&Entry](const FGroup& Group)
        {
            return Group.Entry.ItemName.EqualTo(Entry.ItemName) && Group.Entry.Value == Entry.Value
                && Group.Entry.PreviewMesh == Entry.PreviewMesh
                && Group.Entry.PreviewMaterials == Entry.PreviewMaterials;
        });
        if (Existing) ++Existing->Quantity;
        else { FGroup Group; Group.Entry = Entry; Groups.Add(MoveTemp(Group)); }
    }
    Groups.Sort([this](const FGroup& A, const FGroup& B)
    {
        const int64 ATotal = static_cast<int64>(A.Entry.Value) * A.Quantity;
        const int64 BTotal = static_cast<int64>(B.Entry.Value) * B.Quantity;
        if (bSortByValue && ATotal != BTotal) return ATotal > BTotal;
        return A.Entry.ItemName.ToString() < B.Entry.ItemName.ToString();
    });
    if (Groups.IsEmpty())
    {
        UBorder* Empty = Card(WidgetTree, FLinearColor(0.035f, 0.045f, 0.065f), 40.f);
        UTextBlock* Label = Text(WidgetTree, NSLOCTEXT("Loot", "EmptyBag", "YOUR BAG IS EMPTY\n\nFind a valuable and press F to steal it."), 18, Muted);
        Label->SetJustification(ETextJustify::Center);
        Empty->SetContent(Label);
        Rows->AddChildToVerticalBox(Empty);
    }
    for (const FGroup& Group : Groups)
    {
        UBorder* RowCard = Card(WidgetTree, LootUITheme::Surface, 12.f);
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        RowCard->SetContent(Row);
        USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
        IconBox->SetWidthOverride(64.f);
        IconBox->SetHeightOverride(64.f);
        UImage* Icon = WidgetTree->ConstructWidget<UImage>();
        if (UTexture* Thumbnail = MakeThumbnail(Group.Entry))
        {
            FSlateBrush Brush;
            Brush.SetResourceObject(Thumbnail);
            Brush.ImageSize = FVector2D(64.f);
            Brush.DrawAs = ESlateBrushDrawType::Image;
            Icon->SetBrush(Brush);
        }
        else Icon->SetColorAndOpacity(Gold);
        IconBox->SetContent(Icon);
        Column(WidgetTree, Row, IconBox, 64.f);
        UTextBlock* Name = Text(WidgetTree, Group.Entry.ItemName, 18);
        Name->SetAutoWrapText(true);
        Column(WidgetTree, Row, Name, 0.f, true);
        Column(WidgetTree, Row, Text(WidgetTree, FText::Format(NSLOCTEXT("Loot", "Currency", "${0}"), FText::AsNumber(Group.Entry.Value)), 15, Muted), 110.f);
        Column(WidgetTree, Row, Text(WidgetTree, FText::Format(NSLOCTEXT("Loot", "Quantity", "x{0}"), FText::AsNumber(Group.Quantity)), 17, Gold), 50.f);
        Column(WidgetTree, Row, Text(WidgetTree, FText::Format(NSLOCTEXT("Loot", "Currency", "${0}"),
            FText::AsNumber(static_cast<int64>(Group.Entry.Value) * Group.Quantity)), 18, LootUITheme::Success), 130.f);
        Rows->AddChildToVerticalBox(RowCard)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }
}

UTexture* ULootInventoryWidget::MakeThumbnail(const FLootEntry& Entry)
{
    // Always show the actual collected mesh and materials, even if an ItemIcon was assigned.
    if (!Entry.PreviewMesh || !GetWorld()) return Entry.Icon;
    FString CacheKey = Entry.PreviewMesh->GetPathName();
    for (const UMaterialInterface* Material : Entry.PreviewMaterials)
        CacheKey += TEXT("|") + GetPathNameSafe(Material);
    if (const TObjectPtr<UTexture>* Cached = ThumbnailCache.Find(CacheKey)) return Cached->Get();
    if (!PreviewActor.IsValid())
    {
        // Render in a separate world: house fog, atmosphere, exposure, and lights
        // must never leak into the inventory portrait.
        ThumbnailScene = MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues()
            .SetEditor(false).SetCreatePhysicsScene(false).SetTransactional(false)
            .SetLightRotation(FRotator(-40.f, 135.f, 0.f))
            .SetLightBrightness(5.f).SetSkyBrightness(0.6f));
        if (PreviewEnvironment) ThumbnailScene->SetSkyCubemap(PreviewEnvironment);
        UDirectionalLightComponent* Fill = NewObject<UDirectionalLightComponent>(this);
        Fill->SetIntensity(2.f);
        Fill->SetLightColor(FLinearColor::White);
        Fill->SetCastShadows(false);
        ThumbnailScene->AddComponent(Fill, FTransform(FRotator(-25.f, -35.f, 0.f)));
        ThumbnailScene->UpdateCaptureContents();
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        AActor* Actor = ThumbnailScene->GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (!Actor) return nullptr;
        PreviewActor = Actor;
        Actor->SetReplicates(false);
        PreviewMesh = NewObject<UStaticMeshComponent>(Actor);
        PreviewMesh->SetMobility(EComponentMobility::Movable);
        PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PreviewMesh->SetCanEverAffectNavigation(false);
        PreviewMesh->SetCastShadow(false);
        PreviewMesh->SetVisibleInSceneCaptureOnly(true);
        PreviewMesh->SetAffectDynamicIndirectLighting(false);
        Actor->SetRootComponent(PreviewMesh);
        ThumbnailScene->AddComponent(PreviewMesh, FTransform::Identity);
        PreviewCapture = NewObject<USceneCaptureComponent2D>(Actor);
        PreviewCapture->bCaptureEveryFrame = false;
        PreviewCapture->bCaptureOnMovement = false;
        // Fixed studio lighting and exposure preserve material color and shape.
        PreviewCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
        PreviewCapture->ShowFlags.SetLighting(true);
        PreviewCapture->ShowFlags.SetFog(false);
        PreviewCapture->ShowFlags.SetVolumetricFog(false);
        PreviewCapture->ShowFlags.SetAtmosphere(false);
        PreviewCapture->ShowFlags.SetCloud(false);
        PreviewCapture->ShowFlags.SetAmbientOcclusion(false);
        PreviewCapture->ShowFlags.SetAntiAliasing(false);
        PreviewCapture->ShowFlags.SetPostProcessing(true);
        PreviewCapture->UnlitViewmode = ESceneCaptureUnlitViewmode::Disabled;
        FPostProcessSettings& Settings = PreviewCapture->PostProcessSettings;
        Settings.bOverride_AutoExposureMethod = true;
        Settings.AutoExposureMethod = AEM_Manual;
        Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
        Settings.AutoExposureApplyPhysicalCameraExposure = false;
        Settings.bOverride_AutoExposureBias = true;
        Settings.AutoExposureBias = 0.f;
        Settings.bOverride_VignetteIntensity = true;
        Settings.VignetteIntensity = 0.f;
        Settings.bOverride_BloomIntensity = true;
        Settings.BloomIntensity = 0.f;
        Settings.bOverride_DynamicGlobalIlluminationMethod = true;
        Settings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
        Settings.bOverride_ReflectionMethod = true;
        Settings.ReflectionMethod = EReflectionMethod::None;
        PreviewCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
        PreviewCapture->FOVAngle = 35.f;
        PreviewCapture->ShowOnlyComponent(PreviewMesh);
        ThumbnailScene->AddComponent(PreviewCapture, FTransform::Identity);
        PreviewBackdrop = NewObject<UStaticMeshComponent>(Actor);
        PreviewBackdrop->SetMobility(EComponentMobility::Movable);
        PreviewBackdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PreviewBackdrop->SetCastShadow(false);
        PreviewBackdrop->SetStaticMesh(BackdropMeshAsset);
        PreviewBackdrop->SetMaterial(0, BackdropMaterial);
        ThumbnailScene->AddComponent(PreviewBackdrop, FTransform::Identity);
        PreviewCapture->ShowOnlyComponent(PreviewBackdrop);
    }
    PreviewMesh->SetStaticMesh(Entry.PreviewMesh);
    PreviewMesh->EmptyOverrideMaterials();
    for (int32 Index = 0; Index < Entry.PreviewMaterials.Num(); ++Index)
        PreviewMesh->SetMaterial(Index, Entry.PreviewMaterials[Index]);
    PreviewMesh->UpdateBounds();
    const FVector Center = PreviewMesh->Bounds.Origin;
    const float Radius = FMath::Max(PreviewMesh->Bounds.SphereRadius, 1.f);
    const float Distance = Radius / FMath::Sin(FMath::DegreesToRadians(17.5f)) * 1.15f;
    const FVector CameraLocation = Center + FVector(1.f, -1.f, 0.65f).GetSafeNormal() * Distance;
    const FVector CameraDirection = (CameraLocation - Center).GetSafeNormal();
    PreviewBackdrop->SetWorldTransform(FTransform(FRotationMatrix::MakeFromZ(CameraDirection).ToQuat(),
        Center - CameraDirection * Radius * 2.f, FVector(Radius * 0.08f)));
    PreviewCapture->SetWorldLocationAndRotation(CameraLocation, (Center - CameraLocation).Rotation());
    UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(this);
    Target->ClearColor = LootUITheme::Surface;
    Target->InitCustomFormat(256, 256, PF_B8G8R8A8, false);
    Target->TargetGamma = 2.2f;
    ThumbnailCache.Add(CacheKey, Target);
    PreviewCapture->TextureTarget = Target;
    PreviewCapture->CaptureScene();
    return Target;
}

void ULootInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    OpenTime = 0.f;
}

void ULootInventoryWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    OpenTime += DeltaTime;
    const float Progress = FMath::Clamp(OpenTime / 0.18f, 0.f, 1.f);
    SetRenderOpacity(Progress);
    SetRenderTranslation(FVector2D(0.f, (1.f - Progress) * 12.f));
}

FReply ULootInventoryWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
    if (KeyEvent.GetKey() == EKeys::Tab || KeyEvent.GetKey() == EKeys::Escape)
    {
        if (!KeyEvent.IsRepeat()) CloseClicked();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

void ULootInventoryWidget::CloseClicked()
{
    if (Collector) Collector->CloseInventory();
}

void ULootInventoryWidget::SortClicked()
{
    bSortByValue = !bSortByValue;
    RefreshInventory();
}

void ULootInventoryWidget::ReleasePreviews()
{
    if (ThumbnailScene)
    {
        if (PreviewCapture) ThumbnailScene->RemoveComponent(PreviewCapture);
        if (PreviewMesh) ThumbnailScene->RemoveComponent(PreviewMesh);
        if (PreviewBackdrop) ThumbnailScene->RemoveComponent(PreviewBackdrop);
    }
    if (AActor* Actor = PreviewActor.Get()) Actor->Destroy();
    PreviewActor.Reset();
    PreviewCapture = nullptr;
    PreviewMesh = nullptr;
    PreviewBackdrop = nullptr;
    ThumbnailScene.Reset();
    ThumbnailCache.Reset();
}
