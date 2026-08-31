#include "RiftAnimatedHumanoidNPC.h"

#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

ARiftAnimatedHumanoidNPC::ARiftAnimatedHumanoidNPC()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 92.0f);
}

void ARiftAnimatedHumanoidNPC::BeginPlay()
{
    Super::BeginPlay();
    NormalizeVisualToCapsule();
    GroundCapsuleToWorld();
    CurrentFallbackAnimation = nullptr;
    UpdateFallbackAnimation();
}

void ARiftAnimatedHumanoidNPC::NormalizeVisualToCapsule()
{
    USkeletalMeshComponent* MeshComponent = GetMesh();
    if (!MeshComponent)
    {
        return;
    }

    MeshComponent->SetVisibility(true, true);
    MeshComponent->SetOwnerNoSee(false);
    MeshComponent->SetRelativeScale3D(FVector(1.0f));
    MeshComponent->SetRelativeRotation(FRotator::ZeroRotator);

    float RelativeZ = -GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    if (const USkeletalMesh* MeshAsset = MeshComponent->GetSkeletalMeshAsset())
    {
        const FBoxSphereBounds Bounds = MeshAsset->GetBounds();
        const float LocalBottom = Bounds.Origin.Z - Bounds.BoxExtent.Z;
        RelativeZ = -GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - LocalBottom;
    }

    MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, RelativeZ));
    MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void ARiftAnimatedHumanoidNPC::GroundCapsuleToWorld()
{
    if (!GetWorld() || !GetCapsuleComponent())
    {
        return;
    }

    const FVector Current = GetActorLocation();
    const FVector Start = Current + FVector(0.0f, 0.0f, 450.0f);
    const FVector End = Current - FVector(0.0f, 0.0f, 1200.0f);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftHumanoidGround), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        SetActorLocation(
            FVector(Current.X, Current.Y, Hit.ImpactPoint.Z + HalfHeight + 1.0f),
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
    }
}

void ARiftAnimatedHumanoidNPC::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDead)
    {
        return;
    }

    // The Colossus path proved reliable because the visual is normalized against
    // the capsule and the animation is re-evaluated from actual velocity. Keep
    // the human enemies on that same invariant instead of trusting stale BP pose state.
    UpdateFallbackAnimation();

    GroundAuditTimer -= DeltaSeconds;
    if (GroundAuditTimer <= 0.0f)
    {
        GroundAuditTimer = 0.75f;
        UCharacterMovementComponent* Movement = GetCharacterMovement();
        if (Movement && Movement->IsMovingOnGround())
        {
            GroundCapsuleToWorld();
        }
    }
}
