param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$ManifestPath = Join-Path $PSScriptRoot "cc0_vertical_slice_manifest.json"

if (-not (Test-Path $ManifestPath)) {
    throw "CC0 manifest not found: $ManifestPath"
}

$Manifest = Get-Content -Path $ManifestPath -Raw | ConvertFrom-Json
$CacheRoot = Join-Path $RepoRoot ".art-cache\kaykit-dungeon-remastered"
$ArchivePath = Join-Path $CacheRoot "source.zip"
$ExtractRoot = Join-Path $CacheRoot "source"
$StagingRoot = Join-Path $RepoRoot ($Manifest.staging_root -replace '/', '\')
$StagingAssets = Join-Path $StagingRoot "Assets"
$StagingFbx = Join-Path $StagingAssets "fbx"
$StampPath = Join-Path $StagingRoot "INSTALL.json"

if ($Force) {
    if (Test-Path $CacheRoot) {
        Remove-Item -Path $CacheRoot -Recurse -Force
    }
    if (Test-Path $StagingRoot) {
        Remove-Item -Path $StagingRoot -Recurse -Force
    }
}

New-Item -ItemType Directory -Path $CacheRoot -Force | Out-Null
New-Item -ItemType Directory -Path $StagingFbx -Force | Out-Null

if (-not (Test-Path $ArchivePath)) {
    Write-Host "[ARASH CC0] Downloading $($Manifest.source.name)..."
    Invoke-WebRequest -Uri $Manifest.source.archive_url -OutFile $ArchivePath -UseBasicParsing
}
else {
    Write-Host "[ARASH CC0] Reusing cached archive."
}

if (-not (Test-Path $ExtractRoot)) {
    New-Item -ItemType Directory -Path $ExtractRoot -Force | Out-Null
    Expand-Archive -Path $ArchivePath -DestinationPath $ExtractRoot -Force
}

$AssetRootSuffix = ($Manifest.source.asset_root -replace '/', '\')
$AssetRoot = Get-ChildItem -Path $ExtractRoot -Directory -Recurse |
    Where-Object { $_.FullName.EndsWith($AssetRootSuffix, [System.StringComparison]::OrdinalIgnoreCase) } |
    Select-Object -First 1

if (-not $AssetRoot) {
    throw "Could not locate KayKit asset root '$AssetRootSuffix' in extracted archive."
}

$SourceFbx = Join-Path $AssetRoot.FullName "fbx"
if (-not (Test-Path $SourceFbx)) {
    throw "KayKit FBX directory not found: $SourceFbx"
}

$MissingRequired = New-Object System.Collections.Generic.List[string]
$InstalledAssets = New-Object System.Collections.Generic.List[object]

foreach ($Asset in $Manifest.assets) {
    $SourcePath = Join-Path $SourceFbx $Asset.source
    $DestinationPath = Join-Path $StagingFbx $Asset.source

    if (-not (Test-Path $SourcePath)) {
        if ([bool]$Asset.required) {
            $MissingRequired.Add($Asset.source)
        }
        else {
            Write-Warning "[ARASH CC0] Optional asset missing: $($Asset.source)"
        }
        continue
    }

    Copy-Item -Path $SourcePath -Destination $DestinationPath -Force
    $InstalledAssets.Add([ordered]@{
        role = $Asset.role
        source = $Asset.source
        destination = $Asset.destination
    })
}

if ($MissingRequired.Count -gt 0) {
    throw "Required CC0 assets are missing: $($MissingRequired -join ', ')"
}

$TextureDirectories = @("Textures", "textures", "Texture", "texture")
foreach ($DirectoryName in $TextureDirectories) {
    $TextureSource = Join-Path $AssetRoot.FullName $DirectoryName
    if (Test-Path $TextureSource) {
        $TextureDestination = Join-Path $StagingAssets $DirectoryName
        Copy-Item -Path $TextureSource -Destination $TextureDestination -Recurse -Force
    }
}

Get-ChildItem -Path $AssetRoot.FullName -File -Recurse |
    Where-Object { $_.Extension -match '^\.(png|jpg|jpeg|tga)$' } |
    ForEach-Object {
        $Relative = $_.FullName.Substring($AssetRoot.FullName.Length).TrimStart('\')
        $Destination = Join-Path $StagingAssets $Relative
        New-Item -ItemType Directory -Path (Split-Path $Destination -Parent) -Force | Out-Null
        Copy-Item -Path $_.FullName -Destination $Destination -Force
    }

$License = Get-ChildItem -Path $AssetRoot.FullName -File -Recurse |
    Where-Object { $_.Name -match '^LICENSE(\.txt)?$' } |
    Select-Object -First 1

if ($License) {
    Copy-Item -Path $License.FullName -Destination (Join-Path $StagingRoot "LICENSE-KAYKIT-CC0.txt") -Force
}

$InstallStamp = [ordered]@{
    installed_at_utc = [DateTime]::UtcNow.ToString("o")
    source = $Manifest.source
    assets = $InstalledAssets
}

$InstallStamp | ConvertTo-Json -Depth 8 | Set-Content -Path $StampPath -Encoding UTF8

Write-Host "[ARASH CC0] Prepared $($InstalledAssets.Count) assets in:"
Write-Host "[ARASH CC0] $StagingRoot"
