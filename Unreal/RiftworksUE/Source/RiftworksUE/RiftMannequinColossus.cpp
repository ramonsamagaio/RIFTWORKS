#include "RiftMannequinColossus.h"

#include "RiftEngineeringJoint.h"
#include "AIController.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

ARiftMannequinColossus::ARiftMannequinColossus()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    GetCapsuleComponent()->InitCapsuleSize(520.0f, 1750.0f);
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;

    // Weakpoints now occupy the same local vertical band as the capsule/body.
    // Previous values extended far above the actual visual mesh.
    LegsCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("LegsWeakpoint"));
    LegsCollision->SetupAttachment(GetCapsuleComponent());
    LegsCollision->SetBoxExtent(FVector(430.0f, 360.0f, 700.0f));
    LegsCollision->SetRelativeLocation(FVector(0.0f, 0.0f, -900.0f));
    LegsCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    LegsCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    LegsCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    LegsCollision->SetHiddenInGame(true);

    TorsoCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("TorsoWeakpoint"));
    TorsoCollision->SetupAttachment(GetCapsuleComponent());
    TorsoCollision->SetBoxExtent(FVector(620.0f, 430.0f, 540.0f));
    TorsoCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 280.0f));
    TorsoCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TorsoCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    TorsoCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TorsoCollision->SetHiddenInGame(true);

    HeadCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HeadWeakpoint"));
    HeadCollision->SetupAttachment(GetCapsuleComponent());
    HeadCollision->SetBoxExtent(FVector(360.0f, 320.0f, 320.0f));
    HeadCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 1160.0f));
    HeadCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HeadCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    HeadCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    HeadCollision->SetHiddenInGame(true);
}

void ARiftMannequinColossus::BeginPlay()
{
    Super::BeginPlay();

    for (UStaticMeshComponent* Block : BodyBlocks)
    {
        if (Block)
        {
            Block->SetVisibility(false, true);
            Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    NormalizeVisualToCapsule();
    GroundCapsuleToWorld();

    PrototypeRouteCenter = GetActorLocation();
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    EnvironmentPulseTimer = FMath::FRandRange(0.0f, EnvironmentPulseInterval);
    UpdatePrototypeAnimation();
}

void ARiftMannequinColossus::NormalizeVisualToCapsule()
{
    if (!GetMesh())
    {
        return;
    }

    GetMesh()->SetVisibility(true, true);
    GetMesh()->SetOwnerNoSee(false);
    GetMesh()->SetRelativeScale3D(FVector(VisualScale));
    GetMesh()->SetRelativeRotation(FRotator::ZeroRotator);

    float RelativeZ = -GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    if (const USkeletalMesh* MeshAsset = GetMesh()->GetSkeletalMeshAsset())
    {
        const FBoxSphereBounds Bounds = MeshAsset->GetBounds();
        const float LocalBottom = Bounds.Origin.Z - Bounds.BoxExtent.Z;
        RelativeZ = -GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - LocalBottom * VisualScale;
    }
    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, RelativeZ));
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void ARiftMannequinColossus::GroundCapsuleToWorld()
{
    if (!GetWorld() || !GetCapsuleComponent())
    {
        return;
    }

    const FVector Current = GetActorLocation();
    const FVector Start = Current + FVector(0.0f, 0.0f, 4500.0f);
    const FVector End = Current - FVector(0.0f, 0.0f, 5000.0f);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftColossusGround), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        SetActorLocation(FVector(Current.X, Current.Y, Hit.ImpactPoint.Z + HalfHeight + 2.0f), false, nullptr, ETeleportType::TeleportPhysics);
    }
}

void ARiftMannequinColossus::Tick(float DeltaSeconds)
{
    ACharacter::Tick(DeltaSeconds);

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (!Movement || Movement->MovementMode == MOVE_None)
    {
        return;
    }

    PrototypeRouteAngle += DeltaSeconds * 0.075f;
    const FVector Desired = PrototypeRouteCenter + FVector(FMath::Cos(PrototypeRouteAngle), FMath::Sin(PrototypeRouteAngle), 0.0f) * RouteRadius;
    const FVector Direction = (Desired - GetActorLocation()).GetSafeNormal2D();

    if (!Direction.IsNearlyZero())
    {
        Movement->Velocity = FVector(Direction.X * MoveSpeed, Direction.Y * MoveSpeed, Movement->Velocity.Z);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), Direction.Rotation(), DeltaSeconds, 1.4f));
    }

    EnvironmentPulseTimer -= DeltaSeconds;
    if (EnvironmentPulseTimer <= 0.0f)
    {
        EnvironmentPulseTimer = FMath::Max(0.05f, EnvironmentPulseInterval);
        PulseEnvironment();
    }

    UpdatePrototypeAnimation();
}

