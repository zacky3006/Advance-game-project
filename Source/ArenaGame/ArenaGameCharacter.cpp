// Copyright Epic Games, Inc. All Rights Reserved.

#include "ArenaGameCharacter.h"


// =====================================================
// Engine
// =====================================================

#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"


// =====================================================
// Enhanced Input
// =====================================================

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"


// =====================================================
// Project
// =====================================================

#include "ArenaGame.h"
#include "HealthComponent.h"
#include "ArenaGameGameMode.h"


// =====================================================
// Components / Network
// =====================================================

#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"


// =====================================================
// Gameplay / Damage
// =====================================================

#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"


// =====================================================
// AI Perception
// =====================================================

#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Damage.h"


// =====================================================
// Constructor
// =====================================================

AArenaGameCharacter::AArenaGameCharacter()
{
    // -------------------------------------------------
    // Tick
    // -------------------------------------------------

    PrimaryActorTick.bCanEverTick = true;


    // -------------------------------------------------
    // Capsule
    // -------------------------------------------------

    GetCapsuleComponent()->InitCapsuleSize(
        42.f,
        96.f
    );


    // -------------------------------------------------
    // Character Rotation
    // -------------------------------------------------

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;


    // -------------------------------------------------
    // Character Movement
    // -------------------------------------------------

    GetCharacterMovement()->bOrientRotationToMovement = true;

    GetCharacterMovement()->RotationRate =
        FRotator(
            0.f,
            500.f,
            0.f
        );

    GetCharacterMovement()->JumpZVelocity = 700.f;

    GetCharacterMovement()->AirControl = 0.35f;

    GetCharacterMovement()->MaxWalkSpeed =
        WalkSpeed;


    // Enable crouching.
    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;


    // -------------------------------------------------
    // Camera Boom
    // -------------------------------------------------

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(
        TEXT("CameraBoom")
    );

    CameraBoom->SetupAttachment(
        RootComponent
    );

    CameraBoom->TargetArmLength = 400.f;

    CameraBoom->bUsePawnControlRotation = true;


    // -------------------------------------------------
    // Follow Camera
    // -------------------------------------------------

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(
        TEXT("FollowCamera")
    );

    FollowCamera->SetupAttachment(
        CameraBoom,
        USpringArmComponent::SocketName
    );

    FollowCamera->bUsePawnControlRotation = false;


    // -------------------------------------------------
    // Health Component
    // -------------------------------------------------

    HealthComponent =
        CreateDefaultSubobject<UHealthComponent>(
            TEXT("HealthComponent")
        );


    // -------------------------------------------------
    // Melee Reach
    // -------------------------------------------------

    MeleeReach =
        CreateDefaultSubobject<USphereComponent>(
            TEXT("MeleeReach")
        );

    MeleeReach->SetupAttachment(
        RootComponent
    );

    MeleeReach->SetSphereRadius(
        MeleeRange
    );

    MeleeReach->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly
    );

    MeleeReach->SetCollisionResponseToAllChannels(
        ECR_Overlap
    );


    // -------------------------------------------------
    // Replication
    // -------------------------------------------------

    bReplicates = true;

    SetReplicateMovement(true);
}


// =====================================================
// Replication
// =====================================================

void AArenaGameCharacter::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
    Super::GetLifetimeReplicatedProps(
        OutLifetimeProps
    );

    DOREPLIFETIME(
        AArenaGameCharacter,
        bIsSprinting
    );

    DOREPLIFETIME(
        AArenaGameCharacter,
        bIsSneaking
    );
}


// =====================================================
// Tick
// =====================================================

