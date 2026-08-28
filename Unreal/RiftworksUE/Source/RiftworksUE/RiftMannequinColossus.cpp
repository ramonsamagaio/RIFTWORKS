#include "RiftMannequinColossus.h"

#include "AIController.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ARiftMannequinColossus::ARiftMannequinColossus()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    GetCapsuleComponent()->InitCapsuleSize(520.0f, 1750.0f);
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;

    LegsCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("LegsWeakpoint"));
    LegsCollision->SetupAttachment(GetCapsuleComponent());
    LegsCollision->SetBoxExtent(FVector(430.0f, 360.0f, 650.0f));
    LegsCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 520.0f));
    LegsCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    LegsCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    LegsCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    LegsCollision->SetHiddenInGame(true);

    TorsoCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("TorsoWeakpoint"));
    TorsoCollision->SetupAttachment(GetCapsuleComponent());
    TorsoCollision->SetBoxExtent(FVector(620.0f, 430.0f, 560.0f));
    TorsoCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 1900.0f));
    TorsoCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TorsoCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    TorsoCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TorsoCollision->SetHiddenInGame(true);

    HeadCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HeadWeakpoint"));
    HeadCollision->SetupAttachment(GetCapsuleComponent());
    HeadCollision->SetBoxExtent(FVector(360.0f, 320.0f, 330.0f));
    HeadCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 2950.0f));
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

    if (GetMesh())
    {
        GetMesh()->SetVisibility(true, true);
        GetMesh()->SetOwnerNoSee(false);
        GetMesh()->SetRelativeScale3D(FVector(VisualScale));
        GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -1420.0f));
        GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    }

    PrototypeRouteCenter = GetActorLocation();
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    UpdatePrototypeAnimation();
}

void ARiftMannequinColossus::Tick(float DeltaSeconds)
{
    // ARiftColossus has its own early placeholder movement. We intentionally replace it here.
    ACharacter::Tick(DeltaSeconds);

    if (GetCharacterMovement()->MovementMode == MOVE_None)
    {
        return;
    }

    PrototypeRouteAngle += DeltaSeconds * 0.075f;
    const FVector Desired = PrototypeRouteCenter + FVector(FMath::Cos(PrototypeRouteAngle), FMath::Sin(PrototypeRouteAngle), 0.0f) * RouteRadius;
    const FVector Direction = (Desired - GetActorLocation()).GetSafeNormal2D();

    if (!Direction.IsNearlyZero())
    {
        AddMovementInput(Direction, 1.0f);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), Direction.Rotation(), DeltaSeconds, 1.4f));
    }
    UpdatePrototypeAnimation();
}

void ARiftMannequinColossus::UpdatePrototypeAnimation()
{
    if (!GetMesh())
    {
        return;
    }

    UAnimSequenceBase* Desired = GetVelocity().Size2D() > 8.0f ? WalkAnimation.Get() : IdleAnimation.Get();
    if (!Desired)
    {
        Desired = WalkAnimation ? WalkAnimation.Get() : IdleAnimation.Get();
    }
    if (Desired && Desired != CurrentAnimation)
    {
        CurrentAnimation = Desired;
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->PlayAnimation(Desired, true);
    }
}
