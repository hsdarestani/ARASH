#include "Player/ArashCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/ArashBowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/ArashGameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AArashCharacter::AArashCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    bUseControllerRotationYaw = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = 650.0f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

    VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
    VisualRoot->SetupAttachment(RootComponent);

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyVisual"));
    VisualMesh->SetupAttachment(VisualRoot);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
    VisualMesh->SetRelativeScale3D(FVector(0.42f, 0.30f, 0.95f));
    if (CylinderAsset.Succeeded())
    {
        VisualMesh->SetStaticMesh(CylinderAsset.Object);
    }

    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadVisual"));
    HeadMesh->SetupAttachment(VisualRoot);
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 91.0f));
    HeadMesh->SetRelativeScale3D(FVector(0.25f));
    if (SphereAsset.Succeeded())
    {
        HeadMesh->SetStaticMesh(SphereAsset.Object);
    }

    MantleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MantleVisual"));
    MantleMesh->SetupAttachment(VisualRoot);
    MantleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MantleMesh->SetRelativeLocation(FVector(-4.0f, 0.0f, 52.0f));
    MantleMesh->SetRelativeScale3D(FVector(0.27f, 0.52f, 0.11f));
    if (CubeAsset.Succeeded())
    {
        MantleMesh->SetStaticMesh(CubeAsset.Object);
    }

    LeftArmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftArmVisual"));
    LeftArmMesh->SetupAttachment(VisualRoot);
    LeftArmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeftArmMesh->SetRelativeLocation(FVector(15.0f, -19.0f, 48.0f));
    LeftArmMesh->SetRelativeRotation(FRotator(0.0f, -48.0f, 0.0f));
    LeftArmMesh->SetRelativeScale3D(FVector(0.34f, 0.065f, 0.065f));
    if (CubeAsset.Succeeded())
    {
        LeftArmMesh->SetStaticMesh(CubeAsset.Object);
    }

    RightArmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightArmVisual"));
    RightArmMesh->SetupAttachment(VisualRoot);
    RightArmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RightArmMesh->SetRelativeLocation(FVector(9.0f, 18.0f, 47.0f));
    RightArmMesh->SetRelativeRotation(FRotator(0.0f, 32.0f, 0.0f));
    RightArmMesh->SetRelativeScale3D(FVector(0.30f, 0.065f, 0.065f));
    if (CubeAsset.Succeeded())
    {
        RightArmMesh->SetStaticMesh(CubeAsset.Object);
    }

    BowUpperMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BowUpperVisual"));
    BowUpperMesh->SetupAttachment(VisualRoot);
    BowUpperMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BowUpperMesh->SetRelativeLocation(FVector(20.0f, -48.0f, 43.0f));
    BowUpperMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, -11.0f));
    BowUpperMesh->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.54f));
    if (CubeAsset.Succeeded())
    {
        BowUpperMesh->SetStaticMesh(CubeAsset.Object);
    }

    BowLowerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BowLowerVisual"));
    BowLowerMesh->SetupAttachment(VisualRoot);
    BowLowerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BowLowerMesh->SetRelativeLocation(FVector(20.0f, -48.0f, -5.0f));
    BowLowerMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 11.0f));
    BowLowerMesh->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.54f));
    if (CubeAsset.Succeeded())
    {
        BowLowerMesh->SetStaticMesh(CubeAsset.Object);
    }

    BowStringUpperMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BowStringUpperVisual"));
    BowStringUpperMesh->SetupAttachment(VisualRoot);
    BowStringUpperMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BowStringUpperMesh->SetRelativeLocation(FVector(20.0f, -48.0f, 31.0f));
    BowStringUpperMesh->SetRelativeScale3D(FVector(0.012f, 0.012f, 0.34f));
    if (CubeAsset.Succeeded())
    {
        BowStringUpperMesh->SetStaticMesh(CubeAsset.Object);
    }

    BowStringLowerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BowStringLowerVisual"));
    BowStringLowerMesh->SetupAttachment(VisualRoot);
    BowStringLowerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BowStringLowerMesh->SetRelativeLocation(FVector(20.0f, -48.0f, 7.0f));
    BowStringLowerMesh->SetRelativeScale3D(FVector(0.012f, 0.012f, 0.34f));
    if (CubeAsset.Succeeded())
    {
        BowStringLowerMesh->SetStaticMesh(CubeAsset.Object);
    }

    NockedArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NockedArrowVisual"));
    NockedArrowMesh->SetupAttachment(VisualRoot);
    NockedArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NockedArrowMesh->SetRelativeLocation(FVector(45.0f, -47.0f, 19.0f));
    NockedArrowMesh->SetRelativeScale3D(FVector(0.70f, 0.025f, 0.025f));
    NockedArrowMesh->SetVisibility(false);
    if (CubeAsset.Succeeded())
    {
        NockedArrowMesh->SetStaticMesh(CubeAsset.Object);
    }

    QuiverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("QuiverVisual"));
    QuiverMesh->SetupAttachment(VisualRoot);
    QuiverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    QuiverMesh->SetRelativeLocation(FVector(-23.0f, 32.0f, 28.0f));
    QuiverMesh->SetRelativeRotation(FRotator(18.0f, 0.0f, 0.0f));
    QuiverMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.55f));
    if (CylinderAsset.Succeeded())
    {
        QuiverMesh->SetStaticMesh(CylinderAsset.Object);
    }

    if (BasicMaterial.Succeeded())
    {
        VisualMesh->SetMaterial(0, BasicMaterial.Object);
        HeadMesh->SetMaterial(0, BasicMaterial.Object);
        MantleMesh->SetMaterial(0, BasicMaterial.Object);
        LeftArmMesh->SetMaterial(0, BasicMaterial.Object);
        RightArmMesh->SetMaterial(0, BasicMaterial.Object);
        BowUpperMesh->SetMaterial(0, BasicMaterial.Object);
        BowLowerMesh->SetMaterial(0, BasicMaterial.Object);
        BowStringUpperMesh->SetMaterial(0, BasicMaterial.Object);
        BowStringLowerMesh->SetMaterial(0, BasicMaterial.Object);
        NockedArrowMesh->SetMaterial(0, BasicMaterial.Object);
        QuiverMesh->SetMaterial(0, BasicMaterial.Object);
    }

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 1050.0f;
    CameraBoom->SetUsingAbsoluteRotation(true);
    CameraBoom->SetRelativeRotation(FRotator(-58.0f, -45.0f, 0.0f));
    CameraBoom->bDoCollisionTest = false;
    CameraBoom->bUsePawnControlRotation = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;

    Bow = CreateDefaultSubobject<UArashBowComponent>(TEXT("Bow"));
}

