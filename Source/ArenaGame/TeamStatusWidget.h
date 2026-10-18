#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TeamStatusWidget.generated.h"
class UTextBlock;
UCLASS()
class ARENAGAME_API UTeamStatusWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Progress;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
};
