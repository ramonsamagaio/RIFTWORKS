#include "RiftCrawlerCreature.h"

#include "RiftGameplayActors.h"
#include "RiftPlayerCharacter.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

ARiftCrawlerCreature::ARiftCrawlerCreature()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    GetCapsuleComponent()->InitCapsuleSize(48.0f, 55.0f);
    GetMesh()->SetVisibility(false);
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);

    Body = MakePart(TEXT("CrawlerBody"), FVector(0.0f, 0.0f, 15.0f), FVector(0.95f, 0.62f, 0.34f));
    Head = MakePart(TEXT("CrawlerHead"), FVector(68.0f, 0.0f, 9.0f), FVector(0.48f, 0.50f, 0.30f), FRotator(0.0f, 7.0f, 0.0f));

    const float Side[6] = {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
    const float Along[6] = {-54.0f, 0.0f, 52.0f, -54.0f, 0.0f, 52.0f};
    for (int32 Index = 0; Index < 6; ++Index)
    {
        const float Y = Side[Index] * 52.0f;
        UStaticMeshComponent* Leg = MakePart(
            *FString::Printf(TEXT("CrawlerLeg_%02d"), Index),
            FVector(Along[Index], Y, -12.0f),
            FVector(0.13f, 0.48f, 0.12f),
            FRotator(0.0f, Side[Index] < 0.0f ? -18.0f : 18.0f, Side[Index] * 12.0f));
        Legs.Add(Leg);
    }
}

UStaticMeshComponent* ARiftCrawlerCreature::MakePart(FName Name, FVector Location, FVector Scale, FRotator Rotation)
{
    UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Part->SetupAttachment(GetCapsuleComponent());
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        Part->SetStaticMesh(Cube.Object);
    }
    Part->SetRelativeLocation(Location);
    Part->SetRelativeScale3D(Scale);
    Part->SetRelativeRotation(Rotation);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    return Part;
}

void ARiftCrawlerCreature::BeginPlay()
{
    Super::BeginPlay();
    TargetPlayer = Cast<ARiftPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

    UMaterialInterface* Material = Cast<UMaterialInterface>(StaticLoadObject(
        UMaterialInterface::StaticClass(), nullptr,
        TEXT("/Game/Riftworks/Materials/World/M_Breach_BlackStone.M_Breach_BlackStone")));
    if (Material)
    {
        if (Body) Body->SetMaterial(0, Material);
        if (Head) Head->SetMaterial(0, Material);
        for (UStaticMeshComponent* Leg : Legs)
        {
            if (Leg) Leg->SetMaterial(0, Material);
        }
    }
}

void ARiftCrawlerCreature::Tick(float DeltaSeconds)
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

    const float Distance = TargetPlayer ? FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation()) : TNumericLimits<float>::Max();
    if (AAIController* AI = Cast<AAIController>(GetController()))
    {
        if (TargetPlayer && Distance <= DetectionRange)
        {
            if (Distance > AttackRange)
            {
                AI->MoveToActor(TargetPlayer, AttackRange * 0.78f, true, true, true, nullptr, true);
            }
            else
            {
                AI->StopMovement();
                if (AttackCooldown <= 0.0f)
                {
                    UGameplayStatics::ApplyDamage(TargetPlayer, AttackDamage, GetController(), this, nullptr);
                    AttackCooldown = AttackInterval;
                    if (Head)
                    {
                        Head->SetRelativeRotation(FRotator(-24.0f, 0.0f, 0.0f));
                    }
                }
            }
        }
        else
        {
            AI->StopMovement();
        }
    }

    UpdateProceduralGait(DeltaSeconds);
    if (Head && AttackCooldown < AttackInterval * 0.65f)
    {
        Head->SetRelativeRotation(FMath::RInterpTo(Head->GetRelativeRotation(), FRotator(0.0f, 7.0f, 0.0f), DeltaSeconds, 7.0f));
    }
}

void ARiftCrawlerCreature::UpdateProceduralGait(float DeltaSeconds)
{
    const float Speed = GetVelocity().Size2D();
    if (Speed < 5.0f)
    {
        return;
    }

    GaitPhase += DeltaSeconds * (5.5f + Speed * 0.018f);
    for (int32 Index = 0; Index < Legs.Num(); ++Index)
    {
        UStaticMeshComponent* Leg = Legs[Index];
        if (!Leg)
        {
            continue;
        }
        const float TripodOffset = (Index == 0 || Index == 2 || Index == 4) ? 0.0f : PI;
        const float Swing = FMath::Sin(GaitPhase + TripodOffset) * 24.0f;
        const float SideSign = Index < 3 ? -1.0f : 1.0f;
        Leg->SetRelativeRotation(FRotator(Swing, SideSign * 18.0f, SideSign * 12.0f));
    }
    if (Body)
    {
        Body->SetRelativeLocation(FVector(0.0f, 0.0f, 15.0f + FMath::Sin(GaitPhase * 2.0f) * 2.5f));
    }
}

float ARiftCrawlerCreature::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead || DamageAmount <= 0.0f)
    {
        return 0.0f;
    }
    Health -= DamageAmount;
    if (DamageCauser)
    {
        if (ARiftPlayerCharacter* Player = Cast<ARiftPlayerCharacter>(DamageCauser))
        {
            TargetPlayer = Player;
        }
    }
    if (Health <= 0.0f)
    {
        Die();
    }
    return DamageAmount;
}

void ARiftCrawlerCreature::Die()
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
    BP_OnCrawlerKilled();

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (ARiftSalvageActor* Tendon = GetWorld()->SpawnActor<ARiftSalvageActor>(
        ARiftSalvageActor::StaticClass(), GetActorLocation() + FVector(20.0f, 20.0f, 35.0f), FRotator::ZeroRotator, Params))
    {
        Tendon->ItemId = TEXT("tendon_material");
        Tendon->DisplayName = FText::FromString(TEXT("Crawler Tendon Bundle"));
        Tendon->Amount = 1;
        Tendon->bHeavy = false;
        Tendon->SetCarriedState(false);
    }

    if (ARiftSalvageActor* Plate = GetWorld()->SpawnActor<ARiftSalvageActor>(
        ARiftSalvageActor::StaticClass(), GetActorLocation() + FVector(-25.0f, -20.0f, 35.0f), FRotator::ZeroRotator, Params))
    {
        Plate->ItemId = TEXT("crawler_plate");
        Plate->DisplayName = FText::FromString(TEXT("Crawler Carapace Plate"));
        Plate->Amount = 2;
        Plate->bHeavy = false;
        Plate->SetCarriedState(false);
    }

    SetLifeSpan(2.0f);
}
