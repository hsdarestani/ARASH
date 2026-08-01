#include "Environment/ArashEnvironmentManager.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
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

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> FloorMaterial(
        TEXT("/Game/Art/CC0/PolyHaven/marble_01/M_marble_01.M_marble_01"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallMaterial(
        TEXT("/Game/Art/CC0/PolyHaven/red_sandstone_wall/M_red_sandstone_wall.M_red_sandstone_wall"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> StoneMaterial(
        TEXT("/Game/Art/CC0/PolyHaven/sandstone_blocks_05/M_sandstone_blocks_05.M_sandstone_blocks_05"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MetalMaterial(
        TEXT("/Game/Art/CC0/PolyHaven/metal_plate_02/M_metal_plate_02.M_metal_plate_02"));

    BasicShapeMaterial = BasicMaterial.Object;
    CourtFloorMaterial = FloorMaterial.Object;
    CourtWallMaterial = WallMaterial.Object;
    CourtStoneMaterial = StoneMaterial.Object;
    CourtMetalMaterial = MetalMaterial.Object;
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

    CreateAccentMaterials();
    TuneLighting();
    GetWorldTimerManager().SetTimerForNextTick(this, &AArashEnvironmentManager::BuildGeneratedArena);
}

bool AArashEnvironmentManager::HasRequiredAssets() const
{
    return FloorTileMesh && FloorMedallionMesh && WallMesh && PillarMesh && BrazierMesh;
}

void AArashEnvironmentManager::CreateAccentMaterials()
{
    if (!BasicShapeMaterial)
    {
        return;
    }

    GoldMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
    TurquoiseMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
    CrimsonMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
    EmberMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);

    if (GoldMaterial)
    {
        GoldMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.62f, 0.24f, 0.025f));
    }
    if (TurquoiseMaterial)
    {
        TurquoiseMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.015f, 0.18f, 0.21f));
    }
    if (CrimsonMaterial)
    {
        CrimsonMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.16f, 0.006f, 0.012f));
    }
    if (EmberMaterial)
    {
        EmberMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.90f, 0.055f, 0.005f));
    }
}

