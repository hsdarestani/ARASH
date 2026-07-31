#include "Player/ArashCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/ArashBowComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

AArashCharacter::AArashCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    bUseControllerRotationYaw = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = 650.0f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 1050.0f;
    CameraBoom->SetRelativeRotation(FRotator(-58.0f, 0.0f, 0.0f));
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

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        PC->DefaultMouseCursor = EMouseCursor::Crosshairs;
    }
}

void AArashCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateAim();
}

void AArashCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AArashCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AArashCharacter::MoveRight);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AArashCharacter::StartFire);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &AArashCharacter::StopFire);
    PlayerInputComponent->BindAction(TEXT("Dodge"), IE_Pressed, this, &AArashCharacter::Dodge);
}

void AArashCharacter::MoveForward(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(FVector::ForwardVector, Value);
    }
}

void AArashCharacter::MoveRight(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(FVector::RightVector, Value);
    }
}

void AArashCharacter::StartFire()
{
    if (Bow)
    {
        Bow->StartCharge();
    }
}

void AArashCharacter::StopFire()
{
    if (Bow)
    {
        Bow->ReleaseArrow();
    }
}

void AArashCharacter::Dodge()
{
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
