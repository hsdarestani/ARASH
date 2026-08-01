$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$CharacterPath = Join-Path $RepoRoot "Source\ARASH\Player\ArashCharacter.cpp"
$EnvironmentPath = Join-Path $RepoRoot "Source\ARASH\Environment\ArashEnvironmentManager.cpp"
$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)

function Read-Source([string]$Path) {
    if (-not (Test-Path $Path)) {
        throw "Source file not found: $Path"
    }
    return Get-Content -Path $Path -Raw
}

function Write-Source([string]$Path, [string]$Content) {
    [System.IO.File]::WriteAllText($Path, $Content, $Utf8NoBom)
}

# -----------------------------------------------------------------------------
# Player camera and per-camera post process.
# -----------------------------------------------------------------------------
$Character = Read-Source $CharacterPath
$CharacterOriginal = $Character

if (-not $Character.Contains('#include "Engine/Scene.h"')) {
    $Character = $Character.Replace(
        '#include "Engine/World.h"',
        "#include `"Engine/World.h`"`r`n#include `"Engine/Scene.h`"")
}

$OldBoom = @'
    CameraBoom->TargetArmLength = 1050.0f;
    CameraBoom->SetUsingAbsoluteRotation(true);
    CameraBoom->SetRelativeRotation(FRotator(-58.0f, -45.0f, 0.0f));
    CameraBoom->bDoCollisionTest = false;
    CameraBoom->bUsePawnControlRotation = false;
'@

$NewBoom = @'
    CameraBoom->TargetArmLength = 1280.0f;
    CameraBoom->SetUsingAbsoluteRotation(true);
    CameraBoom->SetRelativeRotation(FRotator(-54.0f, -42.0f, 0.0f));
    CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 62.0f);
    CameraBoom->bDoCollisionTest = false;
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 13.0f;
    CameraBoom->CameraLagMaxDistance = 26.0f;
    CameraBoom->bUseCameraLagSubstepping = true;
    CameraBoom->CameraLagMaxTimeStep = 0.016f;
'@

if ($Character.Contains($OldBoom)) {
    $Character = $Character.Replace($OldBoom, $NewBoom)
}

$CameraAnchor = '    Camera->bUsePawnControlRotation = false;'
$CameraBlock = @'
    Camera->bUsePawnControlRotation = false;
    Camera->SetFieldOfView(40.0f);
    Camera->PostProcessBlendWeight = 1.0f;

    FPostProcessSettings& CinematicPP = Camera->PostProcessSettings;
    CinematicPP.bOverride_AutoExposureMethod = true;
    CinematicPP.AutoExposureMethod = AEM_Histogram;
    CinematicPP.bOverride_AutoExposureMinBrightness = true;
    CinematicPP.bOverride_AutoExposureMaxBrightness = true;
    CinematicPP.AutoExposureMinBrightness = 0.82f;
    CinematicPP.AutoExposureMaxBrightness = 0.82f;
    CinematicPP.bOverride_AutoExposureBias = true;
    CinematicPP.AutoExposureBias = -0.65f;
    CinematicPP.bOverride_BloomIntensity = true;
    CinematicPP.BloomIntensity = 0.34f;
    CinematicPP.bOverride_BloomThreshold = true;
    CinematicPP.BloomThreshold = 1.2f;
    CinematicPP.bOverride_VignetteIntensity = true;
    CinematicPP.VignetteIntensity = 0.18f;
    CinematicPP.bOverride_AmbientOcclusionIntensity = true;
    CinematicPP.AmbientOcclusionIntensity = 0.82f;
    CinematicPP.bOverride_AmbientOcclusionRadius = true;
    CinematicPP.AmbientOcclusionRadius = 145.0f;
    CinematicPP.bOverride_MotionBlurAmount = true;
    CinematicPP.MotionBlurAmount = 0.08f;
    CinematicPP.bOverride_FilmSlope = true;
    CinematicPP.FilmSlope = 0.82f;
    CinematicPP.bOverride_FilmToe = true;
    CinematicPP.FilmToe = 0.50f;
    CinematicPP.bOverride_FilmShoulder = true;
    CinematicPP.FilmShoulder = 0.24f;
'@

if (-not $Character.Contains('CinematicPP.bOverride_AutoExposureMethod')) {
    if (-not $Character.Contains($CameraAnchor)) {
        throw "Camera post-process anchor was not found in ArashCharacter.cpp"
    }
    $Character = $Character.Replace($CameraAnchor, $CameraBlock.TrimEnd("`r", "`n"))
}

if ($Character -ne $CharacterOriginal) {
    Write-Source $CharacterPath $Character
    Write-Host "[ARASH Cinematic] Patched player camera and post process."
}
else {
    Write-Host "[ARASH Cinematic] Player camera patch already applied."
}

# -----------------------------------------------------------------------------
# World lighting, volumetric fog and hero separation lights.
# -----------------------------------------------------------------------------
$Environment = Read-Source $EnvironmentPath
$EnvironmentOriginal = $Environment

$RequiredIncludes = @(
    '#include "Components/ExponentialHeightFogComponent.h"',
    '#include "Engine/ExponentialHeightFog.h"',
    '#include "GameFramework/Pawn.h"'
)

