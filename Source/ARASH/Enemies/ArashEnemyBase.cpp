#include "Enemies/ArashEnemyBase.h"

#include "Components/StaticMeshComponent.h"
#include "Enemies/EnemyBoltProjectile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/ArashGameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AArashEnemyBase::AArashEnemyBase()
{
    PrimaryActorTick.bCanEverTick = true;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> RaiderAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> WardenAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeVisual"));
    VisualMesh->SetupAttachment(RootComponent);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeScale3D(DefaultVisualScale);

    AccentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AccentVisual"));
    AccentMesh->SetupAttachment(RootComponent);
    AccentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponVisual"));
    WeaponMesh->SetupAttachment(RootComponent);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (RaiderAsset.Succeeded())
    {
        RaiderMesh = RaiderAsset.Object;
        VisualMesh->SetStaticMesh(RaiderMesh);
    }

    if (WardenAsset.Succeeded())
    {
        WardenMesh = WardenAsset.Object;
    }

    if (CubeAsset.Succeeded())
    {
        AccentMesh->SetStaticMesh(CubeAsset.Object);
        WeaponMesh->SetStaticMesh(CubeAsset.Object);
    }

    if (BasicMaterial.Succeeded())
    {
        VisualMesh->SetMaterial(0, BasicMaterial.Object);
        AccentMesh->SetMaterial(0, BasicMaterial.Object);
        WeaponMesh->SetMaterial(0, BasicMaterial.Object);
    }

    ProjectileClass = AEnemyBoltProjectile::StaticClass();
}

void AArashEnemyBase::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
}

void AArashEnemyBase::ConfigureForWave(int32 WaveNumber, EArashEnemyArchetype NewArchetype)
{
    Archetype = NewArchetype;

    const int32 SafeWave = FMath::Max(1, WaveNumber);
    MaxHealth = 60.0f + static_cast<float>(SafeWave - 1) * 10.0f;
    ChaseSpeed = FMath::Min(500.0f, 360.0f + static_cast<float>(SafeWave - 1) * 14.0f);
    ContactDamage = 18.0f + static_cast<float>(SafeWave - 1) * 2.0f;

    if (Archetype == EArashEnemyArchetype::Warden)
    {
        MaxHealth *= 0.82f;
        ChaseSpeed *= 0.78f;
        ContactDamage *= 0.65f;
        DefaultVisualScale = FVector(0.58f, 0.58f, 0.92f);

        if (WardenMesh)
        {
            VisualMesh->SetStaticMesh(WardenMesh);
        }

        VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.84f, 0.53f, 0.12f));

        AccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 58.0f));
        AccentMesh->SetRelativeScale3D(FVector(0.50f, 0.08f, 0.12f));
        AccentMesh->SetRelativeRotation(FRotator(0.0f, 45.0f, 0.0f));
        AccentMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.02f, 0.30f, 0.36f));

        WeaponMesh->SetRelativeLocation(FVector(-22.0f, 0.0f, -28.0f));
        WeaponMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.70f));
        WeaponMesh->SetRelativeRotation(FRotator::ZeroRotator);
        WeaponMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.035f, 0.09f, 0.16f));
    }
    else
    {
        DefaultVisualScale = FVector(0.7f, 0.7f, 1.4f);

        if (RaiderMesh)
        {
            VisualMesh->SetStaticMesh(RaiderMesh);
        }

        VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.48f, 0.055f, 0.045f));

        AccentMesh->SetRelativeLocation(FVector(4.0f, 44.0f, 18.0f));
        AccentMesh->SetRelativeScale3D(FVector(0.16f, 0.42f, 0.50f));
        AccentMesh->SetRelativeRotation(FRotator(0.0f, 8.0f, 0.0f));
        AccentMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.12f, 0.075f, 0.035f));

        WeaponMesh->SetRelativeLocation(FVector(2.0f, -45.0f, 35.0f));
        WeaponMesh->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.88f));
        WeaponMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 7.0f));
        WeaponMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.68f, 0.38f, 0.10f));
    }

    CurrentHealth = MaxHealth;
    VisualMesh->SetRelativeScale3D(DefaultVisualScale);
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

    if (Archetype == EArashEnemyArchetype::Warden)
    {
        TickWarden(Player, Direction, DistanceToPlayer, DeltaSeconds);
    }
    else
    {
        TickRaider(Player, Direction, DistanceToPlayer, DeltaSeconds);
    }
}

void AArashEnemyBase::TickRaider(ACharacter* Player, const FVector& Direction, float DistanceToPlayer, float DeltaSeconds)
{
    if (DistanceToPlayer > AttackStopDistance)
    {
        AddActorWorldOffset(Direction * ChaseSpeed * DeltaSeconds, true);
        return;
    }

    if (!GetWorld())
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if ((Now - LastAttackTime) >= AttackCooldown)
    {
        LastAttackTime = Now;
        UGameplayStatics::ApplyDamage(Player, ContactDamage, GetController(), this, UDamageType::StaticClass());
    }
}

void AArashEnemyBase::TickWarden(ACharacter* Player, const FVector& Direction, float DistanceToPlayer, float DeltaSeconds)
{
    const float TooCloseDistance = PreferredRange - 110.0f;
    const float TooFarDistance = PreferredRange + 130.0f;

    if (DistanceToPlayer > TooFarDistance)
    {
        AddActorWorldOffset(Direction * ChaseSpeed * DeltaSeconds, true);
    }
    else if (DistanceToPlayer < TooCloseDistance)
    {
        AddActorWorldOffset(-Direction * ChaseSpeed * 0.75f * DeltaSeconds, true);
    }

    if (!GetWorld())
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if ((Now - LastAttackTime) >= RangedAttackCooldown && DistanceToPlayer < 1050.0f)
    {
        LastAttackTime = Now;
        FireBoltAt(Player);
    }
}

void AArashEnemyBase::FireBoltAt(ACharacter* Player)
{
    UWorld* World = GetWorld();
    if (!World || !ProjectileClass || !Player)
    {
        return;
    }

    FVector Direction = Player->GetActorLocation() - GetActorLocation();
    Direction.Z = 0.0f;
    Direction.Normalize();

    const FVector SpawnLocation = GetActorLocation() + Direction * 85.0f + FVector(0.0f, 0.0f, 55.0f);

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.Instigator = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AEnemyBoltProjectile* Bolt = World->SpawnActor<AEnemyBoltProjectile>(ProjectileClass, SpawnLocation, Direction.Rotation(), Params);
    if (Bolt && Bolt->Movement)
    {
        Bolt->Damage = ContactDamage;
        Bolt->Movement->Velocity = Direction * Bolt->Movement->InitialSpeed;
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
        if (AccentMesh)
        {
            AccentMesh->SetVisibility(false, true);
        }
        if (WeaponMesh)
        {
            WeaponMesh->SetVisibility(false, true);
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
