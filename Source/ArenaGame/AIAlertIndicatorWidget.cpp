
#include "AIAlertIndicatorWidget.h"

#include "Components/ProgressBar.h"
#include "Math/UnrealMathUtility.h"


void UAIAlertIndicatorWidget::SetAlertPercent(float Percent)
{
	if (!AlertProgressBar)
	{
		return;
	}

	const float ClampedPercent = FMath::Clamp(Percent, 0.0f, 1.0f);

	AlertProgressBar->SetPercent(ClampedPercent);
}
