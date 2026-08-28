#include "RiftGameplayActors.h"

void ARiftSalvageActor::BeginPlay()
{
    Super::BeginPlay();
    SetCarriedState(false);
}

void ARiftAssemblyPart::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ConfigurePart();
}