void ARiftMannequinColossus::PulseEnvironment()
{
    if (!GetWorld())
    {
        return;
    }

    FCollisionObjectQueryParams ObjectQuery;
    ObjectQuery.AddObjectTypesToQuery(ECC_PhysicsBody);
    ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RiftColossusEnvironment), false, this);
    TArray<FOverlapResult> Results;
    const FVector Origin = GetActorLocation() + GetActorForwardVector() * 250.0f - FVector(0.0f, 0.0f, 1250.0f);

    if (!GetWorld()->OverlapMultiByObjectType(
        Results,
        Origin,
        FQuat::Identity,
        ObjectQuery,
        FCollisionShape::MakeSphere(EnvironmentInteractionRadius),
        QueryParams))
    {
        return;
    }

    TSet<AActor*> ProcessedActors;
    for (const FOverlapResult& Result : Results)
    {
        UPrimitiveComponent* Primitive = Result.GetComponent();
        AActor* Actor = Result.GetActor();
        if (!Actor || Actor == this || Actor->IsA<APawn>())
        {
            continue;
        }

        const float Distance = FVector::Dist2D(Actor->GetActorLocation(), Origin);

        if (Primitive && Primitive->IsSimulatingPhysics())
        {
            Primitive->AddRadialImpulse(
                Origin,
                EnvironmentInteractionRadius,
                EnvironmentImpulse,
                ERadialImpulseFalloff::RIF_Linear,
                true);
        }

        if (ARiftAssemblyPart* Part = Cast<ARiftAssemblyPart>(Actor))
        {
            if (Part->PhysicsMesh && Distance <= EnvironmentBreakRadius)
            {
                if (!Part->PhysicsMesh->IsSimulatingPhysics())
                {
                    Part->PhysicsMesh->SetSimulatePhysics(true);
                }
                Part->PhysicsMesh->AddRadialImpulse(
                    Origin,
                    EnvironmentInteractionRadius,
                    EnvironmentImpulse * 1.25f,
                    ERadialImpulseFalloff::RIF_Linear,
                    true);
            }
        }
        else if (ARiftEngineeringJoint* Joint = Cast<ARiftEngineeringJoint>(Actor))
        {
            if (Distance <= EnvironmentBreakRadius)
            {
                Joint->Destroy();
            }
        }
        else if (ARiftPowerDevice* Device = Cast<ARiftPowerDevice>(Actor))
        {
            if (Distance <= EnvironmentBreakRadius)
            {
                Device->SetDeviceEnabled(false);
            }
        }
        else if (Distance <= EnvironmentBreakRadius && Actor->ActorHasTag(TEXT("RiftFragile")))
        {
            Actor->Destroy();
        }

        if (!ProcessedActors.Contains(Actor))
        {
            ProcessedActors.Add(Actor);
            BP_OnEnvironmentImpact(Actor);
        }
    }
}

bool ARiftMannequinColossus::IsAnimationCompatible(const UAnimSequenceBase* Animation) const
{
    if (!Animation || !GetMesh())
    {
        return false;
    }
    const USkeletalMesh* MeshAsset = GetMesh()->GetSkeletalMeshAsset();
    return MeshAsset && MeshAsset->GetSkeleton() && Animation->GetSkeleton() == MeshAsset->GetSkeleton();
}

void ARiftMannequinColossus::UpdatePrototypeAnimation()
{
    if (!GetMesh())
    {
        return;
    }

    UAnimSequenceBase* Desired = GetVelocity().Size2D() > 8.0f ? WalkAnimation.Get() : IdleAnimation.Get();
    if (!IsAnimationCompatible(Desired))
    {
        if (IsAnimationCompatible(WalkAnimation))
        {
            Desired = WalkAnimation.Get();
        }
        else if (IsAnimationCompatible(IdleAnimation))
        {
            Desired = IdleAnimation.Get();
        }
        else
        {
            return;
        }
    }

    if (Desired != CurrentAnimation)
    {
        CurrentAnimation = Desired;
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->PlayAnimation(Desired, true);
    }
}
