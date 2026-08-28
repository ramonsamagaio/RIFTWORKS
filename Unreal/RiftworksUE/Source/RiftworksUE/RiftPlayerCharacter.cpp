#include "RiftPlayerCharacter.h"
#include "RiftGameplayActors.h"
#include "RiftPersistence.h"

#include "Animation/AnimSequenceBase.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"

namespace RiftPlayerPrivate
{
    ERiftAssemblyPartType ToAssemblyType(ERiftBuildPiece Piece)
    {
        switch (Piece)
        {
            case ERiftBuildPiece::Beam: return ERiftAssemblyPartType::Beam;
            case ERiftBuildPiece::Wheel: return ERiftAssemblyPartType::Wheel;
            case ERiftBuildPiece::MotorWheel: return ERiftAssemblyPartType::MotorWheel;
            case ERiftBuildPiece::Platform:
            default: return ERiftAssemblyPartType::Platform;
        }
    }
}

ARiftPlayerCharacter::ARiftPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    // Keep the flashlight deliberately simple: one physically plausible spotlight.
    // Volumetric scattering is intentionally very low so the player sees the world,
    // not a giant white cone hanging in front of the camera.
    Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("PremiumFlashlight"));
    Flashlight->SetupAttachment(FirstPersonCamera);
    Flashlight->SetRelativeLocation(FVector(14.0f, 9.0f, -8.0f));
    Flashlight->SetRelativeRotation(FRotator(-0.8f, 0.3f, 0.0f));
    Flashlight->IntensityUnits = ELightUnits::Lumens;
    Flashlight->SetIntensity(1550.0f);
    Flashlight->SetAttenuationRadius(5200.0f);
    Flashlight->SetInnerConeAngle(13.0f);
    Flashlight->SetOuterConeAngle(24.0f);
    Flashlight->SetSourceRadius(1.4f);
    Flashlight->SetSoftSourceRadius(3.0f);
    Flashlight->bUseInverseSquaredFalloff = true;
    Flashlight->CastShadows = true;
    Flashlight->SetVolumetricScatteringIntensity(0.12f);
    Flashlight->bUseTemperature = true;
    Flashlight->SetTemperature(5000.0f);

    MuzzleFlash = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlash"));
    MuzzleFlash->SetupAttachment(FirstPersonCamera);
    MuzzleFlash->SetRelativeLocation(FVector(55.0f, 12.0f, -12.0f));
    MuzzleFlash->IntensityUnits = ELightUnits::Lumens;
    MuzzleFlash->SetIntensity(0.0f);
    MuzzleFlash->SetAttenuationRadius(500.0f);
    MuzzleFlash->SetSourceRadius(3.0f);
    MuzzleFlash->SetLightColor(FColor(255, 177, 94));
    MuzzleFlash->SetVolumetricScatteringIntensity(0.15f);
    MuzzleFlash->CastShadows = true;

    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 205.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1500.0f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

    GetMesh()->SetOwnerNoSee(true);
    GetMesh()->SetCastHiddenShadow(true);
    GetMesh()->bCastDynamicShadow = true;
}

void ARiftPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    FirstPersonCamera->SetFieldOfView(FieldOfView);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    if (GetMesh())
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    }
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

    if (bBuildMode)
    {
        UpdateBuildPreview();
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
    PlayerInputComponent->BindAction(TEXT("SaveGame"), IE_Pressed, this, &ARiftPlayerCharacter::SavePressed);
    PlayerInputComponent->BindAction(TEXT("LoadGame"), IE_Pressed, this, &ARiftPlayerCharacter::LoadPressed);

    PlayerInputComponent->BindAction(TEXT("BuildToggle"), IE_Pressed, this, &ARiftPlayerCharacter::BuildTogglePressed);
    PlayerInputComponent->BindAction(TEXT("BuildNext"), IE_Pressed, this, &ARiftPlayerCharacter::BuildNextPressed);
    PlayerInputComponent->BindAction(TEXT("BuildPrev"), IE_Pressed, this, &ARiftPlayerCharacter::BuildPrevPressed);
    PlayerInputComponent->BindAction(TEXT("BuildRotate"), IE_Pressed, this, &ARiftPlayerCharacter::BuildRotatePressed);
    PlayerInputComponent->BindAction(TEXT("BuildAnchor"), IE_Pressed, this, &ARiftPlayerCharacter::BuildAnchorPressed);
    PlayerInputComponent->BindAction(TEXT("BuildPlace"), IE_Pressed, this, &ARiftPlayerCharacter::BuildPlacePressed);
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
    if (bIsCrouched)
    {
        return;
    }
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
    if (bBuildMode)
    {
        const UEnum* Enum = StaticEnum<ERiftBuildPiece>();
        const FString Name = Enum ? Enum->GetNameStringByValue(static_cast<int64>(SelectedBuildPiece)) : TEXT("Piece");
        CurrentInteractionText = FText::FromString(FString::Printf(
            TEXT("BUILD: %s | RMB place | wheel piece | R rotate | Q anchor %s | B exit"),
            *Name,
            bBuildAnchored ? TEXT("ON") : TEXT("OFF")));
        return;
    }

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
    if (bBuildMode || !FirstPersonCamera || !GetWorld())
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
    if (bBuildMode || !FirstPersonCamera || !GetWorld())
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

    MuzzleFlash->SetIntensity(2600.0f);
    GetWorldTimerManager().ClearTimer(MuzzleFlashTimer);
    GetWorldTimerManager().SetTimer(MuzzleFlashTimer, this, &ARiftPlayerCharacter::EndMuzzleFlash, 0.045f, false);

    if (Hit.GetActor())
    {
        UGameplayStatics::ApplyPointDamage(Hit.GetActor(), RifleDamage, Direction, Hit, GetController(), this, nullptr);
    }

    UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 6500.0f, TEXT("Gunshot"));

    if (bUseSingleNodeAnimationFallback && PistolShootAnimation && GetMesh())
    {
        bAttackAnimationLocked = true;
        CurrentFallbackAnimation = PistolShootAnimation;
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
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
        Desired = WalkAnimation ? WalkAnimation.Get() : RunAnimation.Get();
    }
    else
    {
        Desired = RunAnimation ? RunAnimation.Get() : WalkAnimation.Get();
    }

    if (Desired && Desired != CurrentFallbackAnimation)
    {
        CurrentFallbackAnimation = Desired;
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->PlayAnimation(Desired, true);
    }
}

void ARiftPlayerCharacter::ToggleCrouch()
{
    if (bIsCrouched)
    {
        UnCrouch();
        FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    }
    else
    {
        Crouch();
        FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 42.0f));
        GetCharacterMovement()->MaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeedCrouched;
    }
}

void ARiftPlayerCharacter::BuildTogglePressed()
{
    ToggleBuildMode();
}

void ARiftPlayerCharacter::BuildNextPressed()
{
    if (bBuildMode)
    {
        CycleBuildPiece(1);
    }
}

void ARiftPlayerCharacter::BuildPrevPressed()
{
    if (bBuildMode)
    {
        CycleBuildPiece(-1);
    }
}

void ARiftPlayerCharacter::BuildRotatePressed()
{
    if (bBuildMode)
    {
        RotateBuildPreview();
    }
}

void ARiftPlayerCharacter::BuildAnchorPressed()
{
    if (bBuildMode)
    {
        ToggleBuildAnchor();
    }
}

void ARiftPlayerCharacter::BuildPlacePressed()
{
    if (bBuildMode)
    {
        PlaceBuildPiece();
    }
}

void ARiftPlayerCharacter::ToggleBuildMode()
{
    bBuildMode = !bBuildMode;
    if (bBuildMode)
    {
        RecreateBuildPreview();
    }
    else if (BuildPreview)
    {
        BuildPreview->Destroy();
        BuildPreview = nullptr;
    }
    BP_OnBuildModeChanged(bBuildMode, SelectedBuildPiece, bBuildAnchored);
}

void ARiftPlayerCharacter::CycleBuildPiece(int32 Direction)
{
    int32 Value = static_cast<int32>(SelectedBuildPiece);
    Value = (Value + (Direction >= 0 ? 1 : -1) + 4) % 4;
    SelectedBuildPiece = static_cast<ERiftBuildPiece>(Value);
    RecreateBuildPreview();
    BP_OnBuildModeChanged(bBuildMode, SelectedBuildPiece, bBuildAnchored);
}

void ARiftPlayerCharacter::RotateBuildPreview()
{
    BuildYaw = FMath::Fmod(BuildYaw + BuildRotationStep, 360.0f);
    if (BuildPreview)
    {
        BuildPreview->SetActorRotation(FRotator(0.0f, BuildYaw, 0.0f));
    }
}

