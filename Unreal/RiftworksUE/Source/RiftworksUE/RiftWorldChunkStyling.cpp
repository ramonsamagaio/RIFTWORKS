#include "RiftWorldDirector.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace RiftWorldStylePrivate
{
    void Tint(UHierarchicalInstancedStaticMeshComponent* Component, const FLinearColor& Color, float Roughness)
    {
        if (!Component)
        {
            return;
        }
        if (UMaterialInstanceDynamic* Material = Component->CreateDynamicMaterialInstance(0))
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
            Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
        }
    }
}

void ARiftWorldChunk::BeginPlay()
{
    Super::BeginPlay();
    ApplyPrototypeMaterials();
}

void ARiftWorldChunk::ApplyPrototypeMaterials()
{
    RiftWorldStylePrivate::Tint(GroundInstances, FLinearColor(0.055f, 0.064f, 0.055f, 1.0f), 0.96f);
    RiftWorldStylePrivate::Tint(BuildingInstances, FLinearColor(0.16f, 0.18f, 0.20f, 1.0f), 0.88f);
    RiftWorldStylePrivate::Tint(RoadInstances, FLinearColor(0.045f, 0.048f, 0.052f, 1.0f), 0.94f);
    RiftWorldStylePrivate::Tint(TrunkInstances, FLinearColor(0.12f, 0.075f, 0.045f, 1.0f), 1.0f);
    RiftWorldStylePrivate::Tint(FoliageInstances, FLinearColor(0.035f, 0.10f, 0.065f, 1.0f), 0.95f);
}
