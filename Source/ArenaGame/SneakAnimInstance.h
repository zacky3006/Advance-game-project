#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SneakAnimInstance.generated.h"

/** Crouch animation inputs derived locally from replicated character movement. */
UCLASS()
class ARENAGAME_API USneakAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadOnly, Category="Sneak")
    float SneakBlendAlpha = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Sneak")
    float SneakDirection = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Sneak")
    float SneakMoveSpeed = 0.f;
};
