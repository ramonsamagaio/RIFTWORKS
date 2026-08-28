#include "RiftEngineeringJoint.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "UObject/ConstructorHelpers.h"

ARiftEngineeringJoint::ARiftEngineeringJoint()
{
    PrimaryActorTick.bCanEverTick = true;

    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JointMarker"));
    SetRootComponent(Marker);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        Marker->SetStaticMesh(Cube.Object);
    }
    Marker->SetRelativeScale3D(FVector(0.16f));
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetHiddenInGame(true);

    Constraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("Constraint"));
    Constraint->SetupAttachment(Marker);
    Constraint->SetDisableCollision(true);
}

void ARiftEngineeringJoint::BeginPlay()
{
    Super::BeginPlay();
    ConfigureConstraint();
}

void ARiftEngineeringJoint::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ConfigureConstraint();
}

UPrimitiveComponent* ARiftEngineeringJoint::FindPhysicsPrimitive(AActor* Actor) const
{
    if (!Actor)
    {
        return nullptr;
    }

    TArray<UPrimitiveComponent*> Primitives;
    Actor->GetComponents<UPrimitiveComponent>(Primitives);
    for (UPrimitiveComponent* Primitive : Primitives)
    {
        if (Primitive && Primitive->IsSimulatingPhysics())
        {
            return Primitive;
        }
    }
    for (UPrimitiveComponent* Primitive : Primitives)
    {
        if (Primitive && Primitive->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
        {
            return Primitive;
        }
    }
    return nullptr;
}

bool ARiftEngineeringJoint::AttachActors(AActor* NewActorA, AActor* NewActorB)
{
    if (!NewActorA || !NewActorB || NewActorA == NewActorB)
    {
        return false;
    }
    ActorA = NewActorA;
    ActorB = NewActorB;
    SetActorLocation((ActorA->GetActorLocation() + ActorB->GetActorLocation()) * 0.5f);
    ConfigureConstraint();
    return FindPhysicsPrimitive(ActorA) && FindPhysicsPrimitive(ActorB);
}

void ARiftEngineeringJoint::ConfigureConstraint()
{
    if (!Constraint)
    {
        return;
    }

    UPrimitiveComponent* PrimitiveA = FindPhysicsPrimitive(ActorA);
    UPrimitiveComponent* PrimitiveB = FindPhysicsPrimitive(ActorB);
    if (PrimitiveA && PrimitiveB)
    {
        Constraint->SetConstrainedComponents(PrimitiveA, NAME_None, PrimitiveB, NAME_None);
    }

    Constraint->SetAngularVelocityDriveTwistAndSwing(false, false);
    Constraint->SetOrientationDriveTwistAndSwing(false, false);
    Constraint->SetLinearPositionDrive(false, false, false);
    Constraint->SetLinearVelocityDrive(false, false, false);

    switch (Mode)
    {
        case ERiftJointMode::Weld:
            Constraint->SetLinearXLimit(LCM_Locked, 0.0f);
            Constraint->SetLinearYLimit(LCM_Locked, 0.0f);
            Constraint->SetLinearZLimit(LCM_Locked, 0.0f);
            Constraint->SetAngularTwistLimit(ACM_Locked, 0.0f);
            Constraint->SetAngularSwing1Limit(ACM_Locked, 0.0f);
            Constraint->SetAngularSwing2Limit(ACM_Locked, 0.0f);
            break;

        case ERiftJointMode::Hinge:
            Constraint->SetLinearXLimit(LCM_Locked, 0.0f);
            Constraint->SetLinearYLimit(LCM_Locked, 0.0f);
            Constraint->SetLinearZLimit(LCM_Locked, 0.0f);
            Constraint->SetAngularTwistLimit(ACM_Free, 0.0f);
            Constraint->SetAngularSwing1Limit(ACM_Locked, 0.0f);
            Constraint->SetAngularSwing2Limit(ACM_Locked, 0.0f);
            Constraint->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
            Constraint->SetAngularVelocityDriveTwistAndSwing(bSignal, false);
            Constraint->SetAngularDriveParams(0.0f, MotorStrength, MotorStrength * 0.02f);
            Constraint->SetAngularVelocityTarget(FVector(MotorTargetRPM, 0.0f, 0.0f));
            break;

        case ERiftJointMode::Slider:
            Constraint->SetLinearXLimit(LCM_Limited, SliderTravel);
            Constraint->SetLinearYLimit(LCM_Locked, 0.0f);
            Constraint->SetLinearZLimit(LCM_Locked, 0.0f);
            Constraint->SetAngularTwistLimit(ACM_Locked, 0.0f);
            Constraint->SetAngularSwing1Limit(ACM_Locked, 0.0f);
            Constraint->SetAngularSwing2Limit(ACM_Locked, 0.0f);
            Constraint->SetLinearPositionDrive(true, false, false);
            Constraint->SetLinearDriveParams(SliderStrength, SliderStrength * 0.12f, 0.0f);
            Constraint->SetLinearPositionTarget(FVector(bSignal ? SliderTravel : -SliderTravel, 0.0f, 0.0f));
            break;

        case ERiftJointMode::RopeWinch:
            Constraint->SetLinearXLimit(LCM_Limited, RopeLength);
            Constraint->SetLinearYLimit(LCM_Limited, RopeLength);
            Constraint->SetLinearZLimit(LCM_Limited, RopeLength);
            Constraint->SetAngularTwistLimit(ACM_Free, 0.0f);
            Constraint->SetAngularSwing1Limit(ACM_Free, 0.0f);
            Constraint->SetAngularSwing2Limit(ACM_Free, 0.0f);
            break;
    }
}

void ARiftEngineeringJoint::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Constraint)
    {
        return;
    }

    if (Mode == ERiftJointMode::RopeWinch && bSignal)
    {
        RopeLength = FMath::Max(MinimumRopeLength, RopeLength - WinchSpeed * DeltaSeconds);
        Constraint->SetLinearXLimit(LCM_Limited, RopeLength);
        Constraint->SetLinearYLimit(LCM_Limited, RopeLength);
        Constraint->SetLinearZLimit(LCM_Limited, RopeLength);
    }
    else if (Mode == ERiftJointMode::Hinge)
    {
        Constraint->SetAngularVelocityDriveTwistAndSwing(bSignal, false);
    }
    else if (Mode == ERiftJointMode::Slider)
    {
        Constraint->SetLinearPositionTarget(FVector(bSignal ? SliderTravel : -SliderTravel, 0.0f, 0.0f));
    }
}

void ARiftEngineeringJoint::SetSignal(bool bNewSignal)
{
    bSignal = bNewSignal;
    ConfigureConstraint();
}

void ARiftEngineeringJoint::ReverseMotor()
{
    MotorTargetRPM *= -1.0f;
    ConfigureConstraint();
}

FText ARiftEngineeringJoint::GetInteractionText_Implementation() const
{
    const UEnum* Enum = StaticEnum<ERiftJointMode>();
    const FString Name = Enum ? Enum->GetNameStringByValue(static_cast<int64>(Mode)) : TEXT("Joint");
    return FText::FromString(FString::Printf(TEXT("[E] %s  |  SIGNAL %s"), *Name, bSignal ? TEXT("ON") : TEXT("OFF")));
}

void ARiftEngineeringJoint::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    SetSignal(!bSignal);
}
