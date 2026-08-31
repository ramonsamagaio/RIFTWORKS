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
    GetCharacterMovement()->GravityScale = 1.0f;
    GetCharacterMovement()->bAlwaysCheckFloor = true;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
}

void ARiftAnimatedHumanoidNPC::BeginPlay()
{
    Super::BeginPlay();

    // BP_RiftAnimatedHumanoid is authored by riftworks_runtime_fixes from the
    // exact runtime_character_assets() mesh + animation family used by the
    // working Colossus pipeline. Never overwrite those references here with a
    // guessed content path: imported GLB asset layouts can differ between UE
    // versions and a failed LoadObject used to replace valid clips with null.
    LoadColossusAnimationFamily();
    NormalizeVisualToCapsule();
    GroundCapsuleToWorld();

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->SetMovementMode(MOVE_Walking);
        Movement->MaxWalkSpeed = WalkSpeed;
        Movement->bRunPhysicsWithNoController = true;
        Movement->bAlwaysCheckFloor = true;
    }

    CurrentFallbackAnimation = nullptr;
    RuntimeAnimation = nullptr;
    UpdateColossusStyleAnimation();
}

void ARiftAnimatedHumanoidNPC::LoadColossusAnimationFamily()
{
    USkeletalMeshComponent* MeshComponent = GetMesh();
    if (!MeshComponent)
    {
        return;
    }

    // Same proven strategy as the mannequin Colossus: a Skeleton-native mesh
    // with AnimationSingleNode playback. The compatible assets themselves are
    // selected by the editor bootstrap, which can inspect the assets Unreal
    // actually imported instead of guessing package names in native code.
    bUseSingleNodeAnimationFallback = true;
    MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void ARiftAnimatedHumanoidNPC::NormalizeVisualToCapsule()
{
    USkeletalMeshComponent* MeshComponent = GetMesh();
    if (!MeshComponent || !GetCapsuleComponent())
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

    // Sink only two centimeters into the contact plane. This hides tiny FBX/GLB
    // sole offsets without changing the mathematically grounded capsule.
    MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, RelativeZ - 2.0f));
    MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void ARiftAnimatedHumanoidNPC::GroundCapsuleToWorld()
{
    if (!GetWorld() || !GetCapsuleComponent())
    {
        return;
    }

    const FVector Current = GetActorLocation();

    // Start at capsule level, never hundreds of centimeters above the NPC.
    // Indoors the old high trace could hit a ceiling/awning first and teleport
    // the character onto it, which visually read as floating above the floor.
    const FVector Start = Current + FVector(0.0f, 0.0f, 8.0f);
    const FVector End = Current - FVector(0.0f, 0.0f, 2200.0f);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftHumanoidGround), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        SetActorLocation(
            FVector(Current.X, Current.Y, Hit.ImpactPoint.Z + HalfHeight + 0.5f),
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
    }
}

bool ARiftAnimatedHumanoidNPC::IsRuntimeAnimationCompatible(const UAnimSequenceBase* Animation) const
{
    if (!Animation || !GetMesh())
    {
        return false;
    }

    const USkeletalMesh* MeshAsset = GetMesh()->GetSkeletalMeshAsset();
    return MeshAsset && MeshAsset->GetSkeleton() && Animation->GetSkeleton() == MeshAsset->GetSkeleton();
}

void ARiftAnimatedHumanoidNPC::UpdateColossusStyleAnimation()
{
    if (!GetMesh() || bDead || bAttackAnimationLocked)
    {
        return;
    }

    const float Speed = GetVelocity().Size2D();
    UAnimSequenceBase* Desired = nullptr;
    if (Speed < 10.0f)
    {
        Desired = bAlerted && IsRuntimeAnimationCompatible(PistolIdleAnimation) ? PistolIdleAnimation.Get() : IdleAnimation.Get();
    }
    else if (Speed < 250.0f)
    {
        Desired = WalkAnimation.Get();
    }
    else
    {
        Desired = RunAnimation.Get();
    }

    if (!IsRuntimeAnimationCompatible(Desired))
    {
        UAnimSequenceBase* Candidates[] = {
            WalkAnimation.Get(), IdleAnimation.Get(), RunAnimation.Get(), PistolIdleAnimation.Get()
        };
        Desired = nullptr;
        for (UAnimSequenceBase* Candidate : Candidates)
        {
            if (IsRuntimeAnimationCompatible(Candidate))
            {
                Desired = Candidate;
                break;
            }
        }
    }

    if (Desired && Desired != RuntimeAnimation)
    {
        RuntimeAnimation = Desired;
        CurrentFallbackAnimation = Desired;
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->PlayAnimation(Desired, true);
    }
}

