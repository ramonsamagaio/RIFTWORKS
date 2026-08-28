#include "RiftPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"

ARiftPlayerCharacter::ARiftPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("PremiumFlashlight"));
    Flashlight->SetupAttachment(FirstPersonCamera);
    Flashlight->SetRelativeLocation(FVector(18.0f, 11.0f, -10.0f));
    Flashlight->SetRelativeRotation(FRotator(-1.5f, 0.7f, 0.0f));
    Flashlight->IntensityUnits = ELightUnits::Lumens;
    Flashlight->Intensity = 3400.0f;
    Flashlight->AttenuationRadius = 3600.0f;
    Flashlight->InnerConeAngle = 9.0f;
    Flashlight->OuterConeAngle = 27.0f;
    Flashlight->SourceRadius = 2.5f;
    Flashlight->SoftSourceRadius = 5.5f;
    Flashlight->bUseInverseSquaredFalloff = true;
    Flashlight->CastShadows = true;
    Flashlight->VolumetricScatteringIntensity = 2.2f;
    Flashlight->bUseTemperature = true;
    Flashlight->Temperature = 4450.0f;

    MuzzleFlash = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlash"));
    MuzzleFlash->SetupAttachment(FirstPersonCamera);
    MuzzleFlash->SetRelativeLocation(FVector(55.0f, 12.0f, -12.0f));
    MuzzleFlash->IntensityUnits = ELightUnits::Lumens;
    MuzzleFlash->Intensity = 0.0f;
    MuzzleFlash->AttenuationRadius = 550.0f;
    MuzzleFlash->SourceRadius = 4.0f;
    MuzzleFlash->LightColor = FColor(255, 177, 94);
    MuzzleFlash->VolumetricScatteringIntensity = 2.5f;
    MuzzleFlash->CastShadows = true;

    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 220.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1500.0f;
    GetCharacterMovement()->AirControl = 0.35f;

    GetMesh()->SetOwnerNoSee(true);
    GetMesh()->SetCastHiddenShadow(true);
    GetMesh()->bCastDynamicShadow = true;
}

void ARiftPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    FirstPersonCamera->SetFieldOfView(FieldOfView);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    SetFlashlightEnabled(bFlashlightOn && FlashlightBattery > 0.0f);
}

void ARiftPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bFlashlightOn)
    {
        FlashlightBattery = FMath::Max(0.0f, FlashlightBattery - FlashlightDrainPerSecond * DeltaSeconds);
        if (FlashlightBattery <= 0.0f)
        {
            SetFlashlightEnabled(false);
        }
    }

    UpdateInteractionTrace();
}

void ARiftPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ARiftPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ARiftPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ARiftPlayerCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ARiftPlayerCharacter::LookUp);

    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &ARiftPlayerCharacter::StartSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &ARiftPlayerCharacter::StopSprint);
    PlayerInputComponent->BindAction(TEXT("Flashlight"), IE_Pressed, this, &ARiftPlayerCharacter::ToggleFlashlight);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ARiftPlayerCharacter::InteractPressed);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &ARiftPlayerCharacter::FirePressed);
    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &ARiftPlayerCharacter::ToggleCrouch);
}

void ARiftPlayerCharacter::MoveForward(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void ARiftPlayerCharacter::MoveRight(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void ARiftPlayerCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void ARiftPlayerCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void ARiftPlayerCharacter::StartSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void ARiftPlayerCharacter::StopSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ARiftPlayerCharacter::ToggleFlashlight()
{
    SetFlashlightEnabled(!bFlashlightOn && FlashlightBattery > 0.0f);
}

void ARiftPlayerCharacter::SetFlashlightEnabled(bool bEnabled)
{
    bFlashlightOn = bEnabled && FlashlightBattery > 0.0f;
    Flashlight->SetVisibility(bFlashlightOn, true);
    BP_OnFlashlightChanged(bFlashlightOn, FlashlightBattery);
}

void ARiftPlayerCharacter::UpdateInteractionTrace()
{
    CurrentInteractionText = FText::GetEmpty();
    if (!FirstPersonCamera || !GetWorld())
    {
        return;
    }

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * 450.0f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftInteraction), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.GetActor() && Hit.GetActor()->Implements<URiftInteractable>())
    {
        CurrentInteractionText = IRiftInteractable::Execute_GetInteractionText(Hit.GetActor());
    }
}

void ARiftPlayerCharacter::InteractPressed()
{
    if (!FirstPersonCamera || !GetWorld())
    {
        return;
    }

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * 450.0f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftInteract), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.GetActor() && Hit.GetActor()->Implements<URiftInteractable>())
    {
        IRiftInteractable::Execute_Interact(Hit.GetActor(), this);
    }
}

void ARiftPlayerCharacter::FirePressed()
{
    if (!FirstPersonCamera || !GetWorld())
    {
        return;
    }

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector Direction = FirstPersonCamera->GetForwardVector();
    const FVector End = Start + Direction * RifleRange;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftFire), true, this);
    GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

    MuzzleFlash->SetIntensity(5200.0f);
    GetWorldTimerManager().ClearTimer(MuzzleFlashTimer);
    GetWorldTimerManager().SetTimer(MuzzleFlashTimer, this, &ARiftPlayerCharacter::EndMuzzleFlash, 0.055f, false);

    if (Hit.GetActor())
    {
        UGameplayStatics::ApplyPointDamage(Hit.GetActor(), RifleDamage, Direction, Hit, GetController(), this, nullptr);
    }

    UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 6500.0f, TEXT("Gunshot"));
    BP_OnWeaponFired(Hit);
}

void ARiftPlayerCharacter::EndMuzzleFlash()
{
    if (MuzzleFlash)
    {
        MuzzleFlash->SetIntensity(0.0f);
    }
}

void ARiftPlayerCharacter::ToggleCrouch()
{
    if (bIsCrouched)
    {
        UnCrouch();
    }
    else
    {
        Crouch();
    }
}

void ARiftPlayerCharacter::AddComponentItem(FName ItemId, int32 Amount)
{
    Components.FindOrAdd(ItemId) += FMath::Max(0, Amount);
    BP_OnInventoryChanged();
}

int32 ARiftPlayerCharacter::GetComponentCount(FName ItemId) const
{
    if (const int32* Count = Components.Find(ItemId))
    {
        return *Count;
    }
    return 0;
}

bool ARiftPlayerCharacter::ConsumeComponentItem(FName ItemId, int32 Amount)
{
    int32* Count = Components.Find(ItemId);
    if (!Count || Amount <= 0 || *Count < Amount)
    {
        return false;
    }
    *Count -= Amount;
    BP_OnInventoryChanged();
    return true;
}
