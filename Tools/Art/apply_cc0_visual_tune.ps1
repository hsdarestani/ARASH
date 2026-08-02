$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$SourcePath = Join-Path $RepoRoot "Source\ARASH\Environment\ArashEnvironmentManager.cpp"
$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)

if (-not (Test-Path $SourcePath)) {
    throw "Environment source not found: $SourcePath"
}

$Source = Get-Content -Path $SourcePath -Raw
$Original = $Source

$CreateMaterialsPattern = '(?s)void AArashEnvironmentManager::CreateAccentMaterials\(\)\s*\{.*?\r?\n\}\r?\n\r?\nvoid AArashEnvironmentManager::ApplyMaterials'
$CreateMaterialsReplacement = @'
void AArashEnvironmentManager::CreateAccentMaterials()
{
    if (!BasicShapeMaterial)
    {
        return;
    }

    FloorFallbackMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
    WallFallbackMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
    StoneFallbackMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
    MetalFallbackMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
    GoldMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
    TurquoiseMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
    CrimsonMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
    EmberMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);

    auto SetColor = [](UMaterialInstanceDynamic* Material, const FLinearColor& Color)
    {
        if (Material)
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
        }
    };

    // Restrained Persian dark-fantasy palette. These materials are used only
    // when the optional PolyHaven materials are not present in the project.
    SetColor(FloorFallbackMaterial, FLinearColor(0.040f, 0.055f, 0.068f));
    SetColor(WallFallbackMaterial, FLinearColor(0.19f, 0.072f, 0.030f));
    SetColor(StoneFallbackMaterial, FLinearColor(0.105f, 0.095f, 0.082f));
    SetColor(MetalFallbackMaterial, FLinearColor(0.028f, 0.042f, 0.052f));
    SetColor(GoldMaterial, FLinearColor(0.62f, 0.24f, 0.025f));
    SetColor(TurquoiseMaterial, FLinearColor(0.005f, 0.16f, 0.19f));
    SetColor(CrimsonMaterial, FLinearColor(0.17f, 0.004f, 0.010f));
    SetColor(EmberMaterial, FLinearColor(0.95f, 0.045f, 0.004f));
}

void AArashEnvironmentManager::ApplyMaterials
'@

if (-not $Source.Contains('FloorFallbackMaterial = UMaterialInstanceDynamic::Create')) {
    if (-not [regex]::IsMatch($Source, $CreateMaterialsPattern)) {
        throw "CreateAccentMaterials function could not be located."
    }
    $Source = [regex]::Replace($Source, $CreateMaterialsPattern, $CreateMaterialsReplacement, 1)
}

$SlotAnchor = '    const TArray<FName> SlotNames = MeshComponent->GetMaterialSlotNames();'
$SlotReplacement = @'
    const TArray<FName> SlotNames = MeshComponent->GetMaterialSlotNames();
    auto ResolveMaterial = [](UMaterialInterface* Preferred, UMaterialInterface* Fallback)
    {
        return Preferred ? Preferred : Fallback;
    };
'@

if (-not $Source.Contains('auto ResolveMaterial = []')) {
    if (-not $Source.Contains($SlotAnchor)) {
        throw "Material slot anchor could not be located."
    }
    $Source = $Source.Replace($SlotAnchor, $SlotReplacement.TrimEnd("`r", "`n"))
}

$Source = $Source.Replace(
    'CourtFloorMaterial.Get()',
    'ResolveMaterial(CourtFloorMaterial.Get(), FloorFallbackMaterial.Get())')
$Source = $Source.Replace(
    'CourtWallMaterial.Get()',
    'ResolveMaterial(CourtWallMaterial.Get(), WallFallbackMaterial.Get())')
$Source = $Source.Replace(
    'CourtStoneMaterial.Get()',
    'ResolveMaterial(CourtStoneMaterial.Get(), StoneFallbackMaterial.Get())')
$Source = $Source.Replace(
    'CourtMetalMaterial.Get()',
    'ResolveMaterial(CourtMetalMaterial.Get(), MetalFallbackMaterial.Get())')

$Source = $Source.Replace('Light->SetIntensity(2.4f);', 'Light->SetIntensity(1.55f);')
$Source = $Source.Replace(
    'Light->SetLightColor(FLinearColor(1.0f, 0.64f, 0.42f));',
    'Light->SetLightColor(FLinearColor(1.0f, 0.52f, 0.27f));')
$Source = $Source.Replace('Light->SetIntensity(0.75f);', 'Light->SetIntensity(0.42f);')
$Source = $Source.Replace('Light->SetIntensity(420.0f);', 'Light->SetIntensity(190.0f);')
$Source = $Source.Replace(
    '[ARASH Environment] Blender kit assembled with optimized materials and lighting.',
    '[ARASH Environment] CC0 kit assembled with Persian palette and low-VRAM lighting.')

if ($Source -ne $Original) {
    [System.IO.File]::WriteAllText($SourcePath, $Source, $Utf8NoBom)
    Write-Host "[ARASH CC0] Applied Persian palette and low-VRAM lighting tune."
}
else {
    Write-Host "[ARASH CC0] Runtime visual tune already applied."
}

$Verify = Get-Content -Path $SourcePath -Raw
$RequiredMarkers = @(
    'FloorFallbackMaterial = UMaterialInstanceDynamic::Create',
    'ResolveMaterial(CourtFloorMaterial.Get(), FloorFallbackMaterial.Get())',
    'Light->SetIntensity(1.55f);',
    'Light->SetIntensity(190.0f);'
)

foreach ($Marker in $RequiredMarkers) {
    if (-not $Verify.Contains($Marker)) {
        throw "Runtime visual tune verification failed: $Marker"
    }
}
