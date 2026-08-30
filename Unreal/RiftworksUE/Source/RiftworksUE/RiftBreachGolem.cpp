#include "RiftBreachGolem.h"

#include "RiftGameplayActors.h"
#include "RiftPlayerCharacter.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

ARiftBreachGolem::ARiftBreachGolem()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    // ACharacter actor origin is the center of the capsule, not the floor.
    // Keep the procedural body entirely inside a capsule whose bottom nearly
    // matches the visible feet. The old layout left ~1.1 m of invisible air
    // below the legs and made a correctly-grounded Character look airborne.
    GetCapsuleComponent()->InitCapsuleSize(62.0f, 150.0f);
    GetMesh()->SetVisibility(false);
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 280.0f, 0.0f);

    Torso = MakeBlock(TEXT("Torso"), FVector(0, 0, -18), FVector(0.85f, 0.58f, 1.05f));
    Head = MakeBlock(TEXT("Head"), FVector(0, 0, 92), FVector(0.58f, 0.58f, 0.55f));
    LeftArm = MakeBlock(TEXT("LeftArm"), FVector(0, -78, -5), FVector(0.38f, 0.38f, 0.95f));
    RightArm = MakeBlock(TEXT("RightArm"), FVector(0, 78, -5), FVector(0.38f, 0.38f, 0.95f));
    LeftLeg = MakeBlock(TEXT("LeftLeg"), FVector(0, -37, -105), FVector(0.45f, 0.45f, 0.85f));
    RightLeg = MakeBlock(TEXT("RightLeg"), FVector(0, 37, -105), FVector(0.45f, 0.45f, 0.85f));

    CoreLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BreachCoreLight"));
    CoreLight->SetupAttachment(Torso);
    CoreLight->SetRelativeLocation(FVector(-55.0f, 0.0f, 10.0f));
    CoreLight->SetIntensityUnits(ELightUnits::Lumens);
    CoreLight->SetIntensity(1450.0f);
    CoreLight->SetAttenuationRadius(720.0f);
    CoreLight->SetLightColor(FLinearColor(0.57f, 0.39f, 0.88f));
    CoreLight->VolumetricScatteringIntensity = 0.12f;
    CoreLight->CastShadows = true;
}

UStaticMeshComponent* ARiftBreachGolem::MakeBlock(FName Name, FVector Location, FVector Scale)
{
    UStaticMeshComponent* Block = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Block->SetupAttachment(GetCapsuleComponent());
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        Block->SetStaticMesh(Cube.Object);
    }
    Block->SetRelativeLocation(Location);
    Block->SetRelativeScale3D(Scale);
    Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    return Block;
}

void ARiftBreachGolem::BeginPlay()
{
    Super::BeginPlay();
    TargetPlayer = Cast<ARiftPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

    UMaterialInterface* Stone = Cast<UMaterialInterface>(StaticLoadObject(
        UMaterialInterface::StaticClass(), nullptr,
        TEXT("/Game/Riftworks/Materials/World/M_Breach_BlackStone.M_Breach_BlackStone")));
    UMaterialInterface* Accent = Cast<UMaterialInterface>(StaticLoadObject(
        UMaterialInterface::StaticClass(), nullptr,
        TEXT("/Game/Riftworks/Materials/World/M_Assembly_Motor.M_Assembly_Motor")));

    if (Stone)
    {
        Torso->SetMaterial(0, Stone);
        LeftArm->SetMaterial(0, Stone);
        RightArm->SetMaterial(0, Stone);
        LeftLeg->SetMaterial(0, Stone);
        RightLeg->SetMaterial(0, Stone);
    }
    if (Head)
    {
        Head->SetMaterial(0, Accent ? Accent : Stone);
    }
}

void ARiftBreachGolem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead)
    {
        return;
    }

    AttackCooldown = FMath::Max(0.0f, AttackCooldown - DeltaSeconds);
    if (!TargetPlayer)
    {
        TargetPlayer = Cast<ARiftPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    }

    AAIController* AI = Cast<AAIController>(GetController());
    const float Distance = TargetPlayer ? FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation()) : TNumericLimits<float>::Max();
    if (AI && TargetPlayer && Distance <= DetectionRange)
    {
        if (Distance > AttackRange)
        {
            const EPathFollowingRequestResult::Type MoveResult = AI->MoveToActor(TargetPlayer, AttackRange * 0.82f, true, true, true, nullptr, true);
            if (MoveResult == EPathFollowingRequestResult::Failed)
            {
                const FVector Direction = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
                GetCharacterMovement()->Velocity = FVector(Direction.X * MoveSpeed, Direction.Y * MoveSpeed, GetCharacterMovement()->Velocity.Z);
                if (!Direction.IsNearlyZero())
                {
                    SetActorRotation(FMath::RInterpTo(GetActorRotation(), Direction.Rotation(), DeltaSeconds, 3.0f));
                }
            }
        }
        else
        {
            AI->StopMovement();
            if (AttackCooldown <= 0.0f)
            {
                UGameplayStatics::ApplyDamage(TargetPlayer, AttackDamage, GetController(), this, nullptr);
                AttackCooldown = 1.35f;
                RightArm->SetRelativeRotation(FRotator(-52.0f, 0.0f, 0.0f));
            }
        }
    }

    const float Speed = GetVelocity().Size2D();
    if (Speed > 8.0f)
    {
        WalkPhase += DeltaSeconds * (2.5f + Speed * 0.014f);
        const float Swing = FMath::Sin(WalkPhase) * 28.0f;
        LeftLeg->SetRelativeRotation(FRotator(Swing, 0.0f, 0.0f));
        RightLeg->SetRelativeRotation(FRotator(-Swing, 0.0f, 0.0f));
        LeftArm->SetRelativeRotation(FRotator(-Swing * 0.65f, 0.0f, 0.0f));
        RightArm->SetRelativeRotation(FRotator(Swing * 0.65f, 0.0f, 0.0f));
    }
    else if (AttackCooldown < 1.0f)
    {
        RightArm->SetRelativeRotation(FMath::RInterpTo(RightArm->GetRelativeRotation(), FRotator::ZeroRotator, DeltaSeconds, 6.0f));
    }

    if (CoreLight)
    {
        CoreLight->SetIntensity(1250.0f + FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f) * 210.0f);
    }
}

float ARiftBreachGolem::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead || DamageAmount <= 0.0f)
    {
        return 0.0f;
    }
    Health -= DamageAmount;
    if (DamageCauser)
    {
        TargetPlayer = Cast<ARiftPlayerCharacter>(DamageCauser);
    }
    if (Health <= 0.0f)
    {
        Die();
    }
    return DamageAmount;
}

void ARiftBreachGolem::Die()
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
    BP_OnGolemKilled();

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (ARiftSalvageActor* Core = GetWorld()->SpawnActor<ARiftSalvageActor>(ARiftSalvageActor::StaticClass(), GetActorLocation() + FVector(0, 0, 75), FRotator::ZeroRotator, Params))
    {
        Core->ItemId = TEXT("breach_core");
        Core->DisplayName = FText::FromString(TEXT("Golem Breach Core"));
        Core->Amount = 1;
        Core->bHeavy = false;
        Core->SetCarriedState(false);
    }
    SetLifeSpan(3.5f);
}
