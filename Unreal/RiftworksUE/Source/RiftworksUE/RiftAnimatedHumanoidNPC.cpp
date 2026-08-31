#include "RiftAnimatedHumanoidNPC.h"

#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/UObjectGlobals.h"

namespace
{
    static const TCHAR* UALRoot = TEXT("/Game/Riftworks/Animations/UAL1_Standard/SkeletalMeshes/");

    template <typename T>
    T* LoadUALAsset(const TCHAR* AssetName)
    {
        const FString Path = FString::Printf(TEXT("%s%s.%s"), UALRoot, AssetName, AssetName);
        return LoadObject<T>(nullptr, *Path);
    }
}

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

    // Do not trust stale Blueprint references here. The Colossus is known to work
    // with this exact UAL1 mesh/animation family, so human enemies use the same
    // native asset family and the same AnimationSingleNode strategy.
    LoadColossusAnimationFamily();
    NormalizeVisualToCapsule();
    GroundCapsuleToWorld();

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->SetMovementMode(MOVE_Walking);
        Movement->MaxWalkSpeed = WalkSpeed;
    }

    CurrentFallbackAnimation = nullptr;
    RuntimeAnimation = nullptr;
    UpdateColossusStyleAnimation();
}

void ARiftAnimatedHumanoidNPC::LoadColossusAnimationFamily()
{
    USkeletalMeshComponent* MeshComponent = GetMesh();
    USkeletalMesh* RuntimeMesh = LoadUALAsset<USkeletalMesh>(TEXT("UAL1_Standard"));
    if (MeshComponent && RuntimeMesh)
    {
        MeshComponent->SetSkeletalMesh(RuntimeMesh);
    }

    IdleAnimation = LoadUALAsset<UAnimSequenceBase>(TEXT("UAL1_StandardIdle_Loop"));
    WalkAnimation = LoadUALAsset<UAnimSequenceBase>(TEXT("UAL1_StandardWalk_Loop"));
    RunAnimation = LoadUALAsset<UAnimSequenceBase>(TEXT("UAL1_StandardJog_Fwd_Loop"));
    PistolIdleAnimation = LoadUALAsset<UAnimSequenceBase>(TEXT("UAL1_StandardPistol_Idle_Loop"));
    PistolShootAnimation = LoadUALAsset<UAnimSequenceBase>(TEXT("UAL1_StandardPistol_Shoot"));
    HitAnimation = LoadUALAsset<UAnimSequenceBase>(TEXT("UAL1_StandardHit_Chest"));
    DeathAnimation = LoadUALAsset<UAnimSequenceBase>(TEXT("UAL1_StandardDeath01"));
    bUseSingleNodeAnimationFallback = true;
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

    // IMPORTANT: never begin this trace hundreds of centimeters above the NPC.
    // Indoors that made the first blocking hit a ceiling/awning and teleported the
    // enemy onto it, which visually read as floating. Trace from capsule level
    // downward only, then let CharacterMovement own floor contact afterwards.
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

void ARiftAnimatedHumanoidNPC::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDead)
    {
        return;
    }

    // Mirror the proven Colossus rule: actual velocity selects a compatible UAL1
    // clip and PlayAnimation drives the SkeletalMesh directly.
    UpdateColossusStyleAnimation();
}
