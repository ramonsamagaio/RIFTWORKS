#include "RiftSurvivalPlayer.h"

#include "RiftHUD.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

ARiftSurvivalPlayerCharacter::ARiftSurvivalPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARiftSurvivalPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // The first-person body used to cast a hidden shadow directly in front of a
    // camera-mounted light. Depending on pose/view direction the player could
    // literally occlude their own flashlight, which looked like flicker.
    if (GetMesh())
    {
        GetMesh()->SetCastHiddenShadow(false);
        GetMesh()->SetCastShadow(false);
    }

    if (Flashlight && FirstPersonCamera)
    {
        Flashlight->AttachToComponent(FirstPersonCamera, FAttachmentTransformRules::KeepRelativeTransform);
        Flashlight->SetRelativeLocation(FVector(18.0f, 10.0f, -7.0f));
        Flashlight->SetRelativeRotation(FRotator(-0.5f, 0.0f, 0.0f));
        Flashlight->SetIntensity(2200.0f);
        Flashlight->SetAttenuationRadius(6500.0f);
        Flashlight->SetInnerConeAngle(15.0f);
        Flashlight->SetOuterConeAngle(29.0f);
        Flashlight->SetSourceRadius(1.2f);
        Flashlight->SetSoftSourceRadius(4.0f);
        Flashlight->SetVolumetricScatteringIntensity(0.025f);
        Flashlight->CastShadows = true;
    }

    StabilizeFlashlight();
}

void ARiftSurvivalPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent)
    {
        return;
    }

    // Bind directly so this remains functional even when an old DefaultInput.ini
    // is cached by an existing Unreal project installation.
    PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ARiftSurvivalPlayerCharacter::ToggleInventoryInput);
    PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &ARiftSurvivalPlayerCharacter::ToggleInventoryInput);
}

void ARiftSurvivalPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    StabilizeFlashlight();
}

void ARiftSurvivalPlayerCharacter::StabilizeFlashlight()
{
    if (!Flashlight)
    {
        return;
    }

    const bool bShouldBeVisible = bFlashlightOn && FlashlightBattery > KINDA_SMALL_NUMBER;
    if (Flashlight->IsVisible() != bShouldBeVisible)
    {
        Flashlight->SetVisibility(bShouldBeVisible, false);
    }

    // Guard against stale Blueprint defaults overriding runtime tuning.
    if (bShouldBeVisible)
    {
        if (Flashlight->Intensity < 1500.0f)
        {
            Flashlight->SetIntensity(2200.0f);
        }
        Flashlight->SetVolumetricScatteringIntensity(0.025f);
    }
}

void ARiftSurvivalPlayerCharacter::ToggleInventoryInput()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    ARiftHUD* HUD = PC ? Cast<ARiftHUD>(PC->GetHUD()) : nullptr;
    if (HUD)
    {
        HUD->ToggleInventory();
    }
}
