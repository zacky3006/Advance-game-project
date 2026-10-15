#include "PlayerAlertHUD.h"

#include "AIAlertIndicatorWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Actor.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Math/RotationMatrix.h"

// ============================================================
// Find Indicator Index
// ============================================================

int32 UPlayerAlertHUD::FindIndicatorIndex(AActor* AIActor) const
{
    if (!IsValid(AIActor))
    {
        return INDEX_NONE;
    }

    for (int32 i = 0; i < Indicators.Num(); ++i)
    {
        if (Indicators[i].AIActor == AIActor)
        {
            return i;
        }
    }

    return INDEX_NONE;
}

// ============================================================
// Add Or Update Indicator
// ============================================================

void UPlayerAlertHUD::AddOrUpdateIndicator(
    AActor* AIActor,
    float AlertPercent)
{
    if (!IsValid(AIActor) ||
        !IsValid(AlertCanvas) ||
        !IndicatorWidgetClass)
    {
        return;
    }

    const int32 Index = FindIndicatorIndex(AIActor);

    UAIAlertIndicatorWidget* Indicator = nullptr;

    if (Index == INDEX_NONE)
    {
        Indicator = CreateWidget<UAIAlertIndicatorWidget>(
            GetOwningPlayer(),
            IndicatorWidgetClass
        );

        if (!Indicator)
        {
            return;
        }

        UCanvasPanelSlot* CanvasSlot =
            AlertCanvas->AddChildToCanvas(Indicator);

        if (CanvasSlot)
        {
            CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            CanvasSlot->SetAutoSize(true);
        }

        FAIAlertIndicatorEntry NewEntry;
        NewEntry.AIActor = AIActor;
        NewEntry.IndicatorWidget = Indicator;

        Indicators.Add(NewEntry);
    }
    else
    {
        Indicator = Indicators[Index].IndicatorWidget;
    }

    if (IsValid(Indicator))
    {
        Indicator->SetAlertPercent(AlertPercent);
        UpdateIndicatorPosition(AIActor, Indicator);
    }
}

// ============================================================
// Remove Indicator
// ============================================================

void UPlayerAlertHUD::RemoveIndicator(AActor* AIActor)
{
    const int32 Index = FindIndicatorIndex(AIActor);

    if (Index == INDEX_NONE)
    {
        return;
    }

    if (IsValid(Indicators[Index].IndicatorWidget))
    {
        Indicators[Index].IndicatorWidget->RemoveFromParent();
    }

    Indicators.RemoveAt(Index);
}

// ============================================================
// Clear All Indicators
// ============================================================

void UPlayerAlertHUD::ClearIndicators()
{
    for (FAIAlertIndicatorEntry& Entry : Indicators)
    {
        if (IsValid(Entry.IndicatorWidget))
        {
            Entry.IndicatorWidget->RemoveFromParent();
        }
    }

    Indicators.Empty();
}

// ============================================================
// Native Tick
// ============================================================

void UPlayerAlertHUD::NativeTick(
    const FGeometry& MyGeometry,
    float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    for (int32 i = Indicators.Num() - 1; i >= 0; --i)
    {
        AActor* AIActor = Indicators[i].AIActor;
        UAIAlertIndicatorWidget* Indicator =
            Indicators[i].IndicatorWidget;

        if (!IsValid(AIActor) || !IsValid(Indicator))
        {
            if (IsValid(Indicator))
            {
                Indicator->RemoveFromParent();
            }

            Indicators.RemoveAt(i);
            continue;
        }

        UpdateIndicatorPosition(AIActor, Indicator);
    }
}

// ============================================================
// Update Indicator Position
// ============================================================