void AArashCharacter::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;

    VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.015f, 0.24f, 0.30f));
    HeadMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.62f, 0.40f, 0.22f));
    MantleMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.025f, 0.09f, 0.16f));
    LeftArmMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.54f, 0.31f, 0.16f));
    RightArmMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.54f, 0.31f, 0.16f));
    BowUpperMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.88f, 0.55f, 0.12f));
    BowLowerMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.88f, 0.55f, 0.12f));
    BowStringUpperMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.93f, 0.84f, 0.61f));
    BowStringLowerMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.93f, 0.84f, 0.61f));
    NockedArrowMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(1.0f, 0.68f, 0.14f));
    QuiverMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.38f, 0.055f, 0.04f));

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        PC->DefaultMouseCursor = EMouseCursor::Crosshairs;
    }
}

void AArashCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bIsDead)
    {
        UpdateAim();
    }

    UpdateVisualAnimation(DeltaSeconds);
    UpdateCombatFeedback(DeltaSeconds);
}

void AArashCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AArashCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AArashCharacter::MoveRight);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AArashCharacter::StartFire);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &AArashCharacter::StopFire);
    PlayerInputComponent->BindAction(TEXT("Dodge"), IE_Pressed, this, &AArashCharacter::Dodge);
    PlayerInputComponent->BindAction(TEXT("Upgrade1"), IE_Pressed, this, &AArashCharacter::SelectUpgrade1);
    PlayerInputComponent->BindAction(TEXT("Upgrade2"), IE_Pressed, this, &AArashCharacter::SelectUpgrade2);
    PlayerInputComponent->BindAction(TEXT("Upgrade3"), IE_Pressed, this, &AArashCharacter::SelectUpgrade3);
    PlayerInputComponent->BindAction(TEXT("Restart"), IE_Pressed, this, &AArashCharacter::RestartRun);
}

void AArashCharacter::MoveForward(float Value)
{
    if (!bIsDead && !FMath::IsNearlyZero(Value))
    {
        AddMovementInput(FVector::ForwardVector, Value);
    }
}

void AArashCharacter::MoveRight(float Value)
{
    if (!bIsDead && !FMath::IsNearlyZero(Value))
    {
        AddMovementInput(FVector::RightVector, Value);
    }
}

void AArashCharacter::StartFire()
{
    if (CanUseCombat() && Bow)
    {
        Bow->StartCharge();
    }
}

void AArashCharacter::StopFire()
{
    if (CanUseCombat() && Bow)
    {
        Bow->ReleaseArrow();
    }
}

void AArashCharacter::Dodge()
{
    if (!CanUseCombat())
    {
        return;
    }

    FVector Direction = GetLastMovementInputVector();
    if (Direction.IsNearlyZero())
    {
        Direction = GetActorForwardVector();
    }

    Direction.Z = 0.0f;
    Direction.Normalize();

    DodgeVisualRemaining = 0.30f;
    const float SideDot = FVector::DotProduct(Direction, GetActorRightVector());
    DodgeVisualSide = FMath::Abs(SideDot) > 0.15f ? FMath::Sign(SideDot) : 1.0f;

    LaunchCharacter(Direction * 900.0f + FVector(0.0f, 0.0f, 90.0f), true, true);
}

void AArashCharacter::UpdateAim()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    FHitResult Hit;
    if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        return;
    }

    FVector AimDirection = Hit.ImpactPoint - GetActorLocation();
    AimDirection.Z = 0.0f;

    if (!AimDirection.IsNearlyZero())
    {
        SetActorRotation(AimDirection.Rotation());
    }
}

void AArashCharacter::UpdateVisualAnimation(float DeltaSeconds)
{
    if (!VisualRoot)
    {
        return;
    }

    VisualTime += DeltaSeconds;

    const float MaxSpeed = FMath::Max(1.0f, GetCharacterMovement()->MaxWalkSpeed);
    const float MoveAlpha = FMath::Clamp(GetVelocity().Size2D() / MaxSpeed, 0.0f, 1.0f);
    const float StepWave = FMath::Sin(VisualTime * FMath::Lerp(3.2f, 10.0f, MoveAlpha));
    const float Bob = StepWave * FMath::Lerp(0.7f, 3.0f, MoveAlpha);

    float DodgeAlpha = 0.0f;
    if (DodgeVisualRemaining > 0.0f)
    {
        constexpr float DodgeVisualDuration = 0.30f;
        const float Progress = 1.0f - FMath::Clamp(DodgeVisualRemaining / DodgeVisualDuration, 0.0f, 1.0f);
        DodgeAlpha = FMath::Sin(Progress * PI);
        DodgeVisualRemaining = FMath::Max(0.0f, DodgeVisualRemaining - DeltaSeconds);
    }

    const float ChargeAlpha = Bow ? Bow->GetChargeAlpha() : 0.0f;
    const bool bCanShowNockedArrow = Bow && !Bow->IsArrowActive() && ChargeAlpha > 0.015f;

    VisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, Bob));
    VisualRoot->SetRelativeRotation(FRotator(
        -5.0f * ChargeAlpha,
        0.0f,
        DodgeVisualSide * 18.0f * DodgeAlpha));

    const float MantleSway = FMath::Sin(VisualTime * 5.0f) * (2.0f + 5.0f * MoveAlpha);
    MantleMesh->SetRelativeRotation(FRotator(-2.0f * MoveAlpha, MantleSway, -DodgeVisualSide * 8.0f * DodgeAlpha));

    LeftArmMesh->SetRelativeLocation(FMath::Lerp(
        FVector(15.0f, -19.0f, 48.0f),
        FVector(24.0f, -34.0f, 50.0f),
        ChargeAlpha));
    LeftArmMesh->SetRelativeRotation(FRotator(0.0f, FMath::Lerp(-48.0f, -68.0f, ChargeAlpha), 0.0f));

    RightArmMesh->SetRelativeLocation(FMath::Lerp(
        FVector(9.0f, 18.0f, 47.0f),
        FVector(4.0f, -2.0f, 52.0f),
        ChargeAlpha));
    RightArmMesh->SetRelativeRotation(FRotator(0.0f, FMath::Lerp(32.0f, -8.0f, ChargeAlpha), 0.0f));

    const float BowPull = ChargeAlpha * 15.0f;
    BowUpperMesh->SetRelativeLocation(FVector(22.0f, -48.0f, 43.0f));
    BowLowerMesh->SetRelativeLocation(FVector(22.0f, -48.0f, -5.0f));
    BowUpperMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, -11.0f - 12.0f * ChargeAlpha));
    BowLowerMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 11.0f + 12.0f * ChargeAlpha));

    BowStringUpperMesh->SetRelativeLocation(FVector(22.0f, -48.0f + BowPull * 0.5f, 31.0f));
    BowStringLowerMesh->SetRelativeLocation(FVector(22.0f, -48.0f + BowPull * 0.5f, 7.0f));
    BowStringUpperMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, -22.0f * ChargeAlpha));
    BowStringLowerMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 22.0f * ChargeAlpha));

    NockedArrowMesh->SetVisibility(bCanShowNockedArrow);
    if (bCanShowNockedArrow)
    {
        NockedArrowMesh->SetRelativeLocation(FVector(47.0f, -47.0f + BowPull, 19.0f));
        const float Pulse = 1.0f + 0.05f * FMath::Sin(VisualTime * 18.0f) * ChargeAlpha;
        NockedArrowMesh->SetRelativeScale3D(FVector(0.70f * Pulse, 0.025f, 0.025f));
    }

    HeadMesh->SetRelativeRotation(FRotator(0.0f, -5.0f * ChargeAlpha, 0.0f));
}

