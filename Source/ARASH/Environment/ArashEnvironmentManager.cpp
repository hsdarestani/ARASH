#include "Environment/ArashEnvironmentManager.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace ArashEnvironment
{
    const FName GeneratedTag(TEXT("ARASH_GeneratedEnvironment"));
}

AArashEnvironmentManager::AArashEnvironmentManager()
{
    PrimaryActorTick.bCanEverTick = false;
    SetActorHiddenInGame(true);
    SetCanBeDamaged(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> FloorTileAsset(
        TEXT("/Game/Art/Generated/Environment/SM_ARASH_FloorTile_A.SM_ARASH_FloorTile_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> FloorMedallionAsset(
        TEXT("/Game/Art/Generated/Environment/SM_ARASH_FloorMedallion_A.SM_ARASH_FloorMedallion_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> WallAsset(
        TEXT("/Game/Art/Generated/Environment/SM_ARASH_Wall_A.SM_ARASH_Wall_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PillarAsset(
        TEXT("/Game/Art/Generated/Environment/SM_ARASH_Pillar_A.SM_ARASH_Pillar_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BrazierAsset(
        TEXT("/Game/Art/Generated/Environment/SM_ARASH_Brazier_A.SM_ARASH_Brazier_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BannerAsset(
        TEXT("/Game/Art/Generated/Environment/SM_ARASH_Banner_A.SM_ARASH_Banner_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BrokenColumnAsset(
        TEXT("/Game/Art/Generated/Environment/SM_ARASH_BrokenColumn_A.SM_ARASH_BrokenColumn_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RubbleAsset(
        TEXT("/Game/Art/Generated/Environment/SM_ARASH_Rubble_A.SM_ARASH_Rubble_A"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> GateAsset(
        TEXT("/Game/Art/Generated/Environment/SM_ARASH_Gate_A.SM_ARASH_Gate_A"));

    FloorTileMesh = FloorTileAsset.Object;
    FloorMedallionMesh = FloorMedallionAsset.Object;
    WallMesh = WallAsset.Object;
    PillarMesh = PillarAsset.Object;
    BrazierMesh = BrazierAsset.Object;
    BannerMesh = BannerAsset.Object;
    BrokenColumnMesh = BrokenColumnAsset.Object;
    RubbleMesh = RubbleAsset.Object;
    GateMesh = GateAsset.Object;
}

void AArashEnvironmentManager::BeginPlay()
{
    Super::BeginPlay();

    if (!HasRequiredAssets())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ARASH Environment] Generated Blender kit is incomplete; keeping procedural fallback arena."));
        return;
    }

    GetWorldTimerManager().SetTimerForNextTick(this, &AArashEnvironmentManager::BuildGeneratedArena);
}

bool AArashEnvironmentManager::HasRequiredAssets() const
{
    return FloorTileMesh && FloorMedallionMesh && WallMesh && PillarMesh && BrazierMesh;
}

AStaticMeshActor* AArashEnvironmentManager::SpawnGeneratedMesh(
    UStaticMesh* MeshAsset,
    const FVector& Location,
    const FRotator& Rotation,
    bool bCollision,
    const FVector& Scale)
{
    UWorld* World = GetWorld();
    if (!World || !MeshAsset)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transient;

    AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, Rotation, Params);
    if (!Actor)
    {
        return nullptr;
    }

    Actor->Tags.Add(ArashEnvironment::GeneratedTag);
    Actor->SetActorScale3D(Scale);

    UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(MeshAsset);
    Mesh->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
    Mesh->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->SetCastShadow(true);

    return Actor;
}

void AArashEnvironmentManager::HidePrototypeGeometry()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
    {
        AStaticMeshActor* Actor = *It;
        if (!Actor || Actor->ActorHasTag(ArashEnvironment::GeneratedTag))
        {
            continue;
        }

        if (FVector::DistSquared2D(Actor->GetActorLocation(), ArenaCenter) > FMath::Square(2600.0f))
        {
            continue;
        }

        UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
        if (!Mesh)
        {
            continue;
        }

        UStaticMesh* StaticMesh = Mesh->GetStaticMesh();
        if (StaticMesh != Cube && StaticMesh != Cylinder && StaticMesh != Sphere)
        {
            continue;
        }

        const FVector Extent = Mesh->Bounds.BoxExtent;
        const bool bTraceFloor = Extent.X > 1000.0f && Extent.Y > 1000.0f && Extent.Z < 40.0f;

        Mesh->SetVisibility(false, true);
        Mesh->SetHiddenInGame(true, true);
        Mesh->SetCastShadow(false);

        if (!bTraceFloor)
        {
            Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
}

void AArashEnvironmentManager::SpawnFireLight(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transient;

    APointLight* FireLight = World->SpawnActor<APointLight>(Location, FRotator::ZeroRotator, Params);
    if (!FireLight || !FireLight->PointLightComponent)
    {
        return;
    }

    FireLight->Tags.Add(ArashEnvironment::GeneratedTag);

    UPointLightComponent* Light = FireLight->PointLightComponent;
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetIntensity(2600.0f);
    Light->SetAttenuationRadius(520.0f);
    Light->SetLightColor(FLinearColor(1.0f, 0.18f, 0.025f));
    Light->SetCastShadows(false);
    Light->SetVolumetricScatteringIntensity(1.5f);
}

void AArashEnvironmentManager::BuildGeneratedArena()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        ArenaCenter = PlayerPawn->GetActorLocation();
    }

    HidePrototypeGeometry();

    constexpr int32 GridRadius = 3;
    constexpr float TileStep = 400.0f;
    constexpr float WallOffset = 1450.0f;

    for (int32 X = -GridRadius; X <= GridRadius; ++X)
    {
        for (int32 Y = -GridRadius; Y <= GridRadius; ++Y)
        {
            SpawnGeneratedMesh(
                FloorTileMesh,
                ArenaCenter + FVector(X * TileStep, Y * TileStep, 1.0f),
                FRotator::ZeroRotator,
                false);
        }
    }

    SpawnGeneratedMesh(
        FloorMedallionMesh,
        ArenaCenter + FVector(0.0f, 0.0f, 4.0f),
        FRotator::ZeroRotator,
        false);

    for (int32 Segment = -GridRadius; Segment <= GridRadius; ++Segment)
    {
        const float Along = Segment * TileStep;

        SpawnGeneratedMesh(
            WallMesh,
            ArenaCenter + FVector(Along, WallOffset, 0.0f),
            FRotator::ZeroRotator,
            true);

        if (Segment != 0 || !GateMesh)
        {
            SpawnGeneratedMesh(
                WallMesh,
                ArenaCenter + FVector(Along, -WallOffset, 0.0f),
                FRotator(0.0f, 180.0f, 0.0f),
                true);
        }

        SpawnGeneratedMesh(
            WallMesh,
            ArenaCenter + FVector(WallOffset, Along, 0.0f),
            FRotator(0.0f, -90.0f, 0.0f),
            true);

        SpawnGeneratedMesh(
            WallMesh,
            ArenaCenter + FVector(-WallOffset, Along, 0.0f),
            FRotator(0.0f, 90.0f, 0.0f),
            true);
    }

    if (GateMesh)
    {
        SpawnGeneratedMesh(
            GateMesh,
            ArenaCenter + FVector(0.0f, -WallOffset, 0.0f),
            FRotator(0.0f, 180.0f, 0.0f),
            true);
    }

    constexpr float PillarOffset = 1180.0f;
    for (int32 XSign : {-1, 1})
    {
        for (int32 YSign : {-1, 1})
        {
            SpawnGeneratedMesh(
                PillarMesh,
                ArenaCenter + FVector(PillarOffset * XSign, PillarOffset * YSign, 0.0f),
                FRotator::ZeroRotator,
                true);
        }
    }

    constexpr float BrazierOffset = 980.0f;
    for (int32 XSign : {-1, 1})
    {
        for (int32 YSign : {-1, 1})
        {
            const FVector BrazierLocation = ArenaCenter + FVector(
                BrazierOffset * XSign,
                BrazierOffset * YSign,
                0.0f);

            SpawnGeneratedMesh(BrazierMesh, BrazierLocation, FRotator::ZeroRotator, false);
            SpawnFireLight(BrazierLocation + FVector(0.0f, 0.0f, 165.0f));
        }
    }

    if (BannerMesh)
    {
        SpawnGeneratedMesh(
            BannerMesh,
            ArenaCenter + FVector(-820.0f, WallOffset - 42.0f, 22.0f),
            FRotator(0.0f, 180.0f, 0.0f),
            false,
            FVector(0.92f));

        SpawnGeneratedMesh(
            BannerMesh,
            ArenaCenter + FVector(820.0f, WallOffset - 42.0f, 22.0f),
            FRotator(0.0f, 180.0f, 0.0f),
            false,
            FVector(0.92f));
    }

    if (BrokenColumnMesh)
    {
        SpawnGeneratedMesh(
            BrokenColumnMesh,
            ArenaCenter + FVector(1700.0f, -930.0f, 0.0f),
            FRotator(0.0f, 24.0f, 0.0f),
            false);

        SpawnGeneratedMesh(
            BrokenColumnMesh,
            ArenaCenter + FVector(-1720.0f, 960.0f, 0.0f),
            FRotator(0.0f, -32.0f, 0.0f),
            false,
            FVector(0.82f));
    }

    if (RubbleMesh)
    {
        const FVector RubbleOffsets[] =
        {
            FVector(1640.0f, 1050.0f, 0.0f),
            FVector(-1640.0f, -1020.0f, 0.0f),
            FVector(930.0f, -1660.0f, 0.0f),
            FVector(-980.0f, 1660.0f, 0.0f)
        };

        const float RubbleRotations[] = { 12.0f, -27.0f, 48.0f, -18.0f };

        for (int32 Index = 0; Index < UE_ARRAY_COUNT(RubbleOffsets); ++Index)
        {
            SpawnGeneratedMesh(
                RubbleMesh,
                ArenaCenter + RubbleOffsets[Index],
                FRotator(0.0f, RubbleRotations[Index], 0.0f),
                false,
                FVector(0.78f + Index * 0.06f));
        }
    }

    UE_LOG(LogTemp, Display,
        TEXT("[ARASH Environment] Blender environment kit assembled successfully."));
}
