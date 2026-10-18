#include "SneakAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void USneakAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
    if (!Character)
    {
        SneakBlendAlpha = 0.f;
        return;
    }
    // bIsCrouched also stays true when a low ceiling prevents standing up.
    const bool bCrouchPose = Character->bIsCrouched && !Character->GetCharacterMovement()->IsFalling();
    SneakBlendAlpha = FMath::FInterpConstantTo(SneakBlendAlpha, bCrouchPose ? 1.f : 0.f, DeltaSeconds, 6.f);
    const FVector LocalVelocity = Character->GetActorRotation().UnrotateVector(Character->GetVelocity());
    SneakMoveSpeed = LocalVelocity.Size2D();
    if (SneakMoveSpeed > 3.f)
        SneakDirection = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
}
