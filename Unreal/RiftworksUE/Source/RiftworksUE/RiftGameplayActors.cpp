#include "RiftGameplayActors.h"

#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace RiftGameplayPrivate
{
    UStaticMesh* FindCube()
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
        return Mesh.Succeeded() ? Mesh.Object : nullptr;
    }

    UStaticMesh* FindCylinder()
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        return Mesh.Succeeded() ? Mesh.Object : nullptr;
    }
}

ARiftSalvageActor::ARiftSalvageActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SalvageMesh"));
    SetRootComponent(Mesh);
    Mesh->SetStaticMesh(RiftGameplayPrivate::FindCube());
    Mesh->SetRelativeScale3D(FVector(0.55f, 0.48f, 0.35f));
    Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    DisplayName = FText::FromString(TEXT("Salvage"));
}

FText ARiftSalvageActor::GetInteractionText_Implementation() const
{
    if (bHeavy)
    {
        return FText::Format(NSLOCTEXT("Riftworks", "HeavySalvage", "[E] Lift {0}  x{1}  |  {2} kg"), DisplayName, FText::AsNumber(Amount), FText::AsNumber(FMath::RoundToInt(MassKg)));
    }
    return FText::Format(NSLOCTEXT("Riftworks", "LightSalvage", "[E] Take {0}  x{1}"), DisplayName, FText::AsNumber(Amount));
}

void ARiftSalvageActor::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (!Player)
    {
        return;
    }
    if (bHeavy)
    {
        Player->TryCarrySalvage(this);
        return;
    }
    if (ItemId == TEXT("scrap"))
    {
        Player->Scrap += Amount;
        Player->BP_OnInventoryChanged();
    }
    else
    {
        Player->AddComponentItem(ItemId, Amount);
    }
    Destroy();
}

void ARiftSalvageActor::SetCarriedState(bool bCarried)
{
    Mesh->SetSimulatePhysics(!bCarried && bHeavy);
    Mesh->SetCollisionEnabled(bCarried ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    if (!bCarried && bHeavy)
    {
        Mesh->SetMassOverrideInKg(NAME_None, MassKg, true);
    }
}

ARiftBaseBeacon::ARiftBaseBeacon()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconMesh"));
    SetRootComponent(Mesh);
    Mesh->SetStaticMesh(RiftGameplayPrivate::FindCylinder());
    Mesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.7f));
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));

    BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
    BeaconLight->SetupAttachment(Mesh);
    BeaconLight->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
    BeaconLight->IntensityUnits = ELightUnits::Lumens;
    BeaconLight->Intensity = 1800.0f;
    BeaconLight->AttenuationRadius = 900.0f;
    BeaconLight->LightColor = FColor(227, 196, 129);
    BeaconLight->VolumetricScatteringIntensity = 1.2f;
    BeaconLight->CastShadows = true;

    Tags.Add(TEXT("RiftBase"));
}

void ARiftBaseBeacon::StoreItem(FName ItemId, int32 Amount)
{
    if (Amount > 0)
    {
        Storage.FindOrAdd(ItemId) += Amount;
    }
}

bool ARiftBaseBeacon::TakeItem(FName ItemId, int32 Amount)
{
    int32* Existing = Storage.Find(ItemId);
    if (!Existing || Amount <= 0 || *Existing < Amount)
    {
        return false;
    }
    *Existing -= Amount;
    return true;
}

bool ARiftBaseBeacon::IsPlayerInRange(const ARiftPlayerCharacter* Player) const
{
    return Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= FMath::Square(AccessRadius);
}

FText ARiftBaseBeacon::GetInteractionText_Implementation() const
{
    int32 TotalItems = 0;
    for (const TPair<FName, int32>& Pair : Storage)
    {
        TotalItems += Pair.Value;
    }
    return FText::Format(NSLOCTEXT("Riftworks", "BaseBeaconPrompt", "[E] Base Storage  |  {0} secured items"), FText::AsNumber(TotalItems));
}

void ARiftBaseBeacon::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (!Player || !IsPlayerInRange(Player))
    {
        return;
    }
    if (Player->Scrap > 0)
    {
        StoreItem(TEXT("scrap"), Player->Scrap);
        Player->Scrap = 0;
    }
    for (const TPair<FName, int32>& Pair : Player->Components)
    {
        if (Pair.Value > 0)
        {
            StoreItem(Pair.Key, Pair.Value);
        }
    }
    Player->Components.Reset();
    Player->BP_OnInventoryChanged();
}

