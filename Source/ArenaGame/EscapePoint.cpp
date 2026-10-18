#include "EscapePoint.h"
#include "LootCollectorComponent.h"
#include "HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

AEscapePoint::AEscapePoint()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EscapeMesh"));
    SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Mesh->SetStaticMesh(Shape.Object);
    Mesh->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.08f));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
    Mesh->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Pad(TEXT("/Game/UI/Materials/M_EscapePad.M_EscapePad"));
    PadMaterial = Pad.Object;
    if (PadMaterial) Mesh->SetMaterial(0, PadMaterial);
    Outline = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EscapeOutline"));
    Outline->SetupAttachment(Mesh);
    Outline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Outline->SetGenerateOverlapEvents(false);
    Outline->SetCanEverAffectNavigation(false);
    Outline->SetCastShadow(false);
    Outline->SetBoundsScale(1.2f);
    Outline->SetVisibility(false);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Green(TEXT("/Game/UI/Materials/M_EscapePadXRay.M_EscapePadXRay"));
    OutlineMaterial = Green.Object;
    Prompt = CreateDefaultSubobject<UTextRenderComponent>(TEXT("EscapePrompt"));
    Prompt->SetupAttachment(Mesh);
    Prompt->SetAbsolute(false, false, true);
    Prompt->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    Prompt->SetHorizontalAlignment(EHTA_Center);
    Prompt->SetWorldSize(16.f);
    Prompt->SetVisibility(false);
}

bool AEscapePoint::HasEnoughLoot(const ULootCollectorComponent* Collector) const
{
    return Collector && Collector->GetTotalValue() >= FMath::Max(0, RequiredValue);
}

bool AEscapePoint::CanInteract(const ULootCollectorComponent* Collector) const
{
    const APawn* Pawn = Collector ? Cast<APawn>(Collector->GetOwner()) : nullptr;
    if (!Pawn || !Pawn->IsPlayerControlled() || Collector->IsEndingGame()) return false;
    const UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
    if (Health && Health->IsDead()) return false;
    if (FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) > FMath::Square(FMath::Max(1.f, InteractionRange))) return false;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EscapeVisibility), false, Pawn);
    return !GetWorld()->LineTraceSingleByChannel(Hit, Pawn->GetPawnViewLocation(), Mesh->Bounds.Origin, ECC_Visibility, Params)
        || Hit.GetActor() == this;
}

void AEscapePoint::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    APawn* Pawn = PC && PC->IsLocalController() ? PC->GetPawn() : nullptr;
    ULootCollectorComponent* Loot = Pawn ? Pawn->FindComponentByClass<ULootCollectorComponent>() : nullptr;
    const bool bReady = HasEnoughLoot(Loot);
    const bool bShow = Loot && !Loot->IsEndingGame();
    if (!PadDynamic && PadMaterial)
    {
        PadDynamic = UMaterialInstanceDynamic::Create(PadMaterial, this);
        for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index) Mesh->SetMaterial(Index, PadDynamic);
    }
    if (PadDynamic) PadDynamic->SetVectorParameterValue(TEXT("GlowColor"),
        bReady ? FLinearColor(0.04f, 1.f, 0.18f) : FLinearColor(1.f, 0.015f, 0.008f));
    Mesh->SetVisibility(bShow);
    if (Outline->GetStaticMesh() != Mesh->GetStaticMesh())
    {
        Outline->SetStaticMesh(Mesh->GetStaticMesh());
        for (int32 Index = 0; Index < Outline->GetNumMaterials(); ++Index) Outline->SetMaterial(Index, OutlineMaterial);
    }
    Outline->SetVisibility(bShow && bReady);
    Prompt->SetVisibility(bShow && !Loot->IsInventoryOpen() && CanInteract(Loot));
    if (Prompt->IsVisible())
    {
        Prompt->SetWorldLocation(Mesh->Bounds.Origin + FVector(0.f, 0.f, 90.f));
        Prompt->SetText(bReady ? NSLOCTEXT("Escape", "Ready", "[E] ESCAPE") :
            FText::Format(NSLOCTEXT("Escape", "Locked", "ESCAPE LOCKED\n${0} / ${1}"),
                FText::AsNumber(Loot->GetTotalValue()), FText::AsNumber(RequiredValue)));
        Prompt->SetTextRenderColor(bReady ? FColor(60, 255, 120) : FColor(255, 100, 90));
        Prompt->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation() - Prompt->GetComponentLocation()).Rotation());
    }
}
