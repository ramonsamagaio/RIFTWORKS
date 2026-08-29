#include "RiftPhaseField.h"

#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ARiftPhaseField::ARiftPhaseField()
{
    PrimaryActorTick.bCanEverTick = true;

    CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhaseCore"));
    SetRootComponent(CoreMesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded())
    {
        CoreMesh->SetStaticMesh(Sphere.Object);
    }
    CoreMesh->SetRelativeScale3D(FVector(0.34f));
    CoreMesh->SetCollisionProfileName(TEXT("BlockAll"));

    Field = CreateDefaultSubobject<USphereComponent>(TEXT("PhaseField"));
    Field->SetupAttachment(CoreMesh);
    Field->SetSphereRadius(Radius);
    Field->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Field->SetCollisionResponseToAllChannels(ECR_Overlap);
    Field->SetGenerateOverlapEvents(true);

    CoreLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PhaseLight"));
    CoreLight->SetupAttachment(CoreMesh);
    CoreLight->IntensityUnits = ELightUnits::Lumens;
    CoreLight->SetIntensity(1650.0f);
    CoreLight->SetAttenuationRadius(Radius * 1.25f);
    CoreLight->SetSourceRadius(14.0f);
    CoreLight->SetLightColor(FLinearColor(0.22f, 0.78f, 1.0f));
    CoreLight->SetVolumetricScatteringIntensity(0.10f);
}

void ARiftPhaseField::BeginPlay()
{
    Super::BeginPlay();
    if (Field)
    {
        Field->SetSphereRadius(Radius);
    }
    if (CoreLight)
    {
        CoreLight->SetVisibility(bEnabled, true);
    }
}

void ARiftPhaseField::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RestoreAll();
    Super::EndPlay(EndPlayReason);
}

void ARiftPhaseField::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    (void)DeltaSeconds;

    if (!bEnabled || !Field)
    {
        RestoreAll();
        return;
    }
    UpdatePhasedBodies();
}

void ARiftPhaseField::UpdatePhasedBodies()
{
    TArray<UPrimitiveComponent*> Overlaps;
    Field->GetOverlappingComponents(Overlaps);
    TSet<TWeakObjectPtr<UPrimitiveComponent>> Current;

    for (UPrimitiveComponent* Primitive : Overlaps)
    {
        if (!Primitive || Primitive == CoreMesh || Primitive == Field)
        {
            continue;
        }
        if (!Primitive->IsSimulatingPhysics())
        {
            continue;
        }

        const TWeakObjectPtr<UPrimitiveComponent> Key(Primitive);
        Current.Add(Key);
        if (!PhasedComponents.Contains(Key))
        {
            FRiftPhaseRestoreState State;
            State.CollisionEnabled = Primitive->GetCollisionEnabled();
            State.bGravityEnabled = Primitive->IsGravityEnabled();
            PhasedComponents.Add(Key, State);
        }

        Primitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Primitive->SetEnableGravity(false);
        Primitive->SetPhysicsLinearVelocity(Primitive->GetPhysicsLinearVelocity() * 0.985f);
        Primitive->SetPhysicsAngularVelocityInRadians(Primitive->GetPhysicsAngularVelocityInRadians() * 0.97f);
    }

    TArray<TWeakObjectPtr<UPrimitiveComponent>> RestoreKeys;
    for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, FRiftPhaseRestoreState>& Pair : PhasedComponents)
    {
        if (!Current.Contains(Pair.Key))
        {
            if (UPrimitiveComponent* Primitive = Pair.Key.Get())
            {
                RestoreComponent(Primitive, Pair.Value);
            }
            RestoreKeys.Add(Pair.Key);
        }
    }
    for (const TWeakObjectPtr<UPrimitiveComponent>& Key : RestoreKeys)
    {
        PhasedComponents.Remove(Key);
    }
}

void ARiftPhaseField::RestoreComponent(UPrimitiveComponent* Primitive, const FRiftPhaseRestoreState& State)
{
    if (!Primitive)
    {
        return;
    }
    Primitive->SetCollisionEnabled(State.CollisionEnabled);
    Primitive->SetEnableGravity(State.bGravityEnabled);
    Primitive->WakeAllRigidBodies();
}

void ARiftPhaseField::RestoreAll()
{
    for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, FRiftPhaseRestoreState>& Pair : PhasedComponents)
    {
        if (UPrimitiveComponent* Primitive = Pair.Key.Get())
        {
            RestoreComponent(Primitive, Pair.Value);
        }
    }
    PhasedComponents.Reset();
}

void ARiftPhaseField::SetSignal(bool bSignal)
{
    bEnabled = bSignal;
    if (CoreLight)
    {
        CoreLight->SetVisibility(bEnabled, true);
        CoreLight->SetIntensity(bEnabled ? 1650.0f : 0.0f);
    }
    if (!bEnabled)
    {
        RestoreAll();
    }
}

FText ARiftPhaseField::GetInteractionText_Implementation() const
{
    return FText::FromString(FString::Printf(
        TEXT("[E] PHASE Field | %s | physics cargo becomes non-solid"),
        bEnabled ? TEXT("ACTIVE") : TEXT("OFF")));
}

void ARiftPhaseField::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    (void)Player;
    SetSignal(!bEnabled);
}
