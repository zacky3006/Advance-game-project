#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunResultsWidget.generated.h"

struct FLootEntry;
class UTextBlock;
class UVerticalBox;
class UButton;

UCLASS()
class ARENAGAME_API URunResultsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Populate(const TArray<FLootEntry>& Entries, double RunSeconds);
    void FocusFinish();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    friend class UCoopPIETestLibrary;
    UFUNCTION()
    void FinishClicked();
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Rows;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Count;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Value;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Duration;
    UPROPERTY(Transient) TObjectPtr<UButton> FinishButton;
    float IntroElapsed = 0.f;
    bool bLeaving = false;
};
