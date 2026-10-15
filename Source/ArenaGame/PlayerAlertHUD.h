
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerAlertHUD.generated.h"

class AActor;
class UCanvasPanel;
class UAIAlertIndicatorWidget;
class UCanvasPanelSlot;

USTRUCT()
struct FAIAlertIndicatorEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> AIActor = nullptr;

	UPROPERTY()
	TObjectPtr<UAIAlertIndicatorWidget> IndicatorWidget = nullptr;
};

UCLASS()
class ARENAGAME_API UPlayerAlertHUD : public UUserWidget
{
	GENERATED_BODY()

public:

	// Add an indicator for an AI, or update its alert.
	UFUNCTION(BlueprintCallable, Category = "AI Alert")
	void AddOrUpdateIndicator(AActor* AIActor, float AlertPercent);

	// Remove an AI indicator.
	UFUNCTION(BlueprintCallable, Category = "AI Alert")
	void RemoveIndicator(AActor* AIActor);

	// Remove all indicators.
	UFUNCTION(BlueprintCallable, Category = "AI Alert")
	void ClearIndicators();

protected:

	// Canvas Panel in the PlayerAlertHUD Widget Blueprint.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> AlertCanvas = nullptr;

	// Widget class assigned to AIAlertIndicatorWidget Blueprint.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Alert")
	TSubclassOf<UAIAlertIndicatorWidget> IndicatorWidgetClass;

	// One entry per AI.
	UPROPERTY()
	TArray<FAIAlertIndicatorEntry> Indicators;

	virtual void NativeTick(
		const FGeometry& MyGeometry,
		float InDeltaTime
	) override;

private:

	int32 FindIndicatorIndex(AActor* AIActor) const;

	void UpdateIndicatorPosition(
		AActor* AIActor,
		UAIAlertIndicatorWidget* IndicatorWidget
	);
};
