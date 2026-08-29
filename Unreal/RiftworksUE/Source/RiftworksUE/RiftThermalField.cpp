#include "RiftThermalField.h"

#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ARiftTemperatureField::ARiftTemperatureField()
{
    PrimaryActorTick.bCanEverTick = true;

    CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TemperatureCore"));
    SetRootComponent(CoreMesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded())
    {
        CoreMesh->SetStaticMesh(Sphere.Object);
    }
    CoreMesh->SetRelativeScale3D(FVector(0.42f));
    CoreMesh->SetCollisionProfileName(TEXT("BlockAll"));

    Field = CreateDefaultSubobject<USphereComponent>(TEXT("TemperatureField"));
    Field->SetupAttachment(CoreMesh);
    Field->SetSphereRadius(Radius);
    Field->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Field->SetCollisionResponseToAllChannels(ECR_Overlap);
    Field->SetGenerateOverlapEvents(true);

    CoreLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TemperatureLight"));
    CoreLight->SetupAttachment(CoreMesh);
    CoreLight->IntensityUnits = ELightUnits::Lumens;
    CoreLight->SetIntensity(2100.0f);
    CoreLight->SetAttenuationRadius(Radius * 1.15f);
    CoreLight->SetSourceRadius(18.0f);
    CoreLight->SetVolumetricScatteringIntensity(0.18f);
    CoreLight->CastShadows = true;
}

void ARiftTemperatureField::BeginPlay()
{
    Super::BeginPlay();
    if (Field)
    {
        Field->SetSphereRadius(Radius);
    }
    RefreshVisuals();
}

void ARiftTemperatureField::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (const TPair<TWeakObjectPtr<ACharacter>, float>& Pair : CryoOriginalSpeeds)
    {
        if (ACharacter* Character = Pair.Key.Get())
        {
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            {
                Movement->MaxWalkSpeed = Pair.Value;
            }
        }
    }
    CryoOriginalSpeeds.Reset();
    Super::EndPlay(EndPlayReason);
}

void ARiftTemperatureField::RefreshVisuals()
{
    if (!CoreLight)
    {
        return;
    }

    if (!bEnabled)
    {
        CoreLight->SetIntensity(0.0f);
        return;
    }

    if (Mode == ERiftTemperatureFieldMode::Thermal)
    {
        CoreLight->SetLightColor(FLinearColor(1.0f, 0.20f, 0.035f));
        CoreLight->SetIntensity(2350.0f);
    }
    else
    {
        CoreLight->SetLightColor(FLinearColor(0.08f, 0.62f, 1.0f));
        CoreLight->SetIntensity(1900.0f);
    }
    CoreLight->SetAttenuationRadius(Radius * 1.15f);
}

void ARiftTemperatureField::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bEnabled || !Field)
    {
        RestoreReleasedCryoCharacters({});
        return;
    }

    TArray<AActor*> Overlaps;
    Field->GetOverlappingActors(Overlaps);
    if (Mode == ERiftTemperatureFieldMode::Thermal)
    {
        RestoreReleasedCryoCharacters({});
        ApplyThermal(DeltaSeconds, Overlaps);
    }
    else
    {
        ApplyCryo(DeltaSeconds, Overlaps);
    }
}

void ARiftTemperatureField::ApplyThermal(float DeltaSeconds, const TArray<AActor*>& Overlaps)
{
    const FVector Origin = GetActorLocation();
    for (AActor* Actor : Overlaps)
    {
        if (!Actor || Actor == this)
        {
            continue;
        }

        if (ACharacter* Character = Cast<ACharacter>(Actor))
        {
            UGameplayStatics::ApplyDamage(
                Character,
                ThermalDamagePerSecond * DeltaSeconds,
                nullptr,
                this,
                nullptr);
        }

        TArray<UPrimitiveComponent*> Primitives;
        Actor->GetComponents<UPrimitiveComponent>(Primitives);
        for (UPrimitiveComponent* Primitive : Primitives)
        {
            if (!Primitive || !Primitive->IsSimulatingPhysics())
            {
                continue;
            }

            FVector Direction = Primitive->GetComponentLocation() - Origin;
            Direction.Z = FMath::Max(Direction.Z, 180.0f);
            Direction = Direction.GetSafeNormal();
            const float Mass = FMath::Max(1.0f, Primitive->GetMass());
            Primitive->AddForce(Direction * ThermalLiftForce * Mass * 0.035f);
        }
    }
}

void ARiftTemperatureField::ApplyCryo(float DeltaSeconds, const TArray<AActor*>& Overlaps)
{
    TSet<TWeakObjectPtr<ACharacter>> CurrentlyAffected;

    for (AActor* Actor : Overlaps)
    {
        if (!Actor || Actor == this)
        {
            continue;
        }

        if (ACharacter* Character = Cast<ACharacter>(Actor))
        {
            CurrentlyAffected.Add(Character);
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            {
                if (!CryoOriginalSpeeds.Contains(Character))
                {
                    CryoOriginalSpeeds.Add(Character, Movement->MaxWalkSpeed);
                }
                Movement->MaxWalkSpeed = FMath::Min(Movement->MaxWalkSpeed, CryoCharacterSpeed);
            }
        }

        TArray<UPrimitiveComponent*> Primitives;
        Actor->GetComponents<UPrimitiveComponent>(Primitives);
        for (UPrimitiveComponent* Primitive : Primitives)
        {
            if (!Primitive || !Primitive->IsSimulatingPhysics())
            {
                continue;
            }
            const FVector Velocity = Primitive->GetPhysicsLinearVelocity();
            const float Mass = FMath::Max(1.0f, Primitive->GetMass());
            Primitive->AddForce(-Velocity * CryoDragForce * Mass * 0.0008f * FMath::Max(DeltaSeconds, 0.001f));
            Primitive->AddTorqueInRadians(-Primitive->GetPhysicsAngularVelocityInRadians() * CryoDragForce * 0.018f * DeltaSeconds);
        }
    }

    RestoreReleasedCryoCharacters(CurrentlyAffected);
}

void ARiftTemperatureField::RestoreReleasedCryoCharacters(const TSet<TWeakObjectPtr<ACharacter>>& CurrentlyAffected)
{
    TArray<TWeakObjectPtr<ACharacter>> RemoveKeys;
    for (const TPair<TWeakObjectPtr<ACharacter>, float>& Pair : CryoOriginalSpeeds)
    {
        if (CurrentlyAffected.Contains(Pair.Key))
        {
            continue;
        }
        if (ACharacter* Character = Pair.Key.Get())
        {
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            {
                Movement->MaxWalkSpeed = Pair.Value;
            }
        }
        RemoveKeys.Add(Pair.Key);
    }
    for (const TWeakObjectPtr<ACharacter>& Key : RemoveKeys)
    {
        CryoOriginalSpeeds.Remove(Key);
    }
}

void ARiftTemperatureField::SetSignal(bool bSignal)
{
    bEnabled = bSignal;
    RefreshVisuals();
}

FText ARiftTemperatureField::GetInteractionText_Implementation() const
{
    const TCHAR* ModeName = Mode == ERiftTemperatureFieldMode::Thermal ? TEXT("THERMAL") : TEXT("CRYO");
    return FText::FromString(FString::Printf(
        TEXT("[E] %s Breach Field | %s | signal-compatible"),
        ModeName,
        bEnabled ? TEXT("ACTIVE") : TEXT("OFF")));
}

void ARiftTemperatureField::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    SetSignal(!bEnabled);
}