void ARiftPlayerCharacter::ToggleBuildAnchor()
{
    bBuildAnchored = !bBuildAnchored;
    BP_OnBuildModeChanged(bBuildMode, SelectedBuildPiece, bBuildAnchored);
}

void ARiftPlayerCharacter::RecreateBuildPreview()
{
    if (!GetWorld() || !bBuildMode)
    {
        return;
    }
    if (BuildPreview)
    {
        BuildPreview->Destroy();
        BuildPreview = nullptr;
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    BuildPreview = GetWorld()->SpawnActor<ARiftAssemblyPart>(ARiftAssemblyPart::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params);
    if (BuildPreview)
    {
        BuildPreview->PartType = RiftPlayerPrivate::ToAssemblyType(SelectedBuildPiece);
        BuildPreview->ConfigurePart();
        BuildPreview->SetActorEnableCollision(false);
        if (BuildPreview->PhysicsMesh)
        {
            BuildPreview->PhysicsMesh->SetSimulatePhysics(false);
            BuildPreview->PhysicsMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
    UpdateBuildPreview();
}

void ARiftPlayerCharacter::UpdateBuildPreview()
{
    if (!BuildPreview || !FirstPersonCamera || !GetWorld())
    {
        return;
    }

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * BuildDistance;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RiftBuildTrace), false, this);
    Params.AddIgnoredActor(BuildPreview);
    if (CarriedSalvage)
    {
        Params.AddIgnoredActor(CarriedSalvage);
    }

    FVector Target = End;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        Target = Hit.ImpactPoint + Hit.ImpactNormal * 12.0f;
    }
    Target.X = FMath::GridSnap(Target.X, BuildGridSize);
    Target.Y = FMath::GridSnap(Target.Y, BuildGridSize);
    Target.Z = FMath::GridSnap(Target.Z, BuildGridSize);
    BuildPreview->SetActorLocationAndRotation(Target, FRotator(0.0f, BuildYaw, 0.0f));
}

bool ARiftPlayerCharacter::PlaceBuildPiece()
{
    if (!bBuildMode || !BuildPreview || !GetWorld())
    {
        return false;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ARiftAssemblyPart* Part = GetWorld()->SpawnActor<ARiftAssemblyPart>(ARiftAssemblyPart::StaticClass(), BuildPreview->GetActorTransform(), Params);
    if (!Part)
    {
        return false;
    }
    Part->PartType = RiftPlayerPrivate::ToAssemblyType(SelectedBuildPiece);
    Part->ConfigurePart();
    Part->SetActorEnableCollision(true);
    if (Part->PhysicsMesh)
    {
        Part->PhysicsMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Part->PhysicsMesh->SetSimulatePhysics(!bBuildAnchored);
    }
    RecreateBuildPreview();
    return true;
}

void ARiftPlayerCharacter::DropHeavyPressed()
{
    DropHeavySalvage();
}

void ARiftPlayerCharacter::SecureHeavyPressed()
{
    SecureHeavySalvage();
}

void ARiftPlayerCharacter::SavePressed()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URiftPersistenceSubsystem* Persistence = GameInstance->GetSubsystem<URiftPersistenceSubsystem>())
        {
            Persistence->SaveRiftGame(this);
        }
    }
}

void ARiftPlayerCharacter::LoadPressed()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URiftPersistenceSubsystem* Persistence = GameInstance->GetSubsystem<URiftPersistenceSubsystem>())
        {
            Persistence->LoadRiftGame(this);
        }
    }
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
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URiftPersistenceSubsystem* Persistence = GameInstance->GetSubsystem<URiftPersistenceSubsystem>())
        {
            Persistence->MarkSalvageRemoved(CarriedSalvage->PersistentId);
        }
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
        if (bBuildMode)
        {
            ToggleBuildMode();
        }
        ARiftBaseBeacon* Base = FindNearbyBase(100000000.0f);
        SetActorLocation(Base ? Base->GetActorLocation() + FVector(0.0f, 0.0f, 180.0f) : FVector(0.0f, 0.0f, 220.0f));
        GetCharacterMovement()->StopMovementImmediately();
        Health = 100.0f;
    }
    return DamageAmount;
}
