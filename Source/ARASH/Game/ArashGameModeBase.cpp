#include "Game/ArashGameModeBase.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Enemies/ArashEnemyBase.h"
#include "Engine/PointLight.h"
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
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    if (CubeAsset.Succeeded())
    {
        PrototypeCubeMesh = CubeAsset.Object;
    }
    if (CylinderAsset.Succeeded())
    {
        PrototypeCylinderMesh = CylinderAsset.Object;
    }
    if (SphereAsset.Succeeded())
    {
        PrototypeSphereMesh = SphereAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterial.Succeeded())
    {
        PrototypeMaterial = BasicMaterial.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> FloorMaterial(
        TEXT("/Game/Art/CC0/PolyHaven/marble_01/M_marble_01.M_marble_01"));
    if (FloorMaterial.Succeeded())
    {
        CourtFloorMaterial = FloorMaterial.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallMaterial(
        TEXT("/Game/Art/CC0/PolyHaven/red_sandstone_wall/M_red_sandstone_wall.M_red_sandstone_wall"));
    if (WallMaterial.Succeeded())
    {
        CourtWallMaterial = WallMaterial.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> StoneMaterial(
        TEXT("/Game/Art/CC0/PolyHaven/sandstone_blocks_05/M_sandstone_blocks_05.M_sandstone_blocks_05"));
    if (StoneMaterial.Succeeded())
    {
        CourtStoneMaterial = StoneMaterial.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MetalMaterial(
        TEXT("/Game/Art/CC0/PolyHaven/metal_plate_02/M_metal_plate_02.M_metal_plate_02"));
    if (MetalMaterial.Succeeded())
    {
        CourtMetalMaterial = MetalMaterial.Object;
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
    SpawnCinematicLights();
    SpawnAtmosphere();
    StartWave();
}

AStaticMeshActor* AArashGameModeBase::SpawnArenaMesh(UStaticMesh* MeshAsset, const FVector& Location,
    const FVector& Scale, const FRotator& Rotation, const FVector& Color,
    bool bCollision, UMaterialInterface* OverrideMaterial)
{
    UWorld* World = GetWorld();
    if (!World || !MeshAsset)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, Rotation, Params);
    if (!Actor)
    {
        return nullptr;
    }

    UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
    Mesh->SetStaticMesh(MeshAsset);
    Mesh->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));

    UMaterialInterface* MaterialToUse = OverrideMaterial ? OverrideMaterial : PrototypeMaterial.Get();
    if (MaterialToUse)
    {
        Mesh->SetMaterial(0, MaterialToUse);
        if (MaterialToUse == PrototypeMaterial.Get())
        {
            Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), Color);
        }
    }

    Actor->SetActorScale3D(Scale);
    return Actor;
}

AStaticMeshActor* AArashGameModeBase::SpawnArenaBlock(const FVector& Location, const FVector& Scale,
    const FVector& Color, bool bCollision, UMaterialInterface* OverrideMaterial)
{
    return SpawnArenaMesh(PrototypeCubeMesh, Location, Scale, FRotator::ZeroRotator,
        Color, bCollision, OverrideMaterial);
}