void AArenaGameCharacter::Tick(
    float DeltaSeconds
)
{
    Super::Tick(
        DeltaSeconds
    );


    // -------------------------------------------------
    // AI Hearing is server-side
    // -------------------------------------------------

    if (!HasAuthority())
    {
        return;
    }


    // -------------------------------------------------
    // Only player characters create footsteps
    // -------------------------------------------------

    if (!IsPlayerControlled())
    {
        return;
    }


    // -------------------------------------------------
    // Character must actually be moving
    // -------------------------------------------------

    if (GetVelocity().SizeSquared() < 1.f)
    {
        return;
    }


    // -------------------------------------------------
    // SNEAK = NO FOOTSTEP SOUND
    // -------------------------------------------------

    if (bIsSneaking)
    {
        return;
    }


    // -------------------------------------------------
    // Footstep timer
    // -------------------------------------------------

    FootstepTimer += DeltaSeconds;

    if (FootstepTimer < FootstepInterval)
    {
        return;
    }

    FootstepTimer = 0.f;


    // -------------------------------------------------
    // Select noise range
    // -------------------------------------------------

    const float CurrentFootstepRange =
        bIsSprinting
            ? SprintFootstepRange
            : FootstepRange;


    // -------------------------------------------------
    // Select noise tag
    // -------------------------------------------------

    const FName NoiseTag =
        bIsSprinting
            ? TEXT("SprintFootstep")
            : TEXT("Footstep");


    // -------------------------------------------------
    // Report noise to AI Hearing
    // -------------------------------------------------

    UAISense_Hearing::ReportNoiseEvent(
        GetWorld(),
        GetActorLocation(),
        1.f,
        this,
        CurrentFootstepRange,
        NoiseTag
    );
}


// =====================================================
// Team
// =====================================================

FGenericTeamId AArenaGameCharacter::GetGenericTeamId() const
{
    if (const IGenericTeamAgentInterface* TeamAgent =
        Cast<IGenericTeamAgentInterface>(GetController()))
    {
        return TeamAgent->GetGenericTeamId();
    }

    return FGenericTeamId::NoTeam;
}


// =====================================================
// Input Setup
// =====================================================

void AArenaGameCharacter::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent
)
{
    Super::SetupPlayerInputComponent(
        PlayerInputComponent
    );


    UEnhancedInputComponent* EnhancedInputComponent =
        Cast<UEnhancedInputComponent>(
            PlayerInputComponent
        );

    if (!EnhancedInputComponent)
    {
        return;
    }


    // -------------------------------------------------
    // Jump
    // -------------------------------------------------

    if (JumpAction)
    {
        EnhancedInputComponent->BindAction(
            JumpAction,
            ETriggerEvent::Started,
            this,
            &AArenaGameCharacter::DoJumpStart
        );

        EnhancedInputComponent->BindAction(
            JumpAction,
            ETriggerEvent::Completed,
            this,
            &AArenaGameCharacter::DoJumpEnd
        );
    }


    // -------------------------------------------------
    // Move
    // -------------------------------------------------

    if (MoveAction)
    {
        EnhancedInputComponent->BindAction(
            MoveAction,
            ETriggerEvent::Triggered,
            this,
            &AArenaGameCharacter::Move
        );
    }


    // -------------------------------------------------
    // Look
    // -------------------------------------------------

    if (LookAction)
    {
        EnhancedInputComponent->BindAction(
            LookAction,
            ETriggerEvent::Triggered,
            this,
            &AArenaGameCharacter::Look
        );
    }


    // -------------------------------------------------
    // Mouse Look
    // -------------------------------------------------

    if (MouseLookAction)
    {
        EnhancedInputComponent->BindAction(
            MouseLookAction,
            ETriggerEvent::Triggered,
            this,
            &AArenaGameCharacter::Look
        );
    }


    // -------------------------------------------------
    // Attack
    // -------------------------------------------------

    if (AttackAction)
    {
        EnhancedInputComponent->BindAction(
            AttackAction,
            ETriggerEvent::Started,
            this,
            &AArenaGameCharacter::OnAttack
        );
    }


    // -------------------------------------------------
    // Heal
    // -------------------------------------------------

    if (HealAction)
    {
        EnhancedInputComponent->BindAction(
            HealAction,
            ETriggerEvent::Started,
            this,
            &AArenaGameCharacter::OnHeal
        );
    }


    // -------------------------------------------------
    // Sprint
    //
    // RunAction -> IA_Sprint
    // -------------------------------------------------

    if (RunAction)
    {
        EnhancedInputComponent->BindAction(
            RunAction,
            ETriggerEvent::Started,
            this,
            &AArenaGameCharacter::OnSprintStart
        );

        EnhancedInputComponent->BindAction(
            RunAction,
            ETriggerEvent::Completed,
            this,
            &AArenaGameCharacter::OnSprintEnd
        );
    }


    // -------------------------------------------------
    // Sneak
    //
    // SneakAction -> IA_Sneak
    // -------------------------------------------------

    if (SneakAction)
    {
        EnhancedInputComponent->BindAction(
            SneakAction,
            ETriggerEvent::Started,
            this,
            &AArenaGameCharacter::OnSneakStart
        );

        EnhancedInputComponent->BindAction(
            SneakAction,
            ETriggerEvent::Completed,
            this,
            &AArenaGameCharacter::OnSneakEnd
        );
    }
}


// =====================================================
// Move
// =====================================================

void AArenaGameCharacter::Move(
    const FInputActionValue& Value
)
{
    const FVector2D MovementVector =
        Value.Get<FVector2D>();

    DoMove(
        MovementVector.X,
        MovementVector.Y
    );
}


// =====================================================
// Look
// =====================================================

void AArenaGameCharacter::Look(
    const FInputActionValue& Value
)
{
    const FVector2D LookAxisVector =
        Value.Get<FVector2D>();

    DoLook(
        LookAxisVector.X,
        LookAxisVector.Y
    );
}


// =====================================================
// DoMove
// =====================================================

void AArenaGameCharacter::DoMove(
    float Right,
    float Forward
)
{
    if (Controller == nullptr)
    {
        return;
    }


    const FRotator Rotation =
        Controller->GetControlRotation();


    const FRotator YawRotation(
        0.f,
        Rotation.Yaw,
        0.f
    );


    const FVector ForwardDirection =
        FRotationMatrix(YawRotation)
            .GetUnitAxis(EAxis::X);


    const FVector RightDirection =
        FRotationMatrix(YawRotation)
            .GetUnitAxis(EAxis::Y);


    AddMovementInput(
        ForwardDirection,
        Forward
    );


    AddMovementInput(
        RightDirection,
        Right
    );
}


// =====================================================
// DoLook
// =====================================================

void AArenaGameCharacter::DoLook(
    float Yaw,
    float Pitch
)
{
    AddControllerYawInput(
        Yaw
    );

    AddControllerPitchInput(
        Pitch
    );
}


// =====================================================
// DoJumpStart
// =====================================================

void AArenaGameCharacter::DoJumpStart()
{
    Jump();
}


// =====================================================
// DoJumpEnd
// =====================================================

void AArenaGameCharacter::DoJumpEnd()
{
    StopJumping();
}


// =====================================================
// Attack Input
// =====================================================

void AArenaGameCharacter::OnAttack(
    const FInputActionValue& Value
)
{
    // Server RPC.
    // Client will send the request to Server.
    // If already on Server, Unreal executes it there.
    ServerMeleeAttack();
}


// =====================================================
// Heal Input
// =====================================================

void AArenaGameCharacter::OnHeal(
    const FInputActionValue& Value
)
{
    // Server RPC.
    ServerHeal();
}


// =====================================================
// Update Movement Speed
// =====================================================

void AArenaGameCharacter::UpdateMovementSpeed()
{
    UCharacterMovementComponent* Movement =
        GetCharacterMovement();

    if (!Movement)
    {
        return;
    }


    if (bIsSneaking)
    {
        Movement->MaxWalkSpeed =
            SneakSpeed;
    }
    else if (bIsSprinting)
    {
        Movement->MaxWalkSpeed =
            SprintSpeed;
    }
    else
    {
        Movement->MaxWalkSpeed =
            WalkSpeed;
    }
}


// =====================================================
// Sprint Start
// =====================================================

void AArenaGameCharacter::OnSprintStart(
    const FInputActionValue& Value
)
{
    // Cannot sprint while sneaking.
    if (bIsSneaking)
    {
        return;
    }


    bIsSprinting = true;

    UpdateMovementSpeed();


    // Tell server about the state.
    if (!HasAuthority())
    {
        ServerSetSprintState(
            true
        );
    }
}


// =====================================================
// Sprint End
// =====================================================

void AArenaGameCharacter::OnSprintEnd(
    const FInputActionValue& Value
)
{
    bIsSprinting = false;

    UpdateMovementSpeed();


    // Tell server about the state.
    if (!HasAuthority())
    {
        ServerSetSprintState(
            false
        );
    }
}


// =====================================================
// Server Sprint State
// =====================================================

void AArenaGameCharacter::ServerSetSprintState_Implementation(
    bool bNewSprinting
)
{
    // Do not allow sprint while sneaking.
    if (bIsSneaking && bNewSprinting)
    {
        bNewSprinting = false;
    }


    bIsSprinting = bNewSprinting;

    UpdateMovementSpeed();
}


bool AArenaGameCharacter::ServerSetSprintState_Validate(
    bool bNewSprinting
)
{
    return true;
}


// =====================================================
// Sneak Start
// =====================================================

void AArenaGameCharacter::OnSneakStart(
    const FInputActionValue& Value
)
{
    bIsSneaking = true;

    // Sneaking and sprinting cannot happen together.
    bIsSprinting = false;


    // Change movement speed.
    UpdateMovementSpeed();


    // Start crouching.
    // false = this is not a client-simulation request.
    Crouch(false);


    // Tell server about the state.
    if (!HasAuthority())
    {
        ServerSetSneakState(
            true
        );
    }
}


// =====================================================
// Sneak End
// =====================================================

void AArenaGameCharacter::OnSneakEnd(
    const FInputActionValue& Value
)
{
    bIsSneaking = false;

    UpdateMovementSpeed();


    // Stop crouching.
    Crouch(false);

    // Immediately request standing state.
    UnCrouch(false);


    // Tell server about the state.
    if (!HasAuthority())
    {
        ServerSetSneakState(
            false
        );
    }
}


// =====================================================
// Server Sneak State
// =====================================================

void AArenaGameCharacter::ServerSetSneakState_Implementation(
    bool bNewSneaking
)
{
    bIsSneaking = bNewSneaking;


    // Sneaking and sprinting cannot happen together.
    if (bIsSneaking)
    {
        bIsSprinting = false;

        Crouch(false);
    }
    else
    {
        UnCrouch(false);
    }


    UpdateMovementSpeed();
}


bool AArenaGameCharacter::ServerSetSneakState_Validate(
    bool bNewSneaking
)
{
    return true;
}


// =====================================================
// Server Melee Attack
// =====================================================

void AArenaGameCharacter::ServerMeleeAttack_Implementation()
{
    if (!HasAuthority())
    {
        return;
    }


    // -------------------------------------------------
    // Attack cooldown
    // -------------------------------------------------

    const float CurrentTime =
        GetWorld()->GetTimeSeconds();


    if (CurrentTime - LastAttackTime <
        MinAttackInterval)
    {
        return;
    }


    LastAttackTime = CurrentTime;


    // -------------------------------------------------
    // Find overlapping characters
    // -------------------------------------------------

    TArray<AActor*> OverlappingActors;


    if (MeleeReach)
    {
        MeleeReach->GetOverlappingActors(
            OverlappingActors,
            AArenaGameCharacter::StaticClass()
        );
    }


    // -------------------------------------------------
    // Attack direction
    // -------------------------------------------------

    const FVector Forward =
        GetActorForwardVector();


    FVector HitLocation =
        GetActorLocation();


    bool bHitSomething = false;


    // -------------------------------------------------
    // Check targets
    // -------------------------------------------------

    for (AActor* Actor : OverlappingActors)
    {
        AArenaGameCharacter* Target =
            Cast<AArenaGameCharacter>(
                Actor
            );


        if (!Target || Target == this)
        {
            continue;
        }


        // -------------------------------------------------
        // Team check
        // -------------------------------------------------

        const FGenericTeamId MyTeam =
            GetGenericTeamId();


        const FGenericTeamId TargetTeam =
            Target->GetGenericTeamId();


        // Do not damage same-team characters.
        if (
            MyTeam != FGenericTeamId::NoTeam &&
            TargetTeam != FGenericTeamId::NoTeam &&
            MyTeam == TargetTeam
        )
        {
            continue;
        }


        // -------------------------------------------------
        // Distance check
        // -------------------------------------------------

        FVector ToTarget =
            Target->GetActorLocation()
            - GetActorLocation();


        const float Distance =
            ToTarget.Size();


        if (Distance > MeleeRange)
        {
            continue;
        }


        if (Distance <= KINDA_SMALL_NUMBER)
        {
            continue;
        }


        ToTarget.Normalize();


        // -------------------------------------------------
        // Cone angle check
        // -------------------------------------------------

        const float Dot =
            FVector::DotProduct(
                Forward,
                ToTarget
            );


        const float Angle =
            FMath::RadiansToDegrees(
                FMath::Acos(
                    FMath::Clamp(
                        Dot,
                        -1.f,
                        1.f
                    )
                )
            );


        if (Angle > MeleeHalfAngle)
        {
            continue;
        }


        // -------------------------------------------------
        // Apply damage
        // -------------------------------------------------

        UGameplayStatics::ApplyDamage(
            Target,
            MeleeDamage,
            GetController(),
            this,
            UDamageType::StaticClass()
        );


        HitLocation =
            Target->GetActorLocation();


        bHitSomething = true;


        // One target per attack.
        break;
    }


    // -------------------------------------------------
    // Melee swing noise
    // -------------------------------------------------

    UAISense_Hearing::ReportNoiseEvent(
        GetWorld(),
        GetActorLocation(),
        1.f,
        this,
        SwingNoiseRange,
        TEXT("Swing")
    );


    // -------------------------------------------------
    // Hit FX
    // -------------------------------------------------

    if (bHitSomething)
    {
        MulticastPlayHitFX(
            HitLocation
        );
    }
}


// =====================================================
// Server Melee Attack Validation
// =====================================================

bool AArenaGameCharacter::ServerMeleeAttack_Validate()
{
    return true;
}


// =====================================================
// Server Heal
// =====================================================

void AArenaGameCharacter::ServerHeal_Implementation()
{
    if (!HasAuthority())
    {
        return;
    }


    if (!HealthComponent)
    {
        return;
    }


    if (HealthComponent->IsDead())
    {
        return;
    }


    HealthComponent->Heal(
        5.f
    );
}


// =====================================================
// Server Heal Validation
// =====================================================

bool AArenaGameCharacter::ServerHeal_Validate()
{
    return true;
}


// =====================================================
// Take Damage
// =====================================================

float AArenaGameCharacter::TakeDamage(
    float DamageAmount,
    FDamageEvent const& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser
)
{
    // Damage must be processed on Server.
    if (!HasAuthority())
    {
        return 0.f;
    }


    if (!HealthComponent)
    {
        return 0.f;
    }


    // Do not damage an already dead character.
    if (HealthComponent->IsDead())
    {
        return 0.f;
    }


    // -------------------------------------------------
    // Super
    // -------------------------------------------------

    const float Applied =
        Super::TakeDamage(
            DamageAmount,
            DamageEvent,
            EventInstigator,
            DamageCauser
        );


    // -------------------------------------------------
    // Report damage to AI Perception
    // -------------------------------------------------

    UAISense_Damage::ReportDamageEvent(
        GetWorld(),
        this,
        EventInstigator
            ? EventInstigator->GetPawn()
            : nullptr,
        DamageAmount,
        DamageCauser
            ? DamageCauser->GetActorLocation()
            : GetActorLocation(),
        GetActorLocation()
    );


    // -------------------------------------------------
    // Health
    // -------------------------------------------------

    HealthComponent->ApplyDamage(
        DamageAmount
    );


    // -------------------------------------------------
    // Death
    // -------------------------------------------------

    if (HealthComponent->IsDead())
    {
        if (AArenaGameGameMode* GM =
            GetWorld()->GetAuthGameMode<AArenaGameGameMode>())
        {
            GM->OnCharacterKilled(
                EventInstigator,
                GetController()
            );
        }
    }


    return Applied;
}


// =====================================================
// Multicast Hit FX
// =====================================================

void AArenaGameCharacter::MulticastPlayHitFX_Implementation(
    FVector HitLocation
)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "Melee Hit FX at: %s"
        ),
        *HitLocation.ToString()
    );
}