void AArashCharacter::PlayCombatFeedback(float Strength, bool bUseHitStop)
{
    CameraFeedbackStrength = FMath::Max(CameraFeedbackStrength, FMath::Clamp(Strength, 0.1f, 2.0f));
    CameraFeedbackRemaining = 0.11f;
    CameraFeedbackElapsed = 0.0f;

    if (bUseHitStop && GetWorld())
    {
        UGameplayStatics::SetGlobalTimeDilation(this, 0.48f);
        GetWorldTimerManager().ClearTimer(HitStopTimerHandle);
        GetWorldTimerManager().SetTimer(HitStopTimerHandle, this, &AArashCharacter::EndHitStop, 0.028f, false);
    }
}

void AArashCharacter::UpdateCombatFeedback(float DeltaSeconds)
{
    if (!CameraBoom)
    {
        return;
    }

    if (CameraFeedbackRemaining <= 0.0f)
    {
        CameraBoom->SocketOffset = FVector::ZeroVector;
        CameraFeedbackStrength = 0.0f;
        return;
    }

    CameraFeedbackElapsed += DeltaSeconds;
    CameraFeedbackRemaining -= DeltaSeconds;

    const float Alpha = FMath::Clamp(CameraFeedbackRemaining / 0.11f, 0.0f, 1.0f);
    const float Wave = FMath::Sin(CameraFeedbackElapsed * 92.0f);
    const float SideKick = Wave * 18.0f * CameraFeedbackStrength * Alpha;
    const float VerticalKick = FMath::Abs(Wave) * 7.0f * CameraFeedbackStrength * Alpha;
    CameraBoom->SocketOffset = FVector(0.0f, SideKick, VerticalKick);
}

void AArashCharacter::EndHitStop()
{
    UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
}

float AArashCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead || DamageAmount <= 0.0f || !GetWorld())
    {
        return 0.0f;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if ((Now - LastDamageTime) < DamageInvulnerabilityTime)
    {
        return 0.0f;
    }

    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    const float FinalDamage = Applied > 0.0f ? Applied : DamageAmount;
    LastDamageTime = Now;
    CurrentHealth = FMath::Max(0.0f, CurrentHealth - FinalDamage);
    PlayCombatFeedback(0.85f, false);

    if (CurrentHealth <= 0.0f)
    {
        bIsDead = true;
        EndHitStop();
        GetCharacterMovement()->DisableMovement();

        if (AArashGameModeBase* GameMode = Cast<AArashGameModeBase>(UGameplayStatics::GetGameMode(this)))
        {
            GameMode->NotifyPlayerDied();
        }
    }

    return FinalDamage;
}

float AArashCharacter::GetHealthAlpha() const
{
    return MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f;
}

void AArashCharacter::ApplyPierceUpgrade()
{
    ++ArrowMaxPierces;
}

void AArashCharacter::ApplyRicochetUpgrade()
{
    ++ArrowMaxBounces;
}

void AArashCharacter::ApplyReturnUpgrade()
{
    ArrowReturnSpeedMultiplier += 0.25f;
}

void AArashCharacter::SelectUpgrade1()
{
    if (AArashGameModeBase* GameMode = Cast<AArashGameModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        GameMode->SelectUpgrade(0);
    }
}

void AArashCharacter::SelectUpgrade2()
{
    if (AArashGameModeBase* GameMode = Cast<AArashGameModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        GameMode->SelectUpgrade(1);
    }
}

void AArashCharacter::SelectUpgrade3()
{
    if (AArashGameModeBase* GameMode = Cast<AArashGameModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        GameMode->SelectUpgrade(2);
    }
}

void AArashCharacter::RestartRun()
{
    const AArashGameModeBase* GameMode = Cast<AArashGameModeBase>(UGameplayStatics::GetGameMode(this));
    if (!GameMode || !GameMode->IsGameOver())
    {
        return;
    }

    EndHitStop();
    const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
    UGameplayStatics::OpenLevel(this, FName(*LevelName));
}

bool AArashCharacter::CanUseCombat() const
{
    if (bIsDead)
    {
        return false;
    }

    const AArashGameModeBase* GameMode = Cast<AArashGameModeBase>(UGameplayStatics::GetGameMode(this));
    return !GameMode || (!GameMode->IsWaitingForUpgrade() && !GameMode->IsGameOver());
}
