#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LootHUDWidget.generated.h"

// Concrete native widget: no Widget Blueprint is required for the starter HUD.
UCLASS()
class ARENAGAME_API ULootHUDWidget : public UUserWidget
{
    GENERATED_BODY()
};
