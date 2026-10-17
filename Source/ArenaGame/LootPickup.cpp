#include "LootPickup.h"
#include "LootCollectorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ALootPickup::ALootPickup()
{
    bReplicates = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        Mesh->SetStaticMesh(Cube.Object);
    }
    // A visible placeholder; replace the mesh with a Fab asset in a Blueprint.
    Mesh->SetRelativeScale3D(FVector(0.3f));
}

bool ALootPickup::TryCollect(ULootCollectorComponent* Collector)
{
    if (!HasAuthority() || bCollected || !IsValid(Collector) || !Collector->CanCollect(this))
    {
        return false;
    }
    bCollected = true;
    Collector->RecordPickup(this);
    Destroy();
    return true;
}
