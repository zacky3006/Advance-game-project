#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AIAlertIndicatorWidget.generated.h"

class UProgressBar;

UCLASS()
class ARENAGAME_API UAIAlertIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Progress bar from Alert_widget Blueprint
	UPROPERTY(
		meta = (BindWidget),
		BlueprintReadOnly,
		Category = "AI Alert"
	)
	TObjectPtr<UProgressBar> AlertProgressBar = nullptr;

	// Update alert progress from 0.0 to 1.0
	UFUNCTION(BlueprintCallable, Category = "AI Alert")
	void SetAlertPercent(float Percent);
};