// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GenericTeamAgentInterface.h"
#include "ArenaGameCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UHealthComponent;
class USphereComponent;

struct FInputActionValue;


UCLASS(abstract)
class ARENAGAME_API AArenaGameCharacter
    : public ACharacter,
      public IGenericTeamAgentInterface
{
    GENERATED_BODY()


protected:

    // =====================================================
    // Components
    // =====================================================

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Components",
        meta = (AllowPrivateAccess = "true")
    )
    USpringArmComponent* CameraBoom;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Components",
        meta = (AllowPrivateAccess = "true")
    )
    UCameraComponent* FollowCamera;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Health",
        meta = (AllowPrivateAccess = "true")
    )
    UHealthComponent* HealthComponent;


    // =====================================================
    // Input
    // =====================================================

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* MouseLookAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* AttackAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* HealAction;

    /**
     * Sprint Input Action.
     *
     * Set this to IA_Sprint in BP_ThirdPersonCharacter.
     *
     * The C++ variable is intentionally named RunAction
     * because that is the property name currently used
     * by the Blueprint.
     */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* RunAction;

    /**
     * Sneak Input Action.
     *
     * Set this to IA_Sneak in BP_ThirdPersonCharacter.
     */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* SneakAction;


    // =====================================================
    // Movement
    // =====================================================

    UPROPERTY(EditAnywhere, Category = "Movement")
    float WalkSpeed = 500.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float SprintSpeed = 900.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float SneakSpeed = 200.f;


    /**
     * Whether the character is currently sprinting.
     *
     * Replicated from Server to Clients.
     */
    UPROPERTY(Replicated)
    bool bIsSprinting = false;


    /**
     * Whether the character is currently sneaking.
     *
     * Replicated from Server to Clients.
     */
    UPROPERTY(Replicated)
    bool bIsSneaking = false;


    // =====================================================
    // AI Perception
    // =====================================================

    /**
     * Time between footstep noise reports.
     */
    UPROPERTY(EditDefaultsOnly, Category = "AI Perception")
    float FootstepInterval = 0.4f;


    /**
     * How far AI can hear normal walking footsteps.
     */
    UPROPERTY(EditAnywhere, Category = "AI Perception")
    float FootstepRange = 900.f;


    /**
     * How far AI can hear sprinting footsteps.
     */
    UPROPERTY(EditAnywhere, Category = "AI Perception")
    float SprintFootstepRange = 1800.f;


    /**
     * How far a melee swing can be heard.
     */
    UPROPERTY(EditDefaultsOnly, Category = "AI Perception")
    float SwingNoiseRange = 2500.f;


    /**
     * Runtime timer used to generate footsteps.
     */
    float FootstepTimer = 0.f;


    // =====================================================
    // Combat
    // =====================================================

    /**
     * Minimum time between melee attacks.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float MinAttackInterval = 1.0f;


    /**
     * Time when the last melee attack happened.
     */
    float LastAttackTime = -1000.f;


    // =====================================================
    // Sprint / Sneak
    // =====================================================

    /**
     * Update movement speed according to current state.
     */
    void UpdateMovementSpeed();


    /**
     * Start sprinting.
     */
    void OnSprintStart(
        const FInputActionValue& Value
    );


    /**
     * Stop sprinting.
     */
    void OnSprintEnd(
        const FInputActionValue& Value
    );


    /**
     * Start sneaking.
     */
    void OnSneakStart(
        const FInputActionValue& Value
    );


    /**
     * Stop sneaking.
     */
    void OnSneakEnd(
        const FInputActionValue& Value
    );


    // =====================================================
    // Server RPC - Sprint
    // =====================================================

    /**
     * Client -> Server:
     * Tell the server that sprint state changed.
     */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetSprintState(
        bool bNewSprinting
    );


    // =====================================================
    // Server RPC - Sneak
    // =====================================================

    /**
     * Client -> Server:
     * Tell the server that sneak state changed.
     */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetSneakState(
        bool bNewSneaking
    );


    // =====================================================
    // Tick
    // =====================================================

    /**
     * Generate AI perception noise while the character moves.
     */
    virtual void Tick(
        float DeltaSeconds
    ) override;


public:

    // =====================================================
    // Constructor
    // =====================================================

    AArenaGameCharacter();


    // =====================================================
    // Team
    // =====================================================

    /**
     * Character does not own a team.
     * It asks its Controller for the team.
     */
    virtual FGenericTeamId GetGenericTeamId() const override;


protected:

    // =====================================================
    // Input Binding
    // =====================================================

    virtual void SetupPlayerInputComponent(
        class UInputComponent* PlayerInputComponent
    ) override;


    /**
     * Called for movement input.
     */
    void Move(
        const FInputActionValue& Value
    );


    /**
     * Called for looking input.
     */
    void Look(
        const FInputActionValue& Value
    );


    /**
     * Called for attack input.
     */
    void OnAttack(
        const FInputActionValue& Value
    );


    /**
     * Called for heal input.
     */
    void OnHeal(
        const FInputActionValue& Value
    );


public:

    // =====================================================
    // Input Helpers
    // =====================================================

    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoMove(
        float Right,
        float Forward
    );


    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoLook(
        float Yaw,
        float Pitch
    );


    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpStart();


    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpEnd();


public:

    // =====================================================
    // Camera
    // =====================================================

    FORCEINLINE USpringArmComponent* GetCameraBoom() const
    {
        return CameraBoom;
    }


    FORCEINLINE UCameraComponent* GetFollowCamera() const
    {
        return FollowCamera;
    }


public:

    // =====================================================
    // Melee
    // =====================================================

    /**
     * Maximum melee attack range.
     */
    UPROPERTY(EditAnywhere, Category = "Melee")
    float MeleeRange = 150.f;


    /**
     * Half angle of the melee attack cone.
     */
    UPROPERTY(EditAnywhere, Category = "Melee")
    float MeleeHalfAngle = 60.f;


    /**
     * Damage dealt by melee attack.
     */
    UPROPERTY(EditAnywhere, Category = "Melee")
    float MeleeDamage = 5.f;


    /**
     * Sphere used to find melee targets.
     */
    UPROPERTY(
        VisibleAnywhere,
        Category = "Melee"
    )
    USphereComponent* MeleeReach;


    // =====================================================
    // Networked Combat
    // =====================================================

    /**
     * Client -> Server:
     * Perform melee attack.
     */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerMeleeAttack();


    /**
     * Client -> Server:
     * Perform heal.
     */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerHeal();


    /**
     * Server -> Everyone:
     * Play melee hit effect.
     */
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayHitFX(
        FVector HitLocation
    );


    // =====================================================
    // Damage
    // =====================================================

    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator,
        AActor* DamageCauser
    ) override;


protected:

    // =====================================================
    // Replication
    // =====================================================

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps
    ) const override;
};