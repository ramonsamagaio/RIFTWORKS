#include "RiftLogicNode.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

ARiftLogicNode::ARiftLogicNode()
{
    PrimaryActorTick.bCanEverTick = true;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LogicNodeMesh"));
    SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        Mesh->SetStaticMesh(Cube.Object);
    }
    Mesh->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.18f));
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));

    Sensor = CreateDefaultSubobject<USphereComponent>(TEXT("Sensor"));
    Sensor->SetupAttachment(Mesh);
    Sensor->SetSphereRadius(SensorRadius);
    Sensor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sensor->SetCollisionResponseToAllChannels(ECR_Ignore);
    Sensor->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    Indicator = CreateDefaultSubobject<UPointLightComponent>(TEXT("Indicator"));
    Indicator->SetupAttachment(Mesh);
    Indicator->SetRelativeLocation(FVector(0.0f, 0.0f, 35.0f));
    Indicator->IntensityUnits = ELightUnits::Lumens;
    Indicator->AttenuationRadius = 160.0f;
    Indicator->VolumetricScatteringIntensity = 0.0f;
}

void ARiftLogicNode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Sensor->SetSphereRadius(SensorRadius);

    if (Mode == ERiftLogicMode::ProximitySensor)
    {
        APawn* Pawn = GetWorld() && GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
        const bool bDetected = Pawn && Sensor->IsOverlappingActor(Pawn);
        if (bDetected != bSignal)
        {
            SetSignal(bDetected);
        }
    }
    else if (Mode == ERiftLogicMode::TimerPulse)
    {
        TimerAccumulator += DeltaSeconds;
        if (TimerAccumulator >= FMath::Max(0.05f, TimerSeconds))
        {
            TimerAccumulator = 0.0f;
            ToggleSignal();
        }
    }
    RefreshIndicator();
}

void ARiftLogicNode::SetSignal(bool bNewSignal)
{
    if (bSignal == bNewSignal)
    {
        RefreshIndicator();
        return;
    }
    bSignal = bNewSignal;
    BroadcastSignal();
    RefreshIndicator();
}

void ARiftLogicNode::ToggleSignal()
{
    SetSignal(!bSignal);
}

void ARiftLogicNode::ConnectReceiver(AActor* Receiver)
{
    if (Receiver && Receiver != this)
    {
        Receivers.AddUnique(Receiver);
        SendSignal(Receiver, bInvertOutput ? !bSignal : bSignal);
    }
}

void ARiftLogicNode::DisconnectReceiver(AActor* Receiver)
{
    Receivers.Remove(Receiver);
}

void ARiftLogicNode::BroadcastSignal()
{
    const bool Output = bInvertOutput ? !bSignal : bSignal;
    for (AActor* Receiver : Receivers)
    {
        SendSignal(Receiver, Output);
    }
}

void ARiftLogicNode::SendSignal(AActor* Receiver, bool Value)
{
    if (!IsValid(Receiver))
    {
        return;
    }
    UFunction* Function = Receiver->FindFunction(TEXT("SetSignal"));
    if (!Function)
    {
        return;
    }
    struct FSignalParams
    {
        bool bSignalValue;
    };
    FSignalParams Params{Value};
    Receiver->ProcessEvent(Function, &Params);
}

void ARiftLogicNode::RefreshIndicator()
{
    if (!Indicator)
    {
        return;
    }
    if (bSignal)
    {
        Indicator->SetLightColor(FLinearColor(0.12f, 0.95f, 0.36f));
        Indicator->SetIntensity(160.0f);
    }
    else
    {
        Indicator->SetLightColor(FLinearColor(0.7f, 0.12f, 0.05f));
        Indicator->SetIntensity(45.0f);
    }
}

FText ARiftLogicNode::GetInteractionText_Implementation() const
{
    const UEnum* Enum = StaticEnum<ERiftLogicMode>();
    const FString ModeName = Enum ? Enum->GetNameStringByValue(static_cast<int64>(Mode)) : TEXT("Logic");
    if (Mode == ERiftLogicMode::ToggleButton)
    {
        return FText::FromString(FString::Printf(TEXT("[E] %s  |  %s  |  %d receivers"), *ModeName, bSignal ? TEXT("ON") : TEXT("OFF"), Receivers.Num()));
    }
    return FText::FromString(FString::Printf(TEXT("%s  |  %s  |  %d receivers"), *ModeName, bSignal ? TEXT("ACTIVE") : TEXT("IDLE"), Receivers.Num()));
}

void ARiftLogicNode::Interact_Implementation(ARiftPlayerCharacter* Player)
{
    if (Mode == ERiftLogicMode::ToggleButton)
    {
        ToggleSignal();
    }
}