void AArashEnvironmentManager::ApplyMaterials(UStaticMeshComponent* MeshComponent, UStaticMesh* MeshAsset)
{
    if (!MeshComponent || !MeshAsset)
    {
        return;
    }

    const TArray<FName> SlotNames = MeshComponent->GetMaterialSlotNames();
    for (int32 Index = 0; Index < SlotNames.Num(); ++Index)
    {
        const FString Slot = SlotNames[Index].ToString();
        UMaterialInterface* Selected = BasicShapeMaterial.Get();

        // Repeated floor tiles stay restrained. Their decorative geometry is deliberately
        // rendered as stone so the central medallion and combat silhouettes remain readable.
        if (MeshAsset == FloorTileMesh.Get())
        {
            Selected = Slot.Contains(TEXT("Floor"), ESearchCase::IgnoreCase)
                ? CourtFloorMaterial.Get()
                : CourtStoneMaterial.Get();
        }
        else if (Slot.Contains(TEXT("Floor"), ESearchCase::IgnoreCase))
        {
            Selected = CourtFloorMaterial.Get();
        }
        else if (Slot.Contains(TEXT("Wall"), ESearchCase::IgnoreCase))
        {
            Selected = CourtWallMaterial.Get();
        }
        else if (Slot.Contains(TEXT("Stone"), ESearchCase::IgnoreCase))
        {
            Selected = CourtStoneMaterial.Get();
        }
        else if (Slot.Contains(TEXT("Metal"), ESearchCase::IgnoreCase))
        {
            Selected = CourtMetalMaterial.Get();
        }
        else if (Slot.Contains(TEXT("Gold"), ESearchCase::IgnoreCase))
        {
            Selected = GoldMaterial.Get();
        }
        else if (Slot.Contains(TEXT("Turquoise"), ESearchCase::IgnoreCase))
        {
            Selected = TurquoiseMaterial.Get();
        }
        else if (Slot.Contains(TEXT("Crimson"), ESearchCase::IgnoreCase))
        {
            Selected = CrimsonMaterial.Get();
        }
        else if (Slot.Contains(TEXT("Emissive"), ESearchCase::IgnoreCase))
        {
            Selected = EmberMaterial.Get();
        }

        if (Selected)
        {
            MeshComponent->SetMaterial(Index, Selected);
        }
    }
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
    ApplyMaterials(Mesh, MeshAsset);

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

void AArashEnvironmentManager::TuneLighting()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Lock the prototype out of the aggressively bright editor auto-exposure response.
    UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("r.EyeAdaptationQuality 0"));
    UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("r.BloomQuality 4"));

    for (TActorIterator<ADirectionalLight> It(World); It; ++It)
    {
        if (UDirectionalLightComponent* Light = It->GetDirectionalLightComponent())
        {
            Light->SetIntensity(3.2f);
            Light->SetLightColor(FLinearColor(1.0f, 0.58f, 0.36f));
            Light->SetVolumetricScatteringIntensity(0.55f);
        }
    }

    for (TActorIterator<ASkyLight> It(World); It; ++It)
    {
        if (USkyLightComponent* Light = It->GetLightComponent())
        {
            Light->SetIntensity(0.42f);
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
    Light->SetIntensity(720.0f);
    Light->SetAttenuationRadius(360.0f);
    Light->SetLightColor(FLinearColor(1.0f, 0.16f, 0.025f));
    Light->SetCastShadows(false);
    Light->SetVolumetricScatteringIntensity(0.55f);
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
        false,
        FVector(1.18f));

    for (int32 Segment = -GridRadius; Segment <= GridRadius; ++Segment)
    {
        const float Along = Segment * TileStep;

        SpawnGeneratedMesh(WallMesh, ArenaCenter + FVector(Along, WallOffset, 0.0f),
            FRotator::ZeroRotator, true);

        if (Segment != 0 || !GateMesh)
        {
            SpawnGeneratedMesh(WallMesh, ArenaCenter + FVector(Along, -WallOffset, 0.0f),
                FRotator(0.0f, 180.0f, 0.0f), true);
        }

        SpawnGeneratedMesh(WallMesh, ArenaCenter + FVector(WallOffset, Along, 0.0f),
            FRotator(0.0f, -90.0f, 0.0f), true);
        SpawnGeneratedMesh(WallMesh, ArenaCenter + FVector(-WallOffset, Along, 0.0f),
            FRotator(0.0f, 90.0f, 0.0f), true);
    }

    if (GateMesh)
    {
        SpawnGeneratedMesh(GateMesh, ArenaCenter + FVector(0.0f, -WallOffset, 0.0f),
            FRotator(0.0f, 180.0f, 0.0f), true);
    }

    constexpr float PillarOffset = 1200.0f;
    for (int32 XSign : {-1, 1})
    {
        for (int32 YSign : {-1, 1})
        {
            SpawnGeneratedMesh(PillarMesh,
                ArenaCenter + FVector(PillarOffset * XSign, PillarOffset * YSign, 0.0f),
                FRotator::ZeroRotator, true, FVector(0.92f));
        }
    }

    constexpr float BrazierOffset = 1050.0f;
    for (int32 XSign : {-1, 1})
    {
        for (int32 YSign : {-1, 1})
        {
            const FVector BrazierLocation = ArenaCenter + FVector(
                BrazierOffset * XSign,
                BrazierOffset * YSign,
                0.0f);

            SpawnGeneratedMesh(BrazierMesh, BrazierLocation, FRotator::ZeroRotator,
                false, FVector(0.68f));
            SpawnFireLight(BrazierLocation + FVector(0.0f, 0.0f, 115.0f));
        }
    }

    if (BannerMesh)
    {
        SpawnGeneratedMesh(BannerMesh,
            ArenaCenter + FVector(-760.0f, WallOffset - 44.0f, 36.0f),
            FRotator(0.0f, 180.0f, 0.0f), false, FVector(0.82f));
        SpawnGeneratedMesh(BannerMesh,
            ArenaCenter + FVector(760.0f, WallOffset - 44.0f, 36.0f),
            FRotator(0.0f, 180.0f, 0.0f), false, FVector(0.82f));
    }

    if (BrokenColumnMesh)
    {
        SpawnGeneratedMesh(BrokenColumnMesh,
            ArenaCenter + FVector(1680.0f, -920.0f, 0.0f),
            FRotator(0.0f, 24.0f, 0.0f), false, FVector(0.86f));
        SpawnGeneratedMesh(BrokenColumnMesh,
            ArenaCenter + FVector(-1700.0f, 950.0f, 0.0f),
            FRotator(0.0f, -32.0f, 0.0f), false, FVector(0.72f));
    }

    if (RubbleMesh)
    {
        const FVector RubbleOffsets[] =
        {
            FVector(1620.0f, 1040.0f, 0.0f),
            FVector(-1620.0f, -1010.0f, 0.0f),
            FVector(920.0f, -1640.0f, 0.0f),
            FVector(-970.0f, 1640.0f, 0.0f)
        };
        const float RubbleRotations[] = { 12.0f, -27.0f, 48.0f, -18.0f };

        for (int32 Index = 0; Index < UE_ARRAY_COUNT(RubbleOffsets); ++Index)
        {
            SpawnGeneratedMesh(RubbleMesh,
                ArenaCenter + RubbleOffsets[Index],
                FRotator(0.0f, RubbleRotations[Index], 0.0f),
                false,
                FVector(0.68f + Index * 0.05f));
        }
    }

    UE_LOG(LogTemp, Display,
        TEXT("[ARASH Environment] Blender kit assembled with optimized materials and lighting."));
}
