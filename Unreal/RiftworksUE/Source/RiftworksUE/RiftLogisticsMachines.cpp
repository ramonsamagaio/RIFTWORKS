#include "RiftLogisticsMachines.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace RiftLogisticsPrivate
{
    UStaticMesh* FindCube()
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
        return Mesh.Succeeded() ? Mesh.Object : nullptr;
    }
}

ARiftPoweredConveyor::ARiftPoweredConveyor()
{
    PrimaryActorTick.bCanEverTick = true;
    Kind = ERiftPowerKind::Consumer;
    DeviceName = FText::FromString(TEXT("Powered Conveyor"));
    ConsumptionKW = 0.55f;
    Priority = 4;

    if (Mesh)
    {
        Mesh->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.65f));
    }

    Belt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConveyorBelt"));
    Belt->SetupAttachment(Mesh);
    Belt->SetStaticMesh(RiftLogisticsPrivate::FindCube());
    Belt->SetRelativeLocation(FVector(125.0f, 0.0f, 56.0f));
    Belt->SetRelativeScale3D(FVector(3.6f, 1.05f, 0.16f));
    Belt->SetCollisionProfileName(TEXT("BlockAll"));

    TransportZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ConveyorTransportZone"));
    TransportZone->SetupAttachment(Belt);
    TransportZone->SetRelativeLocation(FVector(0.0f, 0.0f, 42.0f));
    TransportZone->SetBoxExtent(FVector(175.0f, 48.0f, 46.0f));
    TransportZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TransportZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    TransportZone->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
}

void ARiftPoweredConveyor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bEnabled || !bPowered || !TransportZone)
    {
        return;
    }

    TArray<UPrimitiveComponent*> Overlaps;
    TransportZone->GetOverlappingComponents(Overlaps);
    const FVector Direction = Belt ? Belt->GetForwardVector() : GetActorForwardVector();
    for (UPrimitiveComponent* Primitive : Overlaps)
    {
        if (!Primitive || !Primitive->IsSimulatingPhysics())
        {
            continue;
        }
        const float Mass = FMath::Max(1.0f, Primitive->GetMass());
        Primitive->AddForce(Direction * BeltAcceleration * Mass);
    }
}

FText ARiftPoweredConveyor::GetInteractionText_Implementation() const
{
    return FText::FromString(FString::Printf(
        TEXT("[E] Powered Conveyor | %s | %.2f kW | priority %d"),
        bEnabled ? (bPowered ? TEXT("RUNNING") : TEXT("STARVED")) : TEXT("OFF"),
        ConsumptionKW,
        Priority));
}

ARiftFreightLift::ARiftFreightLift()
{
    PrimaryActorTick.bCanEverTick = true;
    Kind = ERiftPowerKind::Consumer;
    DeviceName = FText::FromString(TEXT("Freight Lift"));
    ConsumptionKW = 2.4f;
    Priority = 2;

    if (Mesh)
    {
        Mesh->SetRelativeScale3D(FVector(0.38f, 0.38f, 0.85f));
    }

    Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FreightPlatform"));
    Platform->SetupAttachment(Mesh);
    Platform->SetStaticMesh(RiftLogisticsPrivate::FindCube());
    Platform->SetRelativeLocation(FVector(155.0f, 0.0f, 30.0f));
    Platform->SetRelativeScale3D(FVector(2.3f, 2.3f, 0.20f));
    Platform->SetCollisionProfileName(TEXT("BlockAll"));

    LeftRail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LiftRailLeft"));
    LeftRail->SetupAttachment(Mesh);
    LeftRail->SetStaticMesh(RiftLogisticsPrivate::FindCube());
    LeftRail->SetRelativeLocation(FVector(155.0f, -225.0f, 460.0f));
    LeftRail->SetRelativeScale3D(FVector(0.16f, 0.16f, 9.2f));
    LeftRail->SetCollisionProfileName(TEXT("BlockAll"));

    RightRail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LiftRailRight"));
    RightRail->SetupAttachment(Mesh);
    RightRail->SetStaticMesh(RiftLogisticsPrivate::FindCube());
    RightRail->SetRelativeLocation(FVector(155.0f, 225.0f, 460.0f));
    RightRail->SetRelativeScale3D(FVector(0.16f, 0.16f, 9.2f));
    RightRail->SetCollisionProfileName(TEXT("BlockAll"));
}

void ARiftFreightLift::BeginPlay()
{
    Super::BeginPlay();
    if (Platform)
    {
        PlatformBottomLocation = Platform->GetRelativeLocation();
    }
}

void ARiftFreightLift::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Platform || !bMoving || !bEnabled || !bPowered)
    {
        return;
    }

    const FVector Target = PlatformBottomLocation + FVector(0.0f, 0.0f, bTargetTop ? TravelHeight : 0.0f);
    const FVector Current = Platform->GetRelativeLocation();
    const FVector Next = FMath::VInterpConstantTo(Current, Target, DeltaSeconds, LiftSpeed);
    Platform->SetRelativeLocation(Next);

    if (FVector::DistSquared(Next, Target) <= 1.0f)
    {
        Platform->SetRelativeLocation(Target);
        bMoving = false;
    }
}

FText ARiftFreightLift::GetInteractionText_Implementation() const
{
    const TCHAR* Position = bMoving ? TEXT("MOVING") : (bTargetTop ? TEXT("TOP") : TEXT("BOTTOM"));
    const TCHAR* Power = bEnabled ? (bPowered ? TEXT("POWERED") : TEXT("STARVED")) : TEXT("OFF");
    return FText::FromString(FString::Printf(
        TEXT("[E] Freight Lift | %s | %s | %.1f kW"), Position, Power, ConsumptionKW));
}

void ARiftFreightLift::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (!bEnabled)
    {
        SetDeviceEnabled(true);
    }
    if (!bPowered || bMoving)
    {
        if (Player && !bPowered)
        {
            Player->CurrentInteractionText = FText::FromString(TEXT("Freight lift has no available grid power"));
        }
        return;
    }

    bTargetTop = !bTargetTop;
    bMoving = true;
}

ARiftPhysicsCargoCart::ARiftPhysicsCargoCart()
{
    PrimaryActorTick.bCanEverTick = false;

    Deck = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoDeck"));
    SetRootComponent(Deck);
    Deck->SetStaticMesh(RiftLogisticsPrivate::FindCube());
    Deck->SetRelativeScale3D(FVector(1.45f, 0.90f, 0.18f));
    Deck->SetCollisionProfileName(TEXT("PhysicsActor"));
    Deck->SetSimulatePhysics(true);
    Deck->SetMassOverrideInKg(NAME_None, CartMassKg, true);
    Deck->SetLinearDamping(1.15f);
    Deck->SetAngularDamping(3.5f);

    Handle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoHandle"));
    Handle->SetupAttachment(Deck);
    Handle->SetStaticMesh(RiftLogisticsPrivate::FindCube());
    Handle->SetRelativeLocation(FVector(-125.0f, 0.0f, 88.0f));
    Handle->SetRelativeScale3D(FVector(0.12f, 0.82f, 1.35f));
    Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FText ARiftPhysicsCargoCart::GetInteractionText_Implementation() const
{
    return FText::FromString(TEXT("[E] Push cargo cart | physics transport platform"));
}

void ARiftPhysicsCargoCart::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (!Player || !Deck || !Deck->IsSimulatingPhysics())
    {
        return;
    }
    FVector Direction = Player->GetActorForwardVector();
    Direction.Z = 0.0f;
    Direction.Normalize();
    Deck->AddImpulse(Direction * PushImpulse, NAME_None, true);
}