void AArashGameModeBase::SpawnPrototypeArena()
{
    if (!GetWorld() || !PrototypeCubeMesh)
    {
        return;
    }

    const FVector SandstoneFallback(0.28f, 0.13f, 0.07f);
    const FVector DeepStone(0.09f, 0.075f, 0.06f);
    const FVector Gold(0.68f, 0.36f, 0.055f);
    const FVector Turquoise(0.018f, 0.25f, 0.30f);

    // Visual floor: the template floor remains underneath as the reliable cursor trace surface.
    SpawnArenaBlock(
        ArenaCenter + FVector(0.0f, 0.0f, 2.0f),
        FVector(28.65f, 28.65f, 0.035f),
        FVector(0.62f, 0.57f, 0.46f),
        false,
        CourtFloorMaterial);

    // Build each perimeter wall from modules instead of one stretched cube. This keeps
    // the imported PBR texel density readable and gives the court an architectural rhythm.
    constexpr float HalfExtent = 1450.0f;
    constexpr float SegmentStep = 460.0f;
    constexpr int32 SegmentCount = 6;

    for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
    {
        const float Along = -1150.0f + static_cast<float>(Segment) * SegmentStep;

        for (int32 Sign : {-1, 1})
        {
            const float Side = static_cast<float>(Sign);

            // East / West walls.
            SpawnArenaBlock(ArenaCenter + FVector(Side * HalfExtent, Along, 115.0f),
                FVector(0.72f, 4.55f, 2.25f), SandstoneFallback, true, CourtWallMaterial);
            SpawnArenaBlock(ArenaCenter + FVector(Side * HalfExtent, Along, 18.0f),
                FVector(1.03f, 4.58f, 0.32f), DeepStone, false, CourtStoneMaterial);
            SpawnArenaBlock(ArenaCenter + FVector(Side * HalfExtent, Along, 237.0f),
                FVector(1.03f, 4.58f, 0.22f), DeepStone, false, CourtStoneMaterial);
            SpawnArenaBlock(ArenaCenter + FVector(Side * (HalfExtent - 75.0f), Along, 188.0f),
                FVector(0.08f, 3.80f, 0.07f), Gold, false);
            SpawnArenaBlock(ArenaCenter + FVector(Side * (HalfExtent - 78.0f), Along, 145.0f),
                FVector(0.06f, 3.55f, 0.045f), Turquoise, false);

            // North / South walls.
            SpawnArenaBlock(ArenaCenter + FVector(Along, Side * HalfExtent, 115.0f),
                FVector(4.55f, 0.72f, 2.25f), SandstoneFallback, true, CourtWallMaterial);
            SpawnArenaBlock(ArenaCenter + FVector(Along, Side * HalfExtent, 18.0f),
                FVector(4.58f, 1.03f, 0.32f), DeepStone, false, CourtStoneMaterial);
            SpawnArenaBlock(ArenaCenter + FVector(Along, Side * HalfExtent, 237.0f),
                FVector(4.58f, 1.03f, 0.22f), DeepStone, false, CourtStoneMaterial);
            SpawnArenaBlock(ArenaCenter + FVector(Along, Side * (HalfExtent - 75.0f), 188.0f),
                FVector(3.80f, 0.08f, 0.07f), Gold, false);
            SpawnArenaBlock(ArenaCenter + FVector(Along, Side * (HalfExtent - 78.0f), 145.0f),
                FVector(3.55f, 0.06f, 0.045f), Turquoise, false);
        }
    }

    // Four monumental corners establish the Persian court silhouette.
    constexpr float ColumnOffset = 1180.0f;
    for (int32 XSign : {-1, 1})
    {
        for (int32 YSign : {-1, 1})
        {
            const FVector Base = ArenaCenter + FVector(ColumnOffset * XSign, ColumnOffset * YSign, 0.0f);
            SpawnColumn(Base, 1.0f);
        }
    }

    SpawnFloorInlay();
    SpawnPersianCourtDetails();
    SpawnOuterRuins();
}

