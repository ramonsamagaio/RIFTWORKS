#include "RiftEngineeringPlayer.h"

#include "Components/InputComponent.h"

void ARiftEngineeringPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent)
    {
        return;
    }

    PlayerInputComponent->BindAction(TEXT("UtilityBuildNext"), IE_Pressed, this, &ARiftEngineeringPlayerCharacter::UtilityBuildNextPressed);
    PlayerInputComponent->BindAction(TEXT("UtilityBuildPlace"), IE_Pressed, this, &ARiftEngineeringPlayerCharacter::UtilityBuildPlacePressed);
    PlayerInputComponent->BindAction(TEXT("LogicConnect"), IE_Pressed, this, &ARiftEngineeringPlayerCharacter::LogicConnectPressed);
}
