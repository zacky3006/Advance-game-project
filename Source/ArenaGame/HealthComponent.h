// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ARENAGAME_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    // Sets default values for this component's properties
    UHealthComponent();


protected:

    // Called when the game starts
    virtual void BeginPlay() override;

    // Sets SecretStat after PlayerState is ready
    void InitSecretStat();


public:

    // Called every frame
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;


    // ============================================================
    // HEALTH
    // ============================================================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category="Health"
    )
    float MaxHealth = 100.0f;


    UPROPERTY(
        ReplicatedUsing=OnRep_Health,
        BlueprintReadOnly,
        Category="Health"
    )
    float Health = 100.0f;


    UFUNCTION()
    void OnRep_Health(float OldHealth);


    // ============================================================
    // REPLICATION
    // ============================================================

    virtual void GetLifetimeReplicatedProps(
        TArray<class FLifetimeProperty>& OutLifetimeProps
    ) const override;


    // ============================================================
    // SECRET STAT
    // ============================================================

    UPROPERTY(
        ReplicatedUsing=OnRep_SecretStat
    )
    int32 SecretStat = 0;


    UFUNCTION()
    void OnRep_SecretStat();


    // ============================================================
    // DAMAGE / HEAL
    // ============================================================

    UFUNCTION(
        BlueprintCallable,
        Category="Health"
    )
    void ApplyDamage(float Damage);


    UFUNCTION(
        BlueprintCallable,
        Category="Health"
    )
    void Heal(float Amount);


    // ============================================================
    // HEALTH INFORMATION
    // ============================================================

    UFUNCTION(
        BlueprintPure,
        Category="Health"
    )
    float GetHealthPercent() const;


    UFUNCTION(
        BlueprintPure,
        Category="Health"
    )
    bool IsDead() const;
};