void AArashGameModeBase::SpawnFloorInlay()
{
    const FVector Gold(0.78f, 0.43f, 0.07f);
    const FVector Turquoise(0.015f, 0.30f, 0.34f);
    const FVector Ivory(0.55f, 0.50f, 0.40f);

    // Central medallion: nested thin cylinders create a readable metallic/stone ring
    // while remaining extremely cheap compared with a bespoke hero mesh.
    if (PrototypeCylinderMesh)
    {
        SpawnArenaMesh(PrototypeCylinderMesh, ArenaCenter + FVector(0.0f, 0.0f, 5.0f),
            FVector(5.8f, 5.8f, 0.025f), FRotator::ZeroRotator, Gold, false, CourtMetalMaterial);
        SpawnArenaMesh(PrototypeCylinderMesh, ArenaCenter + FVector(0.0f, 0.0f, 7.0f),
            FVector(5.15f, 5.15f, 0.026f), FRotator::ZeroRotator, Ivory, false, CourtFloorMaterial);
        SpawnArenaMesh(PrototypeCylinderMesh, ArenaCenter + FVector(0.0f, 0.0f, 9.0f),
            FVector(2.0f, 2.0f, 0.028f), FRotator::ZeroRotator, Turquoise, false);
        SpawnArenaMesh(PrototypeCylinderMesh, ArenaCenter + FVector(0.0f, 0.0f, 11.0f),
            FVector(1.55f, 1.55f, 0.029f), FRotator::ZeroRotator, Gold, false, CourtMetalMaterial);
    }

    // Cross-axis and diamond inlays organize combat space without reducing readability.
    SpawnArenaBlock(ArenaCenter + FVector(0.0f, 0.0f, 5.5f), FVector(11.2f, 0.055f, 0.018f), Gold, false, CourtMetalMaterial);
    SpawnArenaBlock(ArenaCenter + FVector(0.0f, 0.0f, 5.6f), FVector(0.055f, 11.2f, 0.018f), Gold, false, CourtMetalMaterial);

    SpawnArenaMesh(PrototypeCubeMesh, ArenaCenter + FVector(0.0f, 0.0f, 5.7f),
        FVector(8.2f, 0.045f, 0.016f), FRotator(0.0f, 45.0f, 0.0f), Turquoise, false);
    SpawnArenaMesh(PrototypeCubeMesh, ArenaCenter + FVector(0.0f, 0.0f, 5.8f),
        FVector(8.2f, 0.045f, 0.016f), FRotator(0.0f, -45.0f, 0.0f), Turquoise, false);

    // Four cardinal floor markers echo the central medallion.
    const FVector MarkerOffsets[] =
    {
        FVector(0.0f, 880.0f, 5.0f), FVector(0.0f, -880.0f, 5.0f),
        FVector(880.0f, 0.0f, 5.0f), FVector(-880.0f, 0.0f, 5.0f)
    };

    for (const FVector& Offset : MarkerOffsets)
    {
        SpawnArenaMesh(PrototypeCylinderMesh, ArenaCenter + Offset,
            FVector(1.25f, 1.25f, 0.022f), FRotator::ZeroRotator,
            Turquoise, false, CourtStoneMaterial);
    }
}

void AArashGameModeBase::SpawnColumn(const FVector& BaseLocation, float Scale)
{
    const FVector Gold(0.72f, 0.39f, 0.06f);
    const FVector Turquoise(0.025f, 0.25f, 0.29f);
    const FVector Stone(0.22f, 0.15f, 0.10f);

    // Stepped plinth.
    SpawnArenaBlock(BaseLocation + FVector(0.0f, 0.0f, 22.0f * Scale),
        FVector(1.60f, 1.60f, 0.42f) * Scale, Stone, true, CourtStoneMaterial);
    SpawnArenaBlock(BaseLocation + FVector(0.0f, 0.0f, 58.0f * Scale),
        FVector(1.28f, 1.28f, 0.25f) * Scale, Turquoise, false);

    // Shaft, neck, capital and gold crown.
    if (PrototypeCylinderMesh)
    {
        SpawnArenaMesh(PrototypeCylinderMesh, BaseLocation + FVector(0.0f, 0.0f, 245.0f * Scale),
            FVector(0.72f, 0.72f, 3.45f) * Scale, FRotator::ZeroRotator,
            Stone, true, CourtStoneMaterial);

        SpawnArenaMesh(PrototypeCylinderMesh, BaseLocation + FVector(0.0f, 0.0f, 420.0f * Scale),
            FVector(0.93f, 0.93f, 0.24f) * Scale, FRotator::ZeroRotator,
            Turquoise, false);
    }

    SpawnArenaBlock(BaseLocation + FVector(0.0f, 0.0f, 458.0f * Scale),
        FVector(1.22f, 1.22f, 0.32f) * Scale, Stone, false, CourtStoneMaterial);
    SpawnArenaBlock(BaseLocation + FVector(0.0f, 0.0f, 491.0f * Scale),
        FVector(1.45f, 0.52f, 0.18f) * Scale, Gold, false, CourtMetalMaterial);
    SpawnArenaBlock(BaseLocation + FVector(0.0f, 0.0f, 491.0f * Scale),
        FVector(0.52f, 1.45f, 0.18f) * Scale, Gold, false, CourtMetalMaterial);

    // Each monumental corner receives a fire bowl slightly toward the arena interior.
    const FVector TowardCenter = (ArenaCenter - BaseLocation).GetSafeNormal2D();
    SpawnBrazier(BaseLocation + TowardCenter * 205.0f * Scale);
}

