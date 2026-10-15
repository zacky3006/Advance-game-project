// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"


// ============================================================
// CONSTRUCTOR
// ============================================================

UHealthComponent::UHealthComponent()
{
    // Set this component to be ticked every frame.
    // สามารถปิดได้ภายหลังถ้าไม่ได้ใช้ Tick
    PrimaryComponentTick.bCanEverTick = true;

    // IMPORTANT:
    // ทำให้ HealthComponent รองรับ Replication
    SetIsReplicatedByDefault(true);
}


// ============================================================
// BEGIN PLAY
// ============================================================

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    // Health และ SecretStat ต้องถูกกำหนดจาก Server
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        // PlayerState อาจยังไม่พร้อมตอน BeginPlay
        // จึงรอ 0.5 วินาทีก่อนกำหนด SecretStat
        FTimerHandle TimerHandle;

        GetWorld()->GetTimerManager().SetTimer(
            TimerHandle,
            this,
            &UHealthComponent::InitSecretStat,
            0.5f,
            false
        );

        // ป้องกัน Health เกิน MaxHealth
        Health = FMath::Clamp(
            Health,
            0.0f,
            MaxHealth
        );
    }
}


// ============================================================
// INITIALIZE SECRET STAT
// ============================================================

void UHealthComponent::InitSecretStat()
{
    if (!GetOwner())
    {
        return;
    }

    APawn* Pawn = Cast<APawn>(GetOwner());

    if (!Pawn)
    {
        return;
    }

    APlayerState* PlayerState =
        Pawn->GetPlayerState();

    if (!PlayerState)
    {
        return;
    }

    // แต่ละ Player จะได้ค่า SecretStat ต่างกัน
    SecretStat =
        100 + PlayerState->GetPlayerId();

    UE_LOG(
        LogTemp,
        Log,
        TEXT(
            "SecretStat initialized for PlayerId %d = %d"
        ),
        PlayerState->GetPlayerId(),
        SecretStat
    );
}


// ============================================================
// TICK
// ============================================================

void UHealthComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction
    );

    // ไม่มี logic เพิ่มในตอนนี้
}


// ============================================================
// HEALTH REPLICATION
// ============================================================

void UHealthComponent::OnRep_Health(
    float OldHealth
)
{
    // OnRep จะถูกเรียกบน Client
    // เมื่อ Server เปลี่ยนค่า Health

    UE_LOG(
        LogTemp,
        Log,
        TEXT(
            "Health replicated: %s %.2f -> %.2f"
        ),
        *GetNameSafe(GetOwner()),
        OldHealth,
        Health
    );
}


// ============================================================
// SECRET STAT REPLICATION
// ============================================================

void UHealthComponent::OnRep_SecretStat()
{
    FString Who = TEXT("unknown");

    if (APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        if (APlayerState* PlayerState =
            Pawn->GetPlayerState())
        {
            Who = FString::Printf(
                TEXT("PlayerId %d"),
                PlayerState->GetPlayerId()
            );
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "OnRep on [%s] received SecretStat = %d"
        ),
        *Who,
        SecretStat
    );
}


// ============================================================
// APPLY DAMAGE
// ============================================================

void UHealthComponent::ApplyDamage(
    float Damage
)
{
    // ไม่รับ Damage ติดลบ
    if (Damage < 0.0f)
    {
        return;
    }

    // ========================================================
    // SERVER AUTHORITY
    // ========================================================

    // Health ต้องถูกเปลี่ยนโดย Server เท่านั้น
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    // ถ้าตายแล้ว ไม่รับ Damage เพิ่ม
    if (IsDead())
    {
        return;
    }

    const float OldHealth = Health;

    Health -= Damage;

    // ป้องกัน Health ต่ำกว่า 0
    if (Health < 0.0f)
    {
        Health = 0.0f;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT(
            "%s took %.2f damage: %.2f -> %.2f"
        ),
        *GetNameSafe(GetOwner()),
        Damage,
        OldHealth,
        Health
    );
}


// ============================================================
// HEAL
// ============================================================

void UHealthComponent::Heal(
    float Amount
)
{
    // ไม่รับ Heal ติดลบ
    if (Amount < 0.0f)
    {
        return;
    }

    // ========================================================
    // SERVER AUTHORITY
    // ========================================================

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    // คนที่ตายแล้วไม่สามารถ Heal ได้
    if (IsDead())
    {
        return;
    }

    const float OldHealth = Health;

    Health += Amount;

    // ป้องกัน Health เกิน MaxHealth
    if (Health > MaxHealth)
    {
        Health = MaxHealth;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT(
            "%s healed %.2f: %.2f -> %.2f"
        ),
        *GetNameSafe(GetOwner()),
        Amount,
        OldHealth,
        Health
    );
}


// ============================================================
// HEALTH PERCENT
// ============================================================

float UHealthComponent::GetHealthPercent() const
{
    // ป้องกันการหารด้วย 0
    if (MaxHealth <= 0.0f)
    {
        return 0.0f;
    }

    return Health / MaxHealth;
}


// ============================================================
// IS DEAD
// ============================================================

bool UHealthComponent::IsDead() const
{
    return Health <= 0.0f;
}


// ============================================================
// REPLICATION
// ============================================================

void UHealthComponent::GetLifetimeReplicatedProps(
    TArray<class FLifetimeProperty>& OutLifetimeProps
) const
{
    Super::GetLifetimeReplicatedProps(
        OutLifetimeProps
    );

    // Health:
    // ทุก Client สามารถได้รับค่า Health
    DOREPLIFETIME(
        UHealthComponent,
        Health
    );

    // SecretStat:
    // ส่งเฉพาะ Owner ของ Actor เท่านั้น
    DOREPLIFETIME_CONDITION(
        UHealthComponent,
        SecretStat,
        COND_OwnerOnly
    );
}