ARiftPowerDevice::ARiftPowerDevice()
{
    PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeviceMesh"));
    SetRootComponent(Mesh);
    Mesh->SetStaticMesh(RiftGameplayPrivate::FindCube());
    Mesh->SetRelativeScale3D(FVector(1.2f, 0.8f, 0.9f));
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));

    StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
    StatusLight->SetupAttachment(Mesh);
    StatusLight->SetRelativeLocation(FVector(0.0f, -55.0f, 55.0f));
    StatusLight->IntensityUnits = ELightUnits::Lumens;
    StatusLight->AttenuationRadius = 220.0f;
    StatusLight->VolumetricScatteringIntensity = 0.0f;

    WorkLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("WorkLight"));
    WorkLight->SetupAttachment(Mesh);
    WorkLight->SetRelativeLocation(FVector(0.0f, -45.0f, 130.0f));
    WorkLight->SetRelativeRotation(FRotator(-10.0f, -90.0f, 0.0f));
    WorkLight->IntensityUnits = ELightUnits::Lumens;
    WorkLight->Intensity = 5200.0f;
    WorkLight->AttenuationRadius = 2600.0f;
    WorkLight->InnerConeAngle = 18.0f;
    WorkLight->OuterConeAngle = 38.0f;
    WorkLight->SourceRadius = 4.0f;
    WorkLight->SoftSourceRadius = 7.0f;
    WorkLight->bUseInverseSquaredFalloff = true;
    WorkLight->VolumetricScatteringIntensity = 1.8f;
    WorkLight->CastShadows = true;
    WorkLight->SetVisibility(false);

    DeviceName = FText::FromString(TEXT("Power Device"));
}

void ARiftPowerDevice::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RecomputeTimer -= DeltaSeconds;
    if (Kind == ERiftPowerKind::Generator && bEnabled && RecomputeTimer <= 0.0f)
    {
        RecomputeTimer = 0.5f;
        RecomputeLinkedGrid(0.5f);
    }
    RefreshVisualState();
}

void ARiftPowerDevice::ConnectTo(ARiftPowerDevice* Other)
{
    if (!Other || Other == this)
    {
        return;
    }
    Links.AddUnique(Other);
    Other->Links.AddUnique(this);
    RecomputeLinkedGrid();
}

void ARiftPowerDevice::SetDeviceEnabled(bool bNewEnabled)
{
    bEnabled = bNewEnabled;
    if (!bEnabled && Kind == ERiftPowerKind::Consumer)
    {
        bPowered = false;
    }
    RecomputeLinkedGrid();
    RefreshVisualState();
}

void ARiftPowerDevice::GatherNetwork(TSet<ARiftPowerDevice*>& OutNetwork)
{
    TArray<ARiftPowerDevice*> Queue;
    Queue.Add(this);
    while (Queue.Num() > 0)
    {
        ARiftPowerDevice* Current = Queue.Pop(false);
        if (!Current || OutNetwork.Contains(Current))
        {
            continue;
        }
        OutNetwork.Add(Current);
        for (ARiftPowerDevice* Linked : Current->Links)
        {
            if (Linked && !OutNetwork.Contains(Linked))
            {
                Queue.Add(Linked);
            }
        }
    }
}

void ARiftPowerDevice::RecomputeLinkedGrid(float DeltaSeconds)
{
    TSet<ARiftPowerDevice*> Network;
    GatherNetwork(Network);
    if (Network.Num() == 0)
    {
        return;
    }

    float Generation = 0.0f;
    TArray<ARiftPowerDevice*> Batteries;
    TArray<ARiftPowerDevice*> Consumers;
    for (ARiftPowerDevice* Device : Network)
    {
        if (!Device)
        {
            continue;
        }
        if (Device->Kind == ERiftPowerKind::Generator && Device->bEnabled)
        {
            Generation += Device->GenerationKW;
            Device->bPowered = true;
        }
        else if (Device->Kind == ERiftPowerKind::Battery)
        {
            Batteries.Add(Device);
            Device->bPowered = Device->bEnabled;
        }
        else if (Device->Kind == ERiftPowerKind::Consumer)
        {
            Consumers.Add(Device);
            Device->bPowered = false;
        }
    }

    Consumers.Sort([](const ARiftPowerDevice& A, const ARiftPowerDevice& B)
    {
        return A.Priority < B.Priority;
    });

    const float Hours = FMath::Max(0.01f, DeltaSeconds) / 3600.0f;
    float RemainingGeneration = Generation;
    for (ARiftPowerDevice* Consumer : Consumers)
    {
        if (!Consumer || !Consumer->bEnabled)
        {
            continue;
        }

        float Need = Consumer->ConsumptionKW;
        const float FromGeneration = FMath::Min(RemainingGeneration, Need);
        RemainingGeneration -= FromGeneration;
        Need -= FromGeneration;

        if (Need > KINDA_SMALL_NUMBER)
        {
            for (ARiftPowerDevice* Battery : Batteries)
            {
                if (!Battery || !Battery->bEnabled || Battery->ChargeKWh <= 0.0f)
                {
                    continue;
                }
                const float EnergyNeed = Need * Hours;
                const float Draw = FMath::Min(EnergyNeed, Battery->ChargeKWh);
                Battery->ChargeKWh -= Draw;
                Need -= Draw / Hours;
                if (Need <= KINDA_SMALL_NUMBER)
                {
                    break;
                }
            }
        }
        Consumer->bPowered = Need <= KINDA_SMALL_NUMBER;
    }

    if (RemainingGeneration > 0.0f)
    {
        for (ARiftPowerDevice* Battery : Batteries)
        {
            if (!Battery || !Battery->bEnabled || Battery->ChargeKWh >= Battery->CapacityKWh)
            {
                continue;
            }
            const float Room = Battery->CapacityKWh - Battery->ChargeKWh;
            const float Charge = FMath::Min(Room, RemainingGeneration * Hours);
            Battery->ChargeKWh += Charge;
            RemainingGeneration -= Charge / Hours;
            if (RemainingGeneration <= 0.0f)
            {
                break;
            }
        }
    }

    for (ARiftPowerDevice* Device : Network)
    {
        if (Device)
        {
            Device->RefreshVisualState();
        }
    }
}

float ARiftPowerDevice::GetPowerSignatureStrength() const
{
    if (!bEnabled)
    {
        return 0.0f;
    }
    switch (Kind)
    {
        case ERiftPowerKind::Generator: return 5.0f + GenerationKW * 7.0f;
        case ERiftPowerKind::Consumer: return bPowered ? 8.0f + ConsumptionKW * 28.0f : 0.0f;
        case ERiftPowerKind::Battery: return ChargeKWh > 0.0f ? 1.0f : 0.0f;
        default: return 0.0f;
    }
}

void ARiftPowerDevice::RefreshVisualState()
{
    if (!StatusLight || !WorkLight)
    {
        return;
    }
    if (!bEnabled)
    {
        StatusLight->SetLightColor(FLinearColor(0.5f, 0.03f, 0.03f));
        StatusLight->SetIntensity(40.0f);
    }
    else if (Kind == ERiftPowerKind::Consumer && !bPowered)
    {
        StatusLight->SetLightColor(FLinearColor(0.8f, 0.32f, 0.03f));
        StatusLight->SetIntensity(80.0f);
    }
    else
    {
        StatusLight->SetLightColor(FLinearColor(0.08f, 0.8f, 0.3f));
        StatusLight->SetIntensity(130.0f);
    }
    WorkLight->SetVisibility(Kind == ERiftPowerKind::Consumer && bEnabled && bPowered, true);
}

FText ARiftPowerDevice::GetInteractionText_Implementation() const
{
    const FText State = bEnabled ? NSLOCTEXT("Riftworks", "PowerOn", "ON") : NSLOCTEXT("Riftworks", "PowerOff", "OFF");
    if (Kind == ERiftPowerKind::Generator)
    {
        return FText::Format(NSLOCTEXT("Riftworks", "GeneratorPrompt", "[E] {0}  {1}  |  {2} kW"), DeviceName, State, FText::AsNumber(GenerationKW));
    }
    if (Kind == ERiftPowerKind::Battery)
    {
        return FText::Format(NSLOCTEXT("Riftworks", "BatteryPrompt", "[E] {0}  {1}  |  {2}/{3} kWh"), DeviceName, State, FText::AsNumber(ChargeKWh), FText::AsNumber(CapacityKWh));
    }
    return FText::Format(NSLOCTEXT("Riftworks", "ConsumerPrompt", "[E] {0}  {1}  |  {2} kW  P{3}"), DeviceName, State, FText::AsNumber(ConsumptionKW), FText::AsNumber(Priority));
}

void ARiftPowerDevice::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    SetDeviceEnabled(!bEnabled);
}

ARiftBreachEmitter::ARiftBreachEmitter()
{
    PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmitterMesh"));
    SetRootComponent(Mesh);
    Mesh->SetStaticMesh(RiftGameplayPrivate::FindCylinder());
    Mesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.2f));
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));

    Field = CreateDefaultSubobject<USphereComponent>(TEXT("Field"));
    Field->SetupAttachment(Mesh);
    Field->SetSphereRadius(Radius);
    Field->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Field->SetCollisionResponseToAllChannels(ECR_Ignore);
    Field->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);

    CoreLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CoreLight"));
    CoreLight->SetupAttachment(Mesh);
    CoreLight->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
    CoreLight->IntensityUnits = ELightUnits::Lumens;
    CoreLight->Intensity = 2600.0f;
    CoreLight->AttenuationRadius = 1100.0f;
    CoreLight->LightColor = FColor(152, 116, 219);
    CoreLight->VolumetricScatteringIntensity = 2.4f;
    CoreLight->CastShadows = true;
}

void ARiftBreachEmitter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Field->SetSphereRadius(Radius);
    CoreLight->SetVisibility(bEnabled, true);
    if (!bEnabled)
    {
        return;
    }

    if (Mode == ERiftBreachMode::Luminance)
    {
        CoreLight->SetIntensity(14000.0f);
        CoreLight->SetAttenuationRadius(3000.0f);
        CoreLight->SetLightColor(FLinearColor(1.0f, 0.72f, 0.34f));
        return;
    }

    CoreLight->SetIntensity(2800.0f);
    TArray<UPrimitiveComponent*> Overlaps;
    Field->GetOverlappingComponents(Overlaps);
    for (UPrimitiveComponent* Primitive : Overlaps)
    {
        if (!Primitive || !Primitive->IsSimulatingPhysics())
        {
            continue;
        }
        const FVector Delta = Primitive->GetComponentLocation() - GetActorLocation();
        const float Distance = FMath::Max(80.0f, Delta.Size());
        const float Falloff = FMath::Square(FMath::Clamp(1.0f - Distance / Radius, 0.0f, 1.0f));
        if (Mode == ERiftBreachMode::Gravity)
        {
            const float GravityCounterForce = Primitive->GetMass() * 980.0f * 0.82f * Falloff;
            Primitive->AddForce(FVector::UpVector * GravityCounterForce);
        }
        else
        {
            FVector Direction = Delta.GetSafeNormal();
            if (Mode == ERiftBreachMode::Attraction)
            {
                Direction *= -1.0f;
            }
            Primitive->AddForce(Direction * ForceStrength * Falloff);
        }
    }
}

void ARiftBreachEmitter::SetSignal(bool bSignal)
{
    bEnabled = bSignal;
}

FText ARiftBreachEmitter::GetInteractionText_Implementation() const
{
    const UEnum* Enum = StaticEnum<ERiftBreachMode>();
    const FString ModeName = Enum ? Enum->GetNameStringByValue(static_cast<int64>(Mode)) : TEXT("Breach");
    return FText::FromString(FString::Printf(TEXT("[E] %s Field  |  %s"), *ModeName, bEnabled ? TEXT("ACTIVE") : TEXT("OFF")));
}

void ARiftBreachEmitter::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    SetSignal(!bEnabled);
}

ARiftAssemblyPart::ARiftAssemblyPart()
{
    PrimaryActorTick.bCanEverTick = true;
    PhysicsMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhysicsMesh"));
    SetRootComponent(PhysicsMesh);
    PhysicsMesh->SetStaticMesh(RiftGameplayPrivate::FindCube());
    PhysicsMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    PhysicsMesh->SetSimulatePhysics(true);
    PhysicsMesh->SetMassOverrideInKg(NAME_None, 18.0f, true);
}

void ARiftAssemblyPart::ConfigurePart()
{
    if (!PhysicsMesh)
    {
        return;
    }
    if (PartType == ERiftAssemblyPartType::Wheel || PartType == ERiftAssemblyPartType::MotorWheel)
    {
        PhysicsMesh->SetStaticMesh(RiftGameplayPrivate::FindCylinder());
        PhysicsMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
        PhysicsMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 0.35f));
        PhysicsMesh->SetMassOverrideInKg(NAME_None, 4.5f, true);
    }
    else if (PartType == ERiftAssemblyPartType::Beam)
    {
        PhysicsMesh->SetStaticMesh(RiftGameplayPrivate::FindCube());
        PhysicsMesh->SetRelativeRotation(FRotator::ZeroRotator);
        PhysicsMesh->SetRelativeScale3D(FVector(0.32f, 0.32f, 3.0f));
        PhysicsMesh->SetMassOverrideInKg(NAME_None, 5.0f, true);
    }
    else
    {
        PhysicsMesh->SetStaticMesh(RiftGameplayPrivate::FindCube());
        PhysicsMesh->SetRelativeRotation(FRotator::ZeroRotator);
        PhysicsMesh->SetRelativeScale3D(FVector(2.6f, 2.2f, 0.28f));
        PhysicsMesh->SetMassOverrideInKg(NAME_None, 18.0f, true);
    }
}

void ARiftAssemblyPart::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (PartType == ERiftAssemblyPartType::MotorWheel && bMotorEnabled && PhysicsMesh->IsSimulatingPhysics())
    {
        PhysicsMesh->AddTorqueInRadians(GetActorRightVector() * MotorTorque * DeltaSeconds, NAME_None, true);
    }
}

void ARiftAssemblyPart::SetSignal(bool bSignal)
{
    bMotorEnabled = bSignal;
}

FText ARiftAssemblyPart::GetInteractionText_Implementation() const
{
    const UEnum* Enum = StaticEnum<ERiftAssemblyPartType>();
    const FString TypeName = Enum ? Enum->GetNameStringByValue(static_cast<int64>(PartType)) : TEXT("Part");
    return FText::FromString(FString::Printf(TEXT("[E] %s  |  physics assembly part"), *TypeName));
}

void ARiftAssemblyPart::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (PartType == ERiftAssemblyPartType::MotorWheel)
    {
        SetSignal(!bMotorEnabled);
    }
}

ARiftColossus::ARiftColossus()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(510.0f, 1700.0f);
    GetMesh()->SetVisibility(false);
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = false;

    UStaticMesh* Cube = RiftGameplayPrivate::FindCube();
    const TCHAR* Names[] = { TEXT("LeftLeg"), TEXT("RightLeg"), TEXT("Torso"), TEXT("LeftArm"), TEXT("RightArm"), TEXT("Head") };
    const FVector Locations[] = {
        FVector(-340.0f, 0.0f, 650.0f), FVector(340.0f, 0.0f, 650.0f), FVector(0.0f, 0.0f, 2250.0f),
        FVector(-710.0f, 0.0f, 2300.0f), FVector(710.0f, 0.0f, 2300.0f), FVector(0.0f, -30.0f, 3050.0f)
    };
    const FVector Scales[] = {
        FVector(3.1f, 3.1f, 13.0f), FVector(3.1f, 3.1f, 13.0f), FVector(12.5f, 7.2f, 9.0f),
        FVector(2.4f, 2.6f, 11.0f), FVector(2.4f, 2.6f, 11.0f), FVector(6.6f, 5.4f, 5.0f)
    };
    for (int32 Index = 0; Index < 6; ++Index)
    {
        UStaticMeshComponent* Block = CreateDefaultSubobject<UStaticMeshComponent>(Names[Index]);
        Block->SetupAttachment(GetCapsuleComponent());
        Block->SetStaticMesh(Cube);
        Block->SetRelativeLocation(Locations[Index]);
        Block->SetRelativeScale3D(Scales[Index]);
        Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BodyBlocks.Add(Block);
    }
}

void ARiftColossus::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead)
    {
        return;
    }
    if (RouteCenter.IsNearlyZero())
    {
        RouteCenter = GetActorLocation();
    }
    RouteAngle += DeltaSeconds * 0.018f;
    const FVector Desired = RouteCenter + FVector(FMath::Cos(RouteAngle), FMath::Sin(RouteAngle), 0.0f) * RouteRadius;
    const FVector Direction = (Desired - GetActorLocation()).GetSafeNormal2D();
    GetCharacterMovement()->Velocity = FVector(Direction.X * MoveSpeed, Direction.Y * MoveSpeed, GetCharacterMovement()->Velocity.Z);
    if (!Direction.IsNearlyZero())
    {
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), Direction.Rotation(), DeltaSeconds, 0.9f));
    }
}

float ARiftColossus::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead)
    {
        return 0.0f;
    }
    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent* PointEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
        const FVector LocalHit = GetActorTransform().InverseTransformPosition(PointEvent->HitInfo.ImpactPoint);
        if (LocalHit.Z >= 2600.0f)
        {
            DamageWeakpoint(TEXT("head"), DamageAmount);
        }
        else if (LocalHit.Z >= 1700.0f)
        {
            DamageWeakpoint(TEXT("torso"), DamageAmount);
        }
        else if (LocalHit.Z <= 1050.0f)
        {
            DamageWeakpoint(TEXT("legs"), DamageAmount);
        }
    }
    return DamageAmount;
}

void ARiftColossus::DamageWeakpoint(FName Weakpoint, float DamageAmount)
{
    if (Weakpoint == TEXT("head") && !bHeadDestroyed)
    {
        HeadHP -= DamageAmount;
        if (HeadHP <= 0.0f)
        {
            bHeadDestroyed = true;
            BP_OnWeakpointDestroyed(Weakpoint);
        }
    }
    else if (Weakpoint == TEXT("torso") && !bTorsoDestroyed)
    {
        TorsoHP -= DamageAmount;
        if (TorsoHP <= 0.0f)
        {
            bTorsoDestroyed = true;
            BP_OnWeakpointDestroyed(Weakpoint);
        }
    }
    else if (Weakpoint == TEXT("legs") && !bLegsDestroyed)
    {
        LegsHP -= DamageAmount;
        if (LegsHP <= 0.0f)
        {
            bLegsDestroyed = true;
            MoveSpeed *= 0.34f;
            BP_OnWeakpointDestroyed(Weakpoint);
        }
    }
    if (bHeadDestroyed && bTorsoDestroyed && bLegsDestroyed)
    {
        Die();
    }
}

void ARiftColossus::RegisterHarpoon()
{
    ++HarpoonCount;
    MoveSpeed *= 0.82f;
    if (HarpoonCount >= 3 && !bLegsDestroyed)
    {
        DamageWeakpoint(TEXT("legs"), 8.0f);
    }
}

void ARiftColossus::Die()
{
    bDead = true;
    GetCharacterMovement()->DisableMovement();
    BP_OnColossusKilled();

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (ARiftSalvageActor* Core = GetWorld()->SpawnActor<ARiftSalvageActor>(ARiftSalvageActor::StaticClass(), GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, Params))
    {
        Core->ItemId = TEXT("bioelectric_core");
        Core->DisplayName = FText::FromString(TEXT("Walker Bioelectric Core"));
        Core->Amount = 2;
        Core->bHeavy = true;
        Core->MassKg = 42.0f;
        Core->SetCarriedState(false);
    }
    if (ARiftSalvageActor* Plate = GetWorld()->SpawnActor<ARiftSalvageActor>(ARiftSalvageActor::StaticClass(), GetActorLocation() + FVector(220.0f, 120.0f, 100.0f), FRotator::ZeroRotator, Params))
    {
        Plate->ItemId = TEXT("carapace");
        Plate->DisplayName = FText::FromString(TEXT("Colossus Carapace"));
        Plate->Amount = 1;
        Plate->bHeavy = true;
        Plate->MassKg = 68.0f;
        Plate->SetCarriedState(false);
    }
    SetLifeSpan(12.0f);
}

ARiftHarpoonAnchor::ARiftHarpoonAnchor()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HarpoonAnchor"));
    SetRootComponent(Mesh);
    Mesh->SetStaticMesh(RiftGameplayPrivate::FindCylinder());
    Mesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.1f));
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ARiftHarpoonAnchor::FireHarpoon()
{
    if (AttachedColossus || !GetWorld())
    {
        return;
    }
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 120.0f);
    const FVector End = Start + GetActorForwardVector() * Range;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftHarpoon), true, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (ARiftColossus* Colossus = Cast<ARiftColossus>(Hit.GetActor()))
        {
            AttachedColossus = Colossus;
            Colossus->RegisterHarpoon();
        }
    }
}

void ARiftHarpoonAnchor::SetSignal(bool bSignal)
{
    if (bSignal)
    {
        FireHarpoon();
    }
}

FText ARiftHarpoonAnchor::GetInteractionText_Implementation() const
{
    return AttachedColossus ? NSLOCTEXT("Riftworks", "HarpoonAttached", "Harpoon attached to Colossus") : NSLOCTEXT("Riftworks", "HarpoonReady", "[E] Fire Harpoon Anchor");
}

void ARiftHarpoonAnchor::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    FireHarpoon();
}