void AArashGameModeBase::SpawnBrazier(const FVector& BaseLocation)
{
    const FVector DarkStone(0.10f, 0.07f, 0.045f);
    const FVector Bronze(0.48f, 0.20f, 0.035f);
    const FVector Fire(1.0f, 0.13f, 0.015f);

    SpawnArenaBlock(BaseLocation + FVector(0.0f, 0.0f, 24.0f),
        FVector(0.85f, 0.85f, 0.40f), DarkStone, false, CourtStoneMaterial);

    if (PrototypeCylinderMesh)
    {
        SpawnArenaMesh(PrototypeCylinderMesh, BaseLocation + FVector(0.0f, 0.0f, 72.0f),
            FVector(0.62f, 0.62f, 0.18f), FRotator::ZeroRotator,
            Bronze, false, CourtMetalMaterial);
    }

    if (PrototypeSphereMesh)
    {
        SpawnArenaMesh(PrototypeSphereMesh, BaseLocation + FVector(0.0f, 0.0f, 94.0f),
            FVector(0.28f, 0.28f, 0.18f), FRotator::ZeroRotator,
            Fire, false);
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APointLight* FireLight = World->SpawnActor<APointLight>(
        BaseLocation + FVector(0.0f, 0.0f, 125.0f), FRotator::ZeroRotator, Params);

    if (FireLight && FireLight->PointLightComponent)
    {
        UPointLightComponent* Light = FireLight->PointLightComponent;
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensity(2200.0f);
        Light->SetAttenuationRadius(430.0f);
        Light->SetLightColor(FLinearColor(1.0f, 0.20f, 0.035f));
        Light->SetCastShadows(false);
        Light->SetVolumetricScatteringIntensity(1.7f);
    }
}

void AArashGameModeBase::SpawnPersianCourtDetails()
{
    const FVector Gold(0.70f, 0.38f, 0.055f);
    const FVector Turquoise(0.018f, 0.26f, 0.31f);
    const FVector Stone(0.24f, 0.16f, 0.10f);

    // Four shallow ceremonial platforms at the cardinal axes. They frame the arena
    // without obstructing the central projectile lanes.
    const FVector Platforms[] =
    {
        FVector(0.0f, 1120.0f, 18.0f), FVector(0.0f, -1120.0f, 18.0f),
        FVector(1120.0f, 0.0f, 18.0f), FVector(-1120.0f, 0.0f, 18.0f)
    };

    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FVector Position = ArenaCenter + Platforms[Index];
        const bool bVertical = Index < 2;

        SpawnArenaBlock(Position,
            bVertical ? FVector(2.6f, 1.45f, 0.26f) : FVector(1.45f, 2.6f, 0.26f),
            Stone, false, CourtStoneMaterial);

        SpawnArenaBlock(Position + FVector(0.0f, 0.0f, 30.0f),
            bVertical ? FVector(2.15f, 1.10f, 0.10f) : FVector(1.10f, 2.15f, 0.10f),
            Turquoise, false);

        SpawnArenaBlock(Position + FVector(0.0f, 0.0f, 42.0f),
            bVertical ? FVector(1.82f, 0.07f, 0.055f) : FVector(0.07f, 1.82f, 0.055f),
            Gold, false, CourtMetalMaterial);
    }
}

