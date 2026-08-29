#include "RiftEngineeringPlayer.h"

#include "Components/InputComponent.h"

void ARiftEngineeringPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent)
    {
        return;
    }

    PlayerInputComponent->BindAction(TEXT("UtilityBuildNext"), IE_Pressed, this, &ARiftEngineeringPlayerCharacter::UtilityBuildNextInput);
    PlayerInputComponent->BindAction(TEXT("UtilityBuildPlace"), IE_Pressed, this, &ARiftEngineeringPlayerCharacter::UtilityBuildPlaceInput);
    PlayerInputComponent->BindAction(TEXT("LogicConnect"), IE_Pressed, this, &ARiftEngineeringPlayerCharacter::LogicConnectInput);
}

void ARiftEngineeringPlayerCharacter::UtilityBuildNextInput()
{
    UtilityBuildNextPressed();
}

void ARiftEngineeringPlayerCharacter::UtilityBuildPlaceInput()
{
    UtilityBuildPlacePressed();
}

void ARiftEngineeringPlayerCharacter::LogicConnectInput()
{
    LogicConnectPressed();
}
