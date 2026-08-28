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
#include "UObject/ConstructorHelpers.h"

ARiftBreachGolem::ARiftBreachGolem()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    GetCapsuleComponent()->InitCapsuleSize(62.0f, 118.0f);
    GetMesh()->SetVisibility(false);
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 280.0f, 0.0f);

    Torso = MakeBlock(TEXT("Torso"), FVector(0, 0, 125), FVector(0.85f, 0.58f, 1.05f));
    Head = MakeBlock(TEXT("Head"), FVector(0, 0, 235), FVector(0.58f, 0.58f, 0.55f));
    LeftArm = MakeBlock(TEXT("LeftArm"), FVector(0, -78, 138), FVector(0.38f, 0.38f, 0.95f));
    RightArm = MakeBlock(TEXT("RightArm"), FVector(0, 78, 138), FVector(0.38f, 0.38f, 0.95f));
    LeftLeg = MakeBlock(TEXT("LeftLeg"), FVector(0, -37, 38), FVector(0.45f, 0.45f, 0.85f));
    RightLeg = MakeBlock(TEXT("RightLeg"), FVector(0, 37, 38), FVector(0.45f, 0.45f, 0.85f));

    CoreLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BreachCoreLight"));
    CoreLight->SetupAttachment(Torso);
    CoreLight->SetRelativeLocation(FVector(-55.0f, 0.0f, 10.0f));
    CoreLight->IntensityUnits = ELightUnits::Lumens;
    CoreLight->Intensity = 1900.0f;
    CoreLight->AttenuationRadius = 800.0f;
    CoreLight->LightColor = FColor(145, 100, 225);
    CoreLight->VolumetricScatteringIntensity = 2.0f;
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
    const float Distance = TargetPlayer ? FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation()) : BIG_NUMBER;
    if (AI && TargetPlayer && Distance <= DetectionRange)
    {
        if (Distance > AttackRange)
        {
            AI->MoveToActor(TargetPlayer, AttackRange * 0.82f, true, true, true, nullptr, true);
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
        CoreLight->SetIntensity(1700.0f + FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f) * 350.0f);
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