void ARiftAnimatedHumanoidNPC::DriveDeterministicMovement(float DeltaSeconds)
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (!Movement || Movement->MovementMode == MOVE_None || Movement->IsFalling())
    {
        return;
    }

    FVector DesiredDirection = FVector::ZeroVector;
    float DesiredSpeed = WalkSpeed * 0.62f;

    if (PlayerTarget && bAlerted)
    {
        const FVector ToPlayerVector = PlayerTarget->GetActorLocation() - GetActorLocation();
        const float Distance = ToPlayerVector.Size2D();
        const FVector ToPlayer = ToPlayerVector.GetSafeNormal2D();

        if (Distance > 1450.0f)
        {
            DesiredDirection = ToPlayer;
            DesiredSpeed = CombatSpeed;
        }
        else if (Distance < 650.0f)
        {
            DesiredDirection = -ToPlayer;
            DesiredSpeed = CombatSpeed * 0.72f;
        }
        else
        {
            // Never become a firing-range mannequin. Inside weapon range the
            // enemy circles the player while keeping a little forward pressure.
            const FVector Tangent(-ToPlayer.Y, ToPlayer.X, 0.0f);
            const float PhaseSeed = static_cast<float>(GetUniqueID() % 17) * 0.37f;
            const float Side = FMath::Sin(GetWorld()->GetTimeSeconds() * 0.85f + PhaseSeed) >= 0.0f ? 1.0f : -1.0f;
            DesiredDirection = (Tangent * Side + ToPlayer * 0.18f).GetSafeNormal2D();
            DesiredSpeed = CombatSpeed * 0.68f;
        }

        if (!ToPlayer.IsNearlyZero())
        {
            const FRotator Look = ToPlayer.Rotation();
            SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.0f, Look.Yaw, 0.0f), DeltaSeconds, 8.0f));
        }
    }
    else if (!PatrolTarget.IsNearlyZero())
    {
        DesiredDirection = (PatrolTarget - GetActorLocation()).GetSafeNormal2D();
        DesiredSpeed = WalkSpeed * 0.62f;
    }

    if (DesiredDirection.IsNearlyZero())
    {
        return;
    }

    Movement->MaxWalkSpeed = DesiredSpeed;
    AddMovementInput(DesiredDirection, 1.0f, true);

    // NavMesh is still preferred by the base AI when it exists, but movement
    // must not freeze when Recast is absent or a request stalls. CharacterMovement
    // owns collision and floor response; we only provide deterministic planar
    // velocity when the requested movement produced effectively no motion.
    if (Movement->Velocity.Size2D() < 18.0f)
    {
        Movement->Velocity = FVector(
            DesiredDirection.X * DesiredSpeed,
            DesiredDirection.Y * DesiredSpeed,
            Movement->Velocity.Z);
    }
}

float ARiftAnimatedHumanoidNPC::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    const bool bWasDead = bDead;
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (Applied <= 0.0f)
    {
        return Applied;
    }

    FVector Away = GetActorForwardVector() * -1.0f;
    if (DamageCauser)
    {
        Away = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D();
    }

    if (!bDead)
    {
        if (UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
            Movement->Velocity += Away * 135.0f;
        }
    }
    else if (!bWasDead && GetMesh() && GetMesh()->GetPhysicsAsset())
    {
        // A successful kill must be visually unmistakable. Use a ragdoll when
        // the imported runtime mesh has a physics asset; otherwise the base class
        // keeps the compatible UAL1 death animation as the fallback.
        GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
        GetMesh()->SetAllBodiesSimulatePhysics(true);
        GetMesh()->SetSimulatePhysics(true);
        GetMesh()->WakeAllRigidBodies();
        GetMesh()->AddImpulse(Away * 9000.0f, NAME_None, true);
    }

    return Applied;
}

void ARiftAnimatedHumanoidNPC::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDead)
    {
        return;
    }

    // The base class can use Recast when available. This layer guarantees visible
    // locomotion even when navigation is missing/stalled, then chooses animation
    // from the velocity that actually happened.
    DriveDeterministicMovement(DeltaSeconds);
    UpdateColossusStyleAnimation();
}
