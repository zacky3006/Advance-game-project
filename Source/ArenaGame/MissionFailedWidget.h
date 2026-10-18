#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MissionFailedWidget.generated.h"
class UTextBlock;

UCLASS()
class ARENAGAME_API UMissionFailedWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    UPROPERTY() TObjectPtr<UTextBlock> Message;
    float Elapsed = 0.f;
};
