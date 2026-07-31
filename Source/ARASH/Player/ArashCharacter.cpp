#include "Player/ArashCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/ArashBowComponent.h"
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

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeVisual"));
    VisualMesh->SetupAttachment(RootComponent);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.6f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlayerVisualAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (PlayerVisualAsset.Succeeded())
    {
        VisualMesh->SetStaticMesh(PlayerVisualAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterial.Succeeded())
    {
        VisualMesh->SetMaterial(0, BasicMaterial.Object);
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
    VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.02f, 0.28f, 0.34f));

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