void UPlayerAlertHUD::UpdateIndicatorPosition(
    AActor* AIActor,
    UAIAlertIndicatorWidget* IndicatorWidget)
{
    if (!IsValid(AIActor) ||
        !IsValid(IndicatorWidget) ||
        !IsValid(AlertCanvas))
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();

    if (!IsValid(PC) ||
        !IsValid(PC->PlayerCameraManager))
    {
        return;
    }

    // --------------------------------------------------------
    // Get viewport dimensions and DPI scale
    // --------------------------------------------------------

    const FVector2D ViewportSize =
        UWidgetLayoutLibrary::GetViewportSize(this);

    const float ViewportScale =
        UWidgetLayoutLibrary::GetViewportScale(this);

    if (ViewportSize.X <= 0.f ||
        ViewportSize.Y <= 0.f ||
        ViewportScale <= 0.f)
    {
        return;
    }

    // Convert physical viewport pixels to logical UI coordinates.
    const FVector2D CanvasSize =
        ViewportSize / ViewportScale;

    const FVector2D Center = CanvasSize * 0.5f;

    // Margin from viewport edges, in logical UI units.
    const float Margin = 60.f;

    const float MaxX =
        FMath::Max(1.f, Center.X - Margin);

    const float MaxY =
        FMath::Max(1.f, Center.Y - Margin);

    // --------------------------------------------------------
    // Get AI and camera positions
    // --------------------------------------------------------

    const FVector TargetLocation =
        AIActor->GetActorLocation();

    const FVector CameraLocation =
        PC->PlayerCameraManager->GetCameraLocation();

    const FRotator CameraRotation =
        PC->PlayerCameraManager->GetCameraRotation();

    const FVector ToTarget =
        TargetLocation - CameraLocation;

    // Convert target direction into camera-local coordinates.
    const FVector LocalDirection =
        CameraRotation.UnrotateVector(ToTarget);

    // Unreal camera-local X is forward.
    const bool bBehindCamera =
        LocalDirection.X <= 0.f;

    // --------------------------------------------------------
    // Project AI world position onto the screen
    // --------------------------------------------------------

    FVector2D ScreenPosition = FVector2D::ZeroVector;

    const bool bProjected =
        PC->ProjectWorldLocationToScreen(
            TargetLocation,
            ScreenPosition,
            true
        );

    // Convert screen pixels to logical UI coordinates.
    ScreenPosition /= ViewportScale;

    // --------------------------------------------------------
    // Calculate direction to AI
    // --------------------------------------------------------

    FVector2D Direction = FVector2D::ZeroVector;

    if (bProjected && !bBehindCamera)
    {
        // AI is in front of the camera.
        Direction = ScreenPosition - Center;
    }
    else
    {
        // AI is behind the camera or cannot be projected.
        Direction.X = LocalDirection.Y;
        Direction.Y = -LocalDirection.Z;

        if (bBehindCamera)
        {
            Direction *= -1.f;
        }
    }

    if (Direction.IsNearlyZero())
    {
        Direction = FVector2D(1.f, 0.f);
    }

    Direction.Normalize();

    // --------------------------------------------------------
    // Determine whether AI is within the visible screen area
    // --------------------------------------------------------

    const bool bOnScreen =
        bProjected &&
        !bBehindCamera &&
        ScreenPosition.X >= Margin &&
        ScreenPosition.X <= CanvasSize.X - Margin &&
        ScreenPosition.Y >= Margin &&
        ScreenPosition.Y <= CanvasSize.Y - Margin;

    FVector2D FinalPosition = FVector2D::ZeroVector;

    if (bOnScreen)
    {
        // AI is visible: place the indicator at its projected position.
        FinalPosition = ScreenPosition;
    }
    else
    {
        // AI is outside the visible screen area:
        // intersect the direction ray with the viewport boundary.

        const float ScaleX =
            FMath::Abs(Direction.X) > KINDA_SMALL_NUMBER
            ? MaxX / FMath::Abs(Direction.X)
            : BIG_NUMBER;

        const float ScaleY =
            FMath::Abs(Direction.Y) > KINDA_SMALL_NUMBER
            ? MaxY / FMath::Abs(Direction.Y)
            : BIG_NUMBER;

        const float EdgeScale =
            FMath::Min(ScaleX, ScaleY);

        FinalPosition =
            Center + Direction * EdgeScale;
    }

    // --------------------------------------------------------
    // Update Canvas position
    // --------------------------------------------------------

    if (UCanvasPanelSlot* CanvasSlot =
        Cast<UCanvasPanelSlot>(IndicatorWidget->Slot))
    {
        CanvasSlot->SetPosition(FinalPosition);
        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    }

    // --------------------------------------------------------
    // Rotate indicator toward AI
    // Assumes the arrow graphic points right by default.
    // --------------------------------------------------------

    const float AngleDegrees =
        FMath::RadiansToDegrees(
            FMath::Atan2(Direction.Y, Direction.X)
        );

    IndicatorWidget->SetRenderTransformAngle(AngleDegrees);
}