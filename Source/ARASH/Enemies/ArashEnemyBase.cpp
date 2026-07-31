#include "Enemies/ArashEnemyBase.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/ArashGameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AArashEnemyBase::AArashEnemyBase()
{
    PrimaryActorTick.bCanEverTick = true;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeVisual"));
    VisualMesh->SetupAttachment(RootComponent);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeScale3D(DefaultVisualScale);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> EnemyVisualAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (EnemyVisualAsset.Succeeded())
    {
        VisualMesh->SetStaticMesh(EnemyVisualAsset.Object);
    }
}

void AArashEnemyBase::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
}

void AArashEnemyBase::ConfigureForWave(int32 WaveNumber)
{
    const int32 SafeWave = FMath::Max(1, WaveNumber);
    MaxHealth = 60.0f + static_cast<float>(SafeWave - 1) * 10.0f;
    CurrentHealth = MaxHealth;
    ChaseSpeed = FMath::Min(500.0f, 360.0f + static_cast<float>(SafeWave - 1) * 14.0f);
    ContactDamage = 18.0f + static_cast<float>(SafeWave - 1) * 2.0f;
    GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
}

void AArashEnemyBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (HitFeedbackRemaining > 0.0f)
    {
        HitFeedbackRemaining -= DeltaSeconds;
        if (HitFeedbackRemaining <= 0.0f && VisualMesh)
        {
            VisualMesh->SetRelativeScale3D(DefaultVisualScale);
        }
    }

    if (bDead)
    {
        return;
    }

    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (!Player)
    {
        return;
    }

    FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
    ToPlayer.Z = 0.0f;

    const float DistanceToPlayer = ToPlayer.Size();
    if (DistanceToPlayer <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector Direction = ToPlayer / DistanceToPlayer;
    SetActorRotation(Direction.Rotation());

    if (DistanceToPlayer > AttackStopDistance)
    {
        AddActorWorldOffset(Direction * ChaseSpeed * DeltaSeconds, true);
        return;
    }

    if (GetWorld())
    {
        const float Now = GetWorld()->GetTimeSeconds();
        if ((Now - LastAttackTime) >= AttackCooldown)
        {
            LastAttackTime = Now;
            UGameplayStatics::ApplyDamage(Player, ContactDamage, GetController(), this, UDamageType::StaticClass());
        }
    }
}

float AArashEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead || DamageAmount <= 0.0f)
    {
        return 0.0f;
    }

    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    const float FinalDamage = Applied > 0.0f ? Applied : DamageAmount;

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - FinalDamage);
    HitFeedbackRemaining = 0.12f;

    if (VisualMesh)
    {
        VisualMesh->SetRelativeScale3D(DefaultVisualScale * 1.18f);
    }

    if (DamageCauser)
    {
        FVector KnockbackDirection = GetActorLocation() - DamageCauser->GetActorLocation();
        KnockbackDirection.Z = 0.0f;
        if (KnockbackDirection.Normalize())
        {
            AddActorWorldOffset(KnockbackDirection * 85.0f, true);
        }
    }

    OnDamaged(FinalDamage, CurrentHealth);

    if (CurrentHealth <= 0.0f)
    {
        bDead = true;
        SetActorEnableCollision(false);
        if (VisualMesh)
        {
            VisualMesh->SetRelativeScale3D(DefaultVisualScale * 0.45f);
        }

        OnDeath();

        if (AArashGameModeBase* GameMode = Cast<AArashGameModeBase>(UGameplayStatics::GetGameMode(this)))
        {
            GameMode->NotifyEnemyKilled(this);
        }

        SetLifeSpan(0.15f);
    }

    return FinalDamage;
}
