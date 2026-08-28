#include "RiftPlayerCharacter.h"
#include "RiftGameplayActors.h"

#include "Animation/AnimSequenceBase.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
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
    UpdateFallbackAnimation();
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
    PlayerInputComponent->BindAction(TEXT("DropHeavy"), IE_Pressed, this, &ARiftPlayerCharacter::DropHeavyPressed);
    PlayerInputComponent->BindAction(TEXT("SecureHeavy"), IE_Pressed, this, &ARiftPlayerCharacter::SecureHeavyPressed);
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
    GetCharacterMovement()->MaxWalkSpeed = CarriedSalvage ? WalkSpeed * 0.72f : SprintSpeed;
}

void ARiftPlayerCharacter::StopSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = CarriedSalvage ? WalkSpeed * 0.72f : WalkSpeed;
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
    if (CarriedSalvage)
    {
        Params.AddIgnoredActor(CarriedSalvage);
    }
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
    if (CarriedSalvage)
    {
        Params.AddIgnoredActor(CarriedSalvage);
    }
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
    if (CarriedSalvage)
    {
        Params.AddIgnoredActor(CarriedSalvage);
    }
    GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

    MuzzleFlash->SetIntensity(5200.0f);
    GetWorldTimerManager().ClearTimer(MuzzleFlashTimer);
    GetWorldTimerManager().SetTimer(MuzzleFlashTimer, this, &ARiftPlayerCharacter::EndMuzzleFlash, 0.055f, false);

    if (Hit.GetActor())
    {
        UGameplayStatics::ApplyPointDamage(Hit.GetActor(), RifleDamage, Direction, Hit, GetController(), this, nullptr);
    }

    UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 6500.0f, TEXT("Gunshot"));

    if (bUseSingleNodeAnimationFallback && PistolShootAnimation && GetMesh())
    {
        bAttackAnimationLocked = true;
        CurrentFallbackAnimation = PistolShootAnimation;
        GetMesh()->PlayAnimation(PistolShootAnimation, false);
        GetWorldTimerManager().ClearTimer(AttackAnimationTimer);
        GetWorldTimerManager().SetTimer(AttackAnimationTimer, this, &ARiftPlayerCharacter::EndAttackAnimation, FMath::Max(0.15f, PistolShootAnimation->GetPlayLength()), false);
    }

    BP_OnWeaponFired(Hit);
}

void ARiftPlayerCharacter::EndMuzzleFlash()
{
    if (MuzzleFlash)
    {
        MuzzleFlash->SetIntensity(0.0f);
    }
}

void ARiftPlayerCharacter::EndAttackAnimation()
{
    bAttackAnimationLocked = false;
    CurrentFallbackAnimation = nullptr;
}

void ARiftPlayerCharacter::UpdateFallbackAnimation()
{
    if (!bUseSingleNodeAnimationFallback || bAttackAnimationLocked || !GetMesh())
    {
        return;
    }

    UAnimSequenceBase* Desired = nullptr;
    const float Speed = GetVelocity().Size2D();
    if (bIsCrouched && CrouchAnimation)
    {
        Desired = CrouchAnimation;
    }
    else if (Speed < 10.0f)
    {
        Desired = IdleAnimation;
    }
    else if (Speed < WalkSpeed * 1.15f)
    {
        Desired = WalkAnimation;
    }
    else
    {
        Desired = RunAnimation;
    }

    if (Desired && Desired != CurrentFallbackAnimation)
    {
        CurrentFallbackAnimation = Desired;
        GetMesh()->PlayAnimation(Desired, true);
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

void ARiftPlayerCharacter::DropHeavyPressed()
{
    DropHeavySalvage();
}

void ARiftPlayerCharacter::SecureHeavyPressed()
{
    SecureHeavySalvage();
}

bool ARiftPlayerCharacter::TryCarrySalvage(ARiftSalvageActor* Salvage)
{
    if (!Salvage || !Salvage->bHeavy || CarriedSalvage)
    {
        return false;
    }
    CarriedSalvage = Salvage;
    Salvage->SetCarriedState(true);
    Salvage->AttachToComponent(FirstPersonCamera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    Salvage->SetActorRelativeLocation(FVector(150.0f, 0.0f, -45.0f));
    Salvage->SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 0.72f;
    BP_OnCarriedSalvageChanged(CarriedSalvage);
    return true;
}

void ARiftPlayerCharacter::DropHeavySalvage()
{
    if (!CarriedSalvage)
    {
        return;
    }
    ARiftSalvageActor* Dropped = CarriedSalvage;
    Dropped->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Dropped->SetActorLocation(GetActorLocation() + FirstPersonCamera->GetForwardVector() * 180.0f + FVector(0.0f, 0.0f, 45.0f));
    Dropped->SetCarriedState(false);
    if (Dropped->Mesh && Dropped->Mesh->IsSimulatingPhysics())
    {
        Dropped->Mesh->SetPhysicsLinearVelocity(FirstPersonCamera->GetForwardVector() * 80.0f);
    }
    CarriedSalvage = nullptr;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    BP_OnCarriedSalvageChanged(nullptr);
}

bool ARiftPlayerCharacter::SecureHeavySalvage()
{
    if (!CarriedSalvage)
    {
        return false;
    }
    ARiftBaseBeacon* Base = FindNearbyBase(1400.0f);
    if (!Base)
    {
        return false;
    }
    Base->StoreItem(CarriedSalvage->ItemId, CarriedSalvage->Amount);
    CarriedSalvage->Destroy();
    CarriedSalvage = nullptr;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    BP_OnCarriedSalvageChanged(nullptr);
    return true;
}

ARiftBaseBeacon* ARiftPlayerCharacter::FindNearbyBase(float Radius) const
{
    if (!GetWorld())
    {
        return nullptr;
    }
    ARiftBaseBeacon* Best = nullptr;
    float BestDistanceSquared = FMath::Square(Radius);
    for (TActorIterator<ARiftBaseBeacon> It(GetWorld()); It; ++It)
    {
        const float DistanceSquared = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
        if (DistanceSquared <= BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            Best = *It;
        }
    }
    return Best;
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

float ARiftPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (DamageAmount <= 0.0f)
    {
        return 0.0f;
    }
    Health = FMath::Max(0.0f, Health - DamageAmount);
    BP_OnDamaged(Health, DamageAmount);
    if (Health <= 0.0f)
    {
        BP_OnDied();
        DropHeavySalvage();
        ARiftBaseBeacon* Base = FindNearbyBase(100000000.0f);
        SetActorLocation(Base ? Base->GetActorLocation() + FVector(0.0f, 0.0f, 180.0f) : FVector(0.0f, 0.0f, 220.0f));
        GetCharacterMovement()->StopMovementImmediately();
        Health = 100.0f;
    }
    return DamageAmount;
}