foreach ($Include in $RequiredIncludes) {
    if (-not $Environment.Contains($Include)) {
        $Environment = $Environment.Replace(
            '#include "Components/DirectionalLightComponent.h"',
            "#include `"Components/DirectionalLightComponent.h`"`r`n$Include")
    }
}

$TunePattern = '(?s)void AArashEnvironmentManager::TuneLighting\(\)\s*\{.*?\r?\n\}\r?\n\r?\nvoid AArashEnvironmentManager::SpawnFireLight'
$TuneReplacement = @'
void AArashEnvironmentManager::TuneLighting()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("r.EyeAdaptationQuality 2"));
    UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("r.Tonemapper.Sharpen 0.28"));
    UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("r.VolumetricFog.GridPixelSize 16"));
    UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("r.VolumetricFog.GridSizeZ 64"));

    for (TActorIterator<ADirectionalLight> It(World); It; ++It)
    {
        It->SetActorRotation(FRotator(-38.0f, -34.0f, 0.0f));

        if (UDirectionalLightComponent* Light = It->FindComponentByClass<UDirectionalLightComponent>())
        {
            Light->SetIntensity(3.15f);
            Light->SetLightColor(FLinearColor(1.0f, 0.58f, 0.31f));
            Light->SetShadowAmount(0.86f);
            Light->SetIndirectLightingIntensity(1.12f);
            Light->SetVolumetricScatteringIntensity(1.1f);
        }
    }

    for (TActorIterator<ASkyLight> It(World); It; ++It)
    {
        if (USkyLightComponent* Light = It->GetLightComponent())
        {
            Light->SetIntensity(0.72f);
            Light->SetLightColor(FLinearColor(0.47f, 0.59f, 0.78f));
        }
    }

    AExponentialHeightFog* HeightFog = nullptr;
    for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
    {
        HeightFog = *It;
        break;
    }

    if (!HeightFog)
    {
        FActorSpawnParameters FogParams;
        FogParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        FogParams.ObjectFlags |= RF_Transient;
        HeightFog = World->SpawnActor<AExponentialHeightFog>(FVector(0.0f, 0.0f, -120.0f), FRotator::ZeroRotator, FogParams);
    }

    if (HeightFog)
    {
        if (UExponentialHeightFogComponent* Fog = HeightFog->GetComponent())
        {
            Fog->SetFogDensity(0.014f);
            Fog->SetFogHeightFalloff(0.22f);
            Fog->SetFogInscatteringColor(FLinearColor(0.12f, 0.075f, 0.045f));
            Fog->SetFogMaxOpacity(0.58f);
            Fog->SetStartDistance(280.0f);
            Fog->SetVolumetricFog(true);
            Fog->SetVolumetricFogStartDistance(180.0f);
            Fog->SetVolumetricFogNearFadeInDistance(240.0f);
            Fog->SetVolumetricFogDistance(4800.0f);
            Fog->SetVolumetricFogScatteringDistribution(0.34f);
            Fog->SetVolumetricFogAlbedo(FColor(205, 171, 132));
            Fog->SetVolumetricFogExtinctionScale(0.58f);
            Fog->SetVolumetricFogEmissive(FLinearColor(0.004f, 0.0015f, 0.0007f));
        }
    }

    if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        auto SpawnAttachedLight = [World, PlayerPawn](
            const FVector& RelativeLocation,
            const FLinearColor& Color,
            float Intensity,
            float Radius)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Params.ObjectFlags |= RF_Transient;

            APointLight* LightActor = World->SpawnActor<APointLight>(PlayerPawn->GetActorLocation(), FRotator::ZeroRotator, Params);
            if (!LightActor || !LightActor->PointLightComponent)
            {
                return;
            }

            LightActor->Tags.Add(ArashEnvironment::GeneratedTag);
            LightActor->AttachToActor(PlayerPawn, FAttachmentTransformRules::KeepWorldTransform);
            LightActor->SetActorRelativeLocation(RelativeLocation);

            UPointLightComponent* Light = LightActor->PointLightComponent;
            Light->SetMobility(EComponentMobility::Movable);
            Light->SetIntensity(Intensity);
            Light->SetAttenuationRadius(Radius);
            Light->SetLightColor(Color);
            Light->SetCastShadows(false);
            Light->SetVolumetricScatteringIntensity(0.35f);
        };

        SpawnAttachedLight(FVector(-115.0f, 95.0f, 150.0f), FLinearColor(1.0f, 0.24f, 0.06f), 240.0f, 290.0f);
        SpawnAttachedLight(FVector(85.0f, -120.0f, 105.0f), FLinearColor(0.08f, 0.38f, 0.72f), 135.0f, 250.0f);
    }
}

void AArashEnvironmentManager::SpawnFireLight
'@

if (-not [regex]::IsMatch($Environment, $TunePattern)) {
    throw "TuneLighting function could not be located in ArashEnvironmentManager.cpp"
}

if (-not $Environment.Contains('SetVolumetricFogNearFadeInDistance(240.0f)')) {
    $Environment = [regex]::Replace($Environment, $TunePattern, $TuneReplacement, 1)
}

if ($Environment -ne $EnvironmentOriginal) {
    Write-Source $EnvironmentPath $Environment
    Write-Host "[ARASH Cinematic] Patched golden-hour lighting, hero light rig and volumetric fog."
}
else {
    Write-Host "[ARASH Cinematic] World presentation patch already applied."
}

$CharacterVerify = Read-Source $CharacterPath
$EnvironmentVerify = Read-Source $EnvironmentPath

$Checks = @(
    @($CharacterVerify, 'Camera->SetFieldOfView(40.0f);'),
    @($CharacterVerify, 'CinematicPP.BloomIntensity = 0.34f;'),
    @($EnvironmentVerify, 'SetVolumetricFogNearFadeInDistance(240.0f);'),
    @($EnvironmentVerify, 'SpawnAttachedLight(FVector(-115.0f, 95.0f, 150.0f)')
)

foreach ($Check in $Checks) {
    if (-not $Check[0].Contains($Check[1])) {
        throw "Cinematic vertical-slice verification failed: $($Check[1])"
    }
}

Write-Host "[ARASH Cinematic] Cinematic vertical-slice presentation layer is ready for build."
