#include "RiftHumanoidNPC.h"

#include "RiftPlayerCharacter.h"
#include "AIController.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"

ARiftHumanoidNPC::ARiftHumanoidNPC()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    // Keep visual scale, collision and nav agent in agreement. Older Blueprints
    // inherited the generic Character capsule and made imported mannequins look
    // suspended even when the Actor itself was on the floor.
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 92.0f);

    Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 3500.0f;
    SightConfig->LoseSightRadius = 4500.0f;
    SightConfig->PeripheralVisionAngleDegrees = 72.0f;
    SightConfig->SetMaxAge(5.0f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 6500.0f;
    HearingConfig->SetMaxAge(7.0f);
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

    Perception->ConfigureSense(*SightConfig);
    Perception->ConfigureSense(*HearingConfig);
    Perception->SetDominantSense(SightConfig->GetSenseImplementation());

    WeaponMuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WeaponMuzzleLight"));
    WeaponMuzzleLight->SetupAttachment(GetMesh());
    WeaponMuzzleLight->SetRelativeLocation(FVector(38.0f, 0.0f, 120.0f));
    WeaponMuzzleLight->IntensityUnits = ELightUnits::Lumens;
    WeaponMuzzleLight->SetIntensity(0.0f);
    WeaponMuzzleLight->SetAttenuationRadius(600.0f);
    WeaponMuzzleLight->SetLightColor(FColor(255, 164, 77));
    WeaponMuzzleLight->SetSourceRadius(4.0f);
    WeaponMuzzleLight->SetVolumetricScatteringIntensity(0.03f);
    WeaponMuzzleLight->CastShadows = true;

    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    bUseControllerRotationYaw = false;
}

void ARiftHumanoidNPC::BeginPlay()
{
    Super::BeginPlay();
    PlayerTarget = Cast<ARiftPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ARiftHumanoidNPC::OnTargetPerceptionUpdated);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    if (GetMesh())
    {
        // Import conversion already establishes the mesh forward axis. Applying
        // the old hard-coded -90 yaw on top of it was rotating some characters
        // sideways. The editor audit computes Z from real mesh bounds.
        GetMesh()->SetRelativeRotation(FRotator::ZeroRotator);
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    }
    PatrolOrigin = GetActorLocation();
    PickPatrolPoint();
}

void ARiftHumanoidNPC::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead)
    {
        return;
    }

    FireCooldown = FMath::Max(0.0f, FireCooldown - DeltaSeconds);
    PatrolCooldown = FMath::Max(0.0f, PatrolCooldown - DeltaSeconds);
    UpdateBehavior(DeltaSeconds);
    UpdateFallbackAnimation();
}

void ARiftHumanoidNPC::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    ARiftPlayerCharacter* SeenPlayer = Cast<ARiftPlayerCharacter>(Actor);
    if (!SeenPlayer)
    {
        return;
    }

    PlayerTarget = SeenPlayer;
    if (Stimulus.WasSuccessfullySensed())
    {
        AlertToLocation(Stimulus.StimulusLocation);
    }
    else if (bAlerted)
    {
        InvestigationLocation = Stimulus.StimulusLocation;
    }
}

void ARiftHumanoidNPC::AlertToLocation(FVector Location)
{
    bAlerted = true;
    InvestigationLocation = Location;
    GetCharacterMovement()->MaxWalkSpeed = CombatSpeed;
    BP_OnAlerted(Location);
}

