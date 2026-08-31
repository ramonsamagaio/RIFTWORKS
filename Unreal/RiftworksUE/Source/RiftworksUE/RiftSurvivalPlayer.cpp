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

    // Battery drain is intentionally disabled while the flashlight rendering
    // path is being proven. A survival mechanic must never masquerade as a
    // rendering bug. Re-enable only after the light is completely stable.
    FlashlightDrainPerSecond = 0.0f;
    FlashlightBattery = 100.0f;
    bFlashlightOn = true;
}

void ARiftSurvivalPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // The first-person hidden body must never occlude a camera light.
    if (GetMesh())
    {
        GetMesh()->SetCastHiddenShadow(false);
        GetMesh()->SetCastShadow(false);
        GetMesh()->bCastDynamicShadow = false;
    }

    FlashlightBattery = 100.0f;
    bFlashlightOn = true;

    if (Flashlight && FirstPersonCamera)
    {
        Flashlight->AttachToComponent(FirstPersonCamera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        Flashlight->SetMobility(EComponentMobility::Movable);

        // Keep the source almost on the optical axis. A large forward offset can
        // put a spotlight inside a wall when the camera is close to geometry,
        // which looks exactly like direction-dependent flicker.
        Flashlight->SetRelativeLocation(FVector(3.0f, 7.0f, -5.0f));
        Flashlight->SetRelativeRotation(FRotator(-0.35f, 0.0f, 0.0f));
        Flashlight->SetIntensity(2350.0f);
        Flashlight->SetAttenuationRadius(6800.0f);
        Flashlight->SetInnerConeAngle(15.0f);
        Flashlight->SetOuterConeAngle(30.0f);
        Flashlight->SetSourceRadius(1.0f);
        Flashlight->SetSoftSourceRadius(3.5f);
        Flashlight->SetUseInverseSquaredFalloff(true);
        Flashlight->SetVolumetricScatteringIntensity(0.018f);
        Flashlight->bUseTemperature = true;
        Flashlight->SetTemperature(5000.0f);
        Flashlight->CastShadows = true;
        Flashlight->SetVisibility(true, false);
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

    // Prototype guarantee: battery cannot silently turn the light off. The user
    // can still explicitly toggle it with the flashlight input.
    FlashlightBattery = 100.0f;
    Flashlight->SetMobility(EComponentMobility::Movable);

    const bool bShouldBeVisible = bFlashlightOn;
    if (Flashlight->IsVisible() != bShouldBeVisible)
    {
        Flashlight->SetVisibility(bShouldBeVisible, false);
    }

    if (bShouldBeVisible)
    {
        Flashlight->SetIntensity(2350.0f);
        Flashlight->SetAttenuationRadius(6800.0f);
        Flashlight->SetInnerConeAngle(15.0f);
        Flashlight->SetOuterConeAngle(30.0f);
        Flashlight->SetVolumetricScatteringIntensity(0.018f);
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
