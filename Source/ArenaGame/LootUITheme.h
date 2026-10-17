#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

// Shared palette and card treatment for the haul menu, HUD, and interaction prompt.
namespace LootUITheme
{
    inline const FLinearColor Gold(1.f, 0.76f, 0.22f);
    inline const FLinearColor Muted(0.52f, 0.61f, 0.73f);
    inline const FLinearColor Panel(0.018f, 0.025f, 0.04f, 0.99f);
    inline const FLinearColor Surface(0.035f, 0.048f, 0.07f);
    inline const FLinearColor Success(0.25f, 1.f, 0.55f);

    inline FSlateBrush CardBrush()
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.TintColor = FSlateColor(FLinearColor::White);
        Brush.OutlineSettings.CornerRadii = FVector4(10.f);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Brush.OutlineSettings.Width = 1.f;
        Brush.OutlineSettings.Color = FSlateColor(FLinearColor(0.25f, 0.31f, 0.4f, 0.5f));
        return Brush;
    }
}
