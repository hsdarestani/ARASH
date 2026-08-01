$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Target = Join-Path $RepoRoot "Source\ARASH\Environment\ArashEnvironmentManager.cpp"

if (-not (Test-Path $Target)) {
    throw "Target source file was not found: $Target"
}

$Content = Get-Content -Path $Target -Raw
$Original = $Content
$Applied = 0

function Replace-Required {
    param(
        [string]$Old,
        [string]$New,
        [string]$Label
    )

    if ($script:Content.Contains($Old)) {
        $script:Content = $script:Content.Replace($Old, $New)
        $script:Applied++
        Write-Host "[ARASH] Patched $Label"
    }
}

Replace-Required `
    'r.EyeAdaptationQuality 0' `
    'r.EyeAdaptationQuality 2' `
    'eye adaptation'

$DirectionalPattern = [regex]::Escape('Light->SetIntensity(3.2f);') + '\s*' +
    [regex]::Escape('Light->SetLightColor(FLinearColor(1.0f, 0.58f, 0.36f));') + '\s*' +
    [regex]::Escape('Light->SetVolumetricScatteringIntensity(0.55f);')

$DirectionalNew = @"
Light->SetIntensity(4.0f);
            Light->SetLightColor(FLinearColor(1.0f, 0.72f, 0.52f));
            Light->SetVolumetricScatteringIntensity(0.35f);
            Light->SetShadowAmount(0.68f);
            Light->SetIndirectLightingIntensity(1.35f);
"@

if ([regex]::IsMatch($Content, $DirectionalPattern)) {
    $Content = [regex]::Replace($Content, $DirectionalPattern, $DirectionalNew, 1)
    $Applied++
    Write-Host "[ARASH] Patched directional-light balance"
}

Replace-Required `
    'Light->SetIntensity(0.42f);' `
    'Light->SetIntensity(1.15f);' `
    'skylight fill'

Replace-Required `
    'Light->SetIntensity(720.0f);' `
    'Light->SetIntensity(480.0f);' `
    'brazier intensity'

Replace-Required `
    'Light->SetAttenuationRadius(360.0f);' `
    'Light->SetAttenuationRadius(300.0f);' `
    'brazier radius'

if ($Content -eq $Original) {
    if (
        $Content.Contains('r.EyeAdaptationQuality 2') -and
        $Content.Contains('Light->SetIntensity(4.0f);') -and
        $Content.Contains('Light->SetIntensity(1.15f);') -and
        $Content.Contains('Light->SetShadowAmount(0.68f);')
    ) {
        Write-Host "[ARASH] Environment lighting v2 is already applied."
        exit 0
    }

    throw "No expected lighting patterns were found. Refusing to modify the source file."
}

$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($Target, $Content, $Utf8NoBom)

$Verify = Get-Content -Path $Target -Raw
$Required = @(
    'r.EyeAdaptationQuality 2',
    'Light->SetIntensity(4.0f);',
    'Light->SetIntensity(1.15f);',
    'Light->SetShadowAmount(0.68f);',
    'Light->SetIndirectLightingIntensity(1.35f);'
)

foreach ($Pattern in $Required) {
    if (-not $Verify.Contains($Pattern)) {
        throw "Lighting patch verification failed. Missing pattern: $Pattern"
    }
}

Write-Host "[ARASH] Applied environment lighting v2 ($Applied changes):"
Write-Host $Target
Write-Host "[ARASH] Auto exposure restored, skylight raised, and shadows softened."