void AArashGameModeBase::SpawnOuterRuins()
{
    const FVector RuinColor(0.18f, 0.11f, 0.07f);

    struct FRuinBlock
    {
        FVector Offset;
        FVector Scale;
        float Yaw;
    };

    const FRuinBlock Ruins[] =
    {
        { FVector(1680.0f, 930.0f, 70.0f), FVector(2.4f, 0.9f, 1.2f), 18.0f },
        { FVector(1770.0f, 720.0f, 35.0f), FVector(1.1f, 1.8f, 0.5f), -24.0f },
        { FVector(-1700.0f, 820.0f, 55.0f), FVector(2.0f, 0.75f, 0.9f), -12.0f },
        { FVector(-1760.0f, -760.0f, 45.0f), FVector(1.45f, 2.0f, 0.7f), 31.0f },
        { FVector(1640.0f, -900.0f, 38.0f), FVector(1.7f, 1.35f, 0.55f), 14.0f },
        { FVector(760.0f, 1710.0f, 50.0f), FVector(1.8f, 1.1f, 0.75f), 36.0f },
        { FVector(-820.0f, -1720.0f, 42.0f), FVector(1.9f, 0.85f, 0.62f), -28.0f }
    };

    for (const FRuinBlock& Ruin : Ruins)
    {
        SpawnArenaMesh(PrototypeCubeMesh, ArenaCenter + Ruin.Offset,
            Ruin.Scale, FRotator(0.0f, Ruin.Yaw, 0.0f), RuinColor,
            false, CourtStoneMaterial);
    }

    // Fallen column fragments make the exterior silhouette less box-like.
    if (PrototypeCylinderMesh)
    {
        SpawnArenaMesh(PrototypeCylinderMesh, ArenaCenter + FVector(1580.0f, -1230.0f, 70.0f),
            FVector(0.55f, 0.55f, 2.2f), FRotator(0.0f, 28.0f, 82.0f),
            RuinColor, false, CourtStoneMaterial);
        SpawnArenaMesh(PrototypeCylinderMesh, ArenaCenter + FVector(-1540.0f, 1240.0f, 60.0f),
            FVector(0.48f, 0.48f, 1.8f), FRotator(0.0f, -14.0f, 86.0f),
            RuinColor, false, CourtStoneMaterial);
    }
}

void AArashGameModeBase::SpawnCinematicLights()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

#if PLATFORM_ANDROID || PLATFORM_IOS
    constexpr float AccentIntensity = 900.0f;
#else
    constexpr float AccentIntensity = 1450.0f;
#endif

    // Cool fill lights counter the warm sun/fire and separate silhouettes from sandstone.
    const FVector FillPositions[] =
    {
        ArenaCenter + FVector(760.0f, 760.0f, 360.0f),
        ArenaCenter + FVector(-760.0f, -760.0f, 360.0f)
    };

    for (const FVector& Position : FillPositions)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        APointLight* AccentLight = World->SpawnActor<APointLight>(Position, FRotator::ZeroRotator, Params);
        if (!AccentLight || !AccentLight->PointLightComponent)
        {
            continue;
        }

        UPointLightComponent* Light = AccentLight->PointLightComponent;
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensity(AccentIntensity);
        Light->SetAttenuationRadius(850.0f);
        Light->SetLightColor(FLinearColor(0.05f, 0.24f, 0.34f));
        Light->SetCastShadows(false);
        Light->SetVolumetricScatteringIntensity(0.55f);
    }
}

void AArashGameModeBase::SpawnAtmosphere()
{
    // The template map already owns DirectionalLight / SkyAtmosphere / HeightFog / SkyLight.
    // We deliberately keep those as the authoritative atmosphere actors instead of
    // spawning duplicates. This hook remains the single place for the upcoming HDRI
    // and color-grading integration once the art assets are committed to the project.
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

        AArashEnemyBase* Enemy = World->SpawnActor<AArashEnemyBase>(EnemyClass,
            ArenaCenter + Offset, FRotator::ZeroRotator, Params);
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
