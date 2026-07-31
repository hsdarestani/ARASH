#include "Game/ArashGameModeBase.h"

#include "Enemies/ArashEnemyBase.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Game/ArashHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Player/ArashCharacter.h"
#include "UObject/ConstructorHelpers.h"

AArashGameModeBase::AArashGameModeBase()
{
    DefaultPawnClass = AArashCharacter::StaticClass();
    HUDClass = AArashHUD::StaticClass();
    EnemyClass = AArashEnemyBase::StaticClass();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded())
    {
        PrototypeCubeMesh = CubeAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterial.Succeeded())
    {
        PrototypeMaterial = BasicMaterial.Object;
    }
}

void AArashGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        ArenaCenter = PlayerPawn->GetActorLocation();
    }

    SpawnPrototypeArena();
    StartWave();
}

AStaticMeshActor* AArashGameModeBase::SpawnArenaBlock(const FVector& Location, const FVector& Scale,
    const FVector& Color, bool bCollision)
{
    UWorld* World = GetWorld();
    if (!World || !PrototypeCubeMesh)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AStaticMeshActor* Block = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
    if (!Block)
    {
        return nullptr;
    }

    UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
    Mesh->SetStaticMesh(PrototypeCubeMesh);
    Mesh->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
    if (PrototypeMaterial)
    {
        Mesh->SetMaterial(0, PrototypeMaterial);
        Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), Color);
    }

    Block->SetActorScale3D(Scale);
    return Block;
}

void AArashGameModeBase::SpawnPrototypeArena()
{
    if (!GetWorld() || !PrototypeCubeMesh)
    {
        return;
    }

    constexpr float HalfExtent = 1450.0f;
    constexpr float WallHeight = 300.0f;
    const FVector LapisColor(0.025f, 0.16f, 0.25f);

    SpawnArenaBlock(ArenaCenter + FVector(HalfExtent, 0.0f, WallHeight * 0.5f), FVector(1.0f, 29.0f, 3.0f), LapisColor, true);
    SpawnArenaBlock(ArenaCenter + FVector(-HalfExtent, 0.0f, WallHeight * 0.5f), FVector(1.0f, 29.0f, 3.0f), LapisColor, true);
    SpawnArenaBlock(ArenaCenter + FVector(0.0f, HalfExtent, WallHeight * 0.5f), FVector(29.0f, 1.0f, 3.0f), LapisColor, true);
    SpawnArenaBlock(ArenaCenter + FVector(0.0f, -HalfExtent, WallHeight * 0.5f), FVector(29.0f, 1.0f, 3.0f), LapisColor, true);

    SpawnPersianCourtDetails();
}

void AArashGameModeBase::SpawnPersianCourtDetails()
{
    const FVector GoldColor(0.72f, 0.42f, 0.08f);
    const FVector TurquoiseColor(0.02f, 0.34f, 0.38f);
    constexpr float Corner = 1210.0f;

    for (int32 XSign : {-1, 1})
    {
        for (int32 YSign : {-1, 1})
        {
            const FVector CornerCenter = ArenaCenter + FVector(Corner * XSign, Corner * YSign, 0.0f);
            SpawnArenaBlock(CornerCenter + FVector(0.0f, 0.0f, 45.0f), FVector(1.6f, 1.6f, 0.9f), TurquoiseColor, false);
            SpawnArenaBlock(CornerCenter + FVector(0.0f, 0.0f, 270.0f), FVector(0.62f, 0.62f, 4.5f), GoldColor, false);
            SpawnArenaBlock(CornerCenter + FVector(0.0f, 0.0f, 505.0f), FVector(1.25f, 1.25f, 0.35f), GoldColor, false);
        }
    }

    const FVector MarkerOffsets[] =
    {
        FVector(0.0f, 980.0f, 30.0f),
        FVector(0.0f, -980.0f, 30.0f),
        FVector(980.0f, 0.0f, 30.0f),
        FVector(-980.0f, 0.0f, 30.0f)
    };

    for (const FVector& Offset : MarkerOffsets)
    {
        SpawnArenaBlock(ArenaCenter + Offset, FVector(1.7f, 1.7f, 0.28f), TurquoiseColor, false);
    }
}

void AArashGameModeBase::StartWave()
{
    if (bGameOver)
    {
        return;
    }

    bWaitingForUpgrade = false;
    SpawnWaveEnemies(CurrentWave);
}

void AArashGameModeBase::SpawnWaveEnemies(int32 WaveNumber)
{
    UWorld* World = GetWorld();
    if (!World || !EnemyClass)
    {
        return;
    }

    const int32 EnemyCount = FMath::Clamp(4 + WaveNumber * 2, 6, 18);
    EnemiesRemaining = 0;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (int32 Index = 0; Index < EnemyCount; ++Index)
    {
        const bool bSpawnWarden = (Index % 4) == 3;
        const EArashEnemyArchetype Archetype = bSpawnWarden
            ? EArashEnemyArchetype::Warden
            : EArashEnemyArchetype::Raider;

        const float Angle = (TWO_PI * static_cast<float>(Index)) / static_cast<float>(EnemyCount);
        const float Radius = bSpawnWarden ? 1080.0f : ((Index % 2 == 0) ? 820.0f : 960.0f);
        const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);

        AArashEnemyBase* Enemy = World->SpawnActor<AArashEnemyBase>(EnemyClass, ArenaCenter + Offset, FRotator::ZeroRotator, Params);
        if (Enemy)
        {
            Enemy->ConfigureForWave(WaveNumber, Archetype);
            ++EnemiesRemaining;
        }
    }
}

void AArashGameModeBase::NotifyEnemyKilled(AArashEnemyBase* Enemy)
{
    if (bGameOver || bWaitingForUpgrade)
    {
        return;
    }

    EnemiesRemaining = FMath::Max(0, EnemiesRemaining - 1);
    if (EnemiesRemaining == 0)
    {
        bWaitingForUpgrade = true;
    }
}

void AArashGameModeBase::NotifyPlayerDied()
{
    bGameOver = true;
    bWaitingForUpgrade = false;
}

void AArashGameModeBase::SelectUpgrade(int32 UpgradeIndex)
{
    if (!bWaitingForUpgrade || bGameOver)
    {
        return;
    }

    AArashCharacter* Player = Cast<AArashCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Player || Player->IsDead())
    {
        return;
    }

    switch (UpgradeIndex)
    {
        case 0:
            Player->ApplyPierceUpgrade();
            break;
        case 1:
            Player->ApplyRicochetUpgrade();
            break;
        case 2:
            Player->ApplyReturnUpgrade();
            break;
        default:
            return;
    }

    ++CurrentWave;
    StartWave();
}
