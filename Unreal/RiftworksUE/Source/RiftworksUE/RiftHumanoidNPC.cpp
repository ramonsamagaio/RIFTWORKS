#include "RiftHumanoidNPC.h"

#include "RiftPlayerCharacter.h"
#include "AIController.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
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
    WeaponMuzzleLight->Intensity = 0.0f;
    WeaponMuzzleLight->AttenuationRadius = 700.0f;
    WeaponMuzzleLight->LightColor = FColor(255, 164, 77);
    WeaponMuzzleLight->SourceRadius = 6.0f;
    WeaponMuzzleLight->VolumetricScatteringIntensity = 3.0f;
    WeaponMuzzleLight->CastShadows = true;

    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
    bUseControllerRotationYaw = false;
}

void ARiftHumanoidNPC::BeginPlay()
{
    Super::BeginPlay();
    PatrolOrigin = GetActorLocation();
    PlayerTarget = Cast<ARiftPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ARiftHumanoidNPC::OnTargetPerceptionUpdated);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
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
    if (!AI)
    {
        return;
    }

    if (!PlayerTarget)
    {
        PlayerTarget = Cast<ARiftPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    }

    if (PlayerTarget)
    {
        const float Distance = FVector::Dist(GetActorLocation(), PlayerTarget->GetActorLocation());
        if (!bAlerted && PlayerTarget->bFlashlightOn && Distance < 4400.0f && AI->LineOfSightTo(PlayerTarget))
        {
            AlertToLocation(PlayerTarget->GetActorLocation());
        }

        if (bAlerted)
        {
            InvestigationLocation = PlayerTarget->GetActorLocation();
            if (Distance <= FireRange && AI->LineOfSightTo(PlayerTarget))
            {
                AI->StopMovement();
                const FRotator Look = (PlayerTarget->GetActorLocation() - GetActorLocation()).Rotation();
                SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.0f, Look.Yaw, 0.0f), DeltaSeconds, 7.0f));
                if (FireCooldown <= 0.0f)
                {
                    FireAtPlayer();
                }
            }
            else
            {
                AI->MoveToActor(PlayerTarget, 850.0f, true, true, true, nullptr, true);
            }
            return;
        }
    }

    if (!InvestigationLocation.IsNearlyZero())
    {
        AI->MoveToLocation(InvestigationLocation, 120.0f, true, true, true, false, nullptr, true);
        if (FVector::DistSquared2D(GetActorLocation(), InvestigationLocation) < FMath::Square(180.0f))
        {
            InvestigationLocation = FVector::ZeroVector;
            PatrolCooldown = 1.5f;
        }
        return;
    }

    if (PatrolCooldown <= 0.0f && AI->GetMoveStatus() != EPathFollowingStatus::Moving)
    {
        PickPatrolPoint();
    }
}

void ARiftHumanoidNPC::PickPatrolPoint()
{
    AAIController* AI = Cast<AAIController>(GetController());
    UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!AI || !Nav)
    {
        return;
    }

    FNavLocation Point;
    if (Nav->GetRandomReachablePointInRadius(PatrolOrigin, PatrolRadius, Point))
    {
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
        AI->MoveToLocation(Point.Location, 90.0f);
        PatrolCooldown = FMath::FRandRange(4.0f, 8.0f);
    }
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

    WeaponMuzzleLight->SetIntensity(4800.0f);
    GetWorldTimerManager().ClearTimer(MuzzleTimer);
    GetWorldTimerManager().SetTimer(MuzzleTimer, this, &ARiftHumanoidNPC::EndMuzzleFlash, 0.06f, false);

    if (Hit.GetActor() == PlayerTarget)
    {
        UGameplayStatics::ApplyPointDamage(PlayerTarget, RangedDamage, Direction, Hit, GetController(), this, nullptr);
    }

    if (bUseSingleNodeAnimationFallback && PistolShootAnimation)
    {
        bAttackAnimationLocked = true;
        CurrentFallbackAnimation = PistolShootAnimation;
        GetMesh()->PlayAnimation(PistolShootAnimation, false);
        GetWorldTimerManager().SetTimer(AttackAnimTimer, this, &ARiftHumanoidNPC::EndAttackAnimation, FMath::Max(0.15f, PistolShootAnimation->GetPlayLength()), false);
    }

    BP_OnRangedAttack(PlayerTarget);
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
        Desired = WalkAnimation.Get();
    }
    else
    {
        Desired = RunAnimation.Get();
    }

    if (Desired && Desired != CurrentFallbackAnimation)
    {
        CurrentFallbackAnimation = Desired;
        GetMesh()->PlayAnimation(Desired, true);
    }
}

float ARiftHumanoidNPC::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead)
    {
        return 0.0f;
    }

    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health -= DamageAmount;
    if (DamageCauser)
    {
        AlertToLocation(DamageCauser->GetActorLocation());
    }
    if (Health <= 0.0f)
    {
        Die();
    }
    return Applied > 0.0f ? Applied : DamageAmount;
}

void ARiftHumanoidNPC::Die()
{
    bDead = true;
    if (AAIController* AI = Cast<AAIController>(GetController()))
    {
        AI->StopMovement();
    }
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (bUseSingleNodeAnimationFallback && DeathAnimation)
    {
        GetMesh()->PlayAnimation(DeathAnimation, false);
    }
    BP_OnKilled();
    SetLifeSpan(8.0f);
}