void ARiftHumanoidNPC::UpdateBehavior(float DeltaSeconds)
{
    AAIController* AI = Cast<AAIController>(GetController());

    if (!PlayerTarget)
    {
        PlayerTarget = Cast<ARiftPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    }

    if (PlayerTarget)
    {
        const float Distance = FVector::Dist(GetActorLocation(), PlayerTarget->GetActorLocation());
        const FVector ToPlayer = (PlayerTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        const bool bHasLineOfSight = AI ? AI->LineOfSightTo(PlayerTarget) : true;

        if (!bAlerted && PlayerTarget->bFlashlightOn && Distance < 4400.0f && bHasLineOfSight)
        {
            AlertToLocation(PlayerTarget->GetActorLocation());
        }

        if (bAlerted)
        {
            InvestigationLocation = PlayerTarget->GetActorLocation();
            if (Distance <= FireRange && bHasLineOfSight)
            {
                if (AI)
                {
                    AI->StopMovement();
                }
                const FRotator Look = (PlayerTarget->GetActorLocation() - GetActorLocation()).Rotation();
                SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.0f, Look.Yaw, 0.0f), DeltaSeconds, 7.0f));
                if (FireCooldown <= 0.0f)
                {
                    FireAtPlayer();
                }
            }
            else
            {
                bool bPathing = false;
                if (AI)
                {
                    const EPathFollowingRequestResult::Type Result = AI->MoveToActor(PlayerTarget, 850.0f, true, true, true, nullptr, true);
                    bPathing = Result != EPathFollowingRequestResult::Failed;
                }
                if (!bPathing || GetVelocity().Size2D() < 8.0f)
                {
                    AddMovementInput(ToPlayer, 1.0f);
                }
            }
            return;
        }
    }

    if (!InvestigationLocation.IsNearlyZero())
    {
        const FVector Direction = (InvestigationLocation - GetActorLocation()).GetSafeNormal2D();
        bool bPathing = false;
        if (AI)
        {
            const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(InvestigationLocation, 120.0f, true, true, true, false, nullptr, true);
            bPathing = Result != EPathFollowingRequestResult::Failed;
        }
        if (!bPathing || GetVelocity().Size2D() < 5.0f)
        {
            AddMovementInput(Direction, 0.75f);
        }
        if (FVector::DistSquared2D(GetActorLocation(), InvestigationLocation) < FMath::Square(180.0f))
        {
            InvestigationLocation = FVector::ZeroVector;
            PatrolCooldown = 1.5f;
        }
        return;
    }

    if (!PatrolTarget.IsNearlyZero())
    {
        const float DistSq = FVector::DistSquared2D(GetActorLocation(), PatrolTarget);
        if (DistSq < FMath::Square(120.0f))
        {
            PatrolTarget = FVector::ZeroVector;
            PatrolCooldown = FMath::FRandRange(1.5f, 3.5f);
        }
        else if (!AI || AI->GetMoveStatus() != EPathFollowingStatus::Moving || GetVelocity().Size2D() < 5.0f)
        {
            AddMovementInput((PatrolTarget - GetActorLocation()).GetSafeNormal2D(), 0.55f);
        }
    }

    if (PatrolCooldown <= 0.0f && PatrolTarget.IsNearlyZero() && (!AI || AI->GetMoveStatus() != EPathFollowingStatus::Moving))
    {
        PickPatrolPoint();
    }
}

void ARiftHumanoidNPC::PickPatrolPoint()
{
    AAIController* AI = Cast<AAIController>(GetController());
    UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());

    FNavLocation Point;
    bool bFoundNavPoint = false;
    if (Nav)
    {
        bFoundNavPoint = Nav->GetRandomReachablePointInRadius(PatrolOrigin, PatrolRadius, Point);
    }

    if (bFoundNavPoint)
    {
        PatrolTarget = Point.Location;
        if (AI)
        {
            AI->MoveToLocation(PatrolTarget, 90.0f);
        }
    }
    else
    {
        const FVector2D Circle = FMath::RandPointInCircle(PatrolRadius);
        PatrolTarget = PatrolOrigin + FVector(Circle.X, Circle.Y, 0.0f);
    }
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    PatrolCooldown = FMath::FRandRange(4.0f, 8.0f);
}

void ARiftHumanoidNPC::FireAtPlayer()
{
    if (!PlayerTarget || !GetWorld())
    {
        return;
    }

    FireCooldown = FireInterval * FMath::FRandRange(0.8f, 1.25f);
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 135.0f);
    const FVector Target = PlayerTarget->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
    const FVector Direction = (Target - Start).GetSafeNormal();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftNPCFire), true, this);
    GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Direction * FireRange, ECC_Visibility, Params);

    WeaponMuzzleLight->SetIntensity(2500.0f);
    GetWorldTimerManager().ClearTimer(MuzzleTimer);
    GetWorldTimerManager().SetTimer(MuzzleTimer, this, &ARiftHumanoidNPC::EndMuzzleFlash, 0.045f, false);

    if (Hit.GetActor() == PlayerTarget)
    {
        UGameplayStatics::ApplyPointDamage(PlayerTarget, RangedDamage, Direction, Hit, GetController(), this, nullptr);
    }

    if (PistolShootAnimation)
    {
        PlayOneShot(PistolShootAnimation);
    }

    BP_OnRangedAttack(PlayerTarget);
}

bool ARiftHumanoidNPC::IsAnimationCompatible(const UAnimSequenceBase* Animation) const
{
    if (!Animation || !GetMesh())
    {
        return false;
    }
    const USkeletalMesh* MeshAsset = GetMesh()->GetSkeletalMeshAsset();
    return MeshAsset && MeshAsset->GetSkeleton() && Animation->GetSkeleton() == MeshAsset->GetSkeleton();
}

void ARiftHumanoidNPC::PlayOneShot(UAnimSequenceBase* Animation, float MinimumLock)
{
    if (!bUseSingleNodeAnimationFallback || !IsAnimationCompatible(Animation))
    {
        return;
    }
    bAttackAnimationLocked = true;
    CurrentFallbackAnimation = Animation;
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    GetMesh()->PlayAnimation(Animation, false);
    GetWorldTimerManager().ClearTimer(AttackAnimTimer);
    GetWorldTimerManager().SetTimer(AttackAnimTimer, this, &ARiftHumanoidNPC::EndAttackAnimation, FMath::Max(MinimumLock, Animation->GetPlayLength()), false);
}

void ARiftHumanoidNPC::EndMuzzleFlash()
{
    if (WeaponMuzzleLight)
    {
        WeaponMuzzleLight->SetIntensity(0.0f);
    }
}

void ARiftHumanoidNPC::EndAttackAnimation()
{
    bAttackAnimationLocked = false;
    CurrentFallbackAnimation = nullptr;
}

void ARiftHumanoidNPC::UpdateFallbackAnimation()
{
    if (!bUseSingleNodeAnimationFallback || bAttackAnimationLocked || bDead || !GetMesh())
    {
        return;
    }

    const float Speed = GetVelocity().Size2D();
    UAnimSequenceBase* Desired = nullptr;
    if (Speed < 10.0f)
    {
        Desired = bAlerted && PistolIdleAnimation ? PistolIdleAnimation.Get() : IdleAnimation.Get();
    }
    else if (Speed < 250.0f)
    {
        Desired = WalkAnimation ? WalkAnimation.Get() : RunAnimation.Get();
    }
    else
    {
        Desired = RunAnimation ? RunAnimation.Get() : WalkAnimation.Get();
    }

    if (!IsAnimationCompatible(Desired))
    {
        UAnimSequenceBase* Candidates[] = {
            IdleAnimation.Get(), WalkAnimation.Get(), RunAnimation.Get(), PistolIdleAnimation.Get()
        };
        Desired = nullptr;
        for (UAnimSequenceBase* Candidate : Candidates)
        {
            if (IsAnimationCompatible(Candidate))
            {
                Desired = Candidate;
                break;
            }
        }
    }

    if (Desired && Desired != CurrentFallbackAnimation)
    {
        CurrentFallbackAnimation = Desired;
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->PlayAnimation(Desired, true);
    }
}

float ARiftHumanoidNPC::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead || DamageAmount <= 0.0f)
    {
        return 0.0f;
    }

    Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health = FMath::Max(0.0f, Health - DamageAmount);
    BP_OnHit(Health, DamageAmount);

    if (DamageCauser)
    {
        AlertToLocation(DamageCauser->GetActorLocation());
    }

    if (Health <= 0.0f)
    {
        Die();
    }
    else if (HitAnimation)
    {
        PlayOneShot(HitAnimation, 0.18f);
    }
    return DamageAmount;
}

void ARiftHumanoidNPC::Die()
{
    if (bDead)
    {
        return;
    }
    bDead = true;
    if (AAIController* AI = Cast<AAIController>(GetController()))
    {
        AI->StopMovement();
    }
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (IsAnimationCompatible(DeathAnimation))
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->PlayAnimation(DeathAnimation, false);
    }
    else
    {
        SetActorRotation(GetActorRotation() + FRotator(0.0f, 0.0f, 88.0f));
    }
    BP_OnKilled();
    SetLifeSpan(9.0f);
}
