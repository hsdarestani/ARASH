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
$GitSourceRoot = Join-Path $CacheRoot "git-source"
$StagingRoot = Join-Path $RepoRoot ($Manifest.staging_root -replace '/', '\')
$StagingAssets = Join-Path $StagingRoot "Assets"
$StagingFbx = Join-Path $StagingAssets "fbx"
$StampPath = Join-Path $StagingRoot "INSTALL.json"

function Find-KayKitFbxDirectory([string]$SearchRoot) {
    if (-not (Test-Path $SearchRoot)) {
        return $null
    }

    $Sentinel = Get-ChildItem -Path $SearchRoot -File -Recurse -Filter "floor_tile_large.fbx" -ErrorAction SilentlyContinue |
        Select-Object -First 1

    if ($Sentinel) {
        return $Sentinel.Directory
    }

    return $null
}

function Invoke-Git([string[]]$Arguments) {
    & git @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "git command failed with exit code ${LASTEXITCODE}: git $($Arguments -join ' ')"
    }
}

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

# GitHub source archives do not always contain Git LFS payloads. First try to
# locate the real FBX payload in the extracted archive without assuming the
# archive's root-directory name or exact casing.
$SourceFbx = Find-KayKitFbxDirectory $ExtractRoot

if (-not $SourceFbx) {
    Write-Warning "[ARASH CC0] FBX payload was not present in the GitHub source archive; using a pinned Git LFS checkout."

    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw "Git is required for the KayKit LFS fallback, but git.exe was not found in PATH."
    }

    if (-not (Test-Path (Join-Path $GitSourceRoot ".git"))) {
        if (Test-Path $GitSourceRoot) {
            Remove-Item -Path $GitSourceRoot -Recurse -Force
        }

        $RepositoryUrl = "https://github.com/$($Manifest.source.repository).git"
        Invoke-Git @("clone", "--no-checkout", $RepositoryUrl, $GitSourceRoot)
    }

    Invoke-Git @("-C", $GitSourceRoot, "fetch", "--depth", "1", "origin", $Manifest.source.commit)
    Invoke-Git @("-C", $GitSourceRoot, "checkout", "--force", "--detach", "FETCH_HEAD")

    if (Get-Command git-lfs -ErrorAction SilentlyContinue) {
        Invoke-Git @("-C", $GitSourceRoot, "lfs", "pull")
    }
    else {
        # `git lfs` can still be available as a git subcommand even when a
        # standalone git-lfs executable is not discoverable via PowerShell.
        & git -C $GitSourceRoot lfs version *> $null
        if ($LASTEXITCODE -eq 0) {
            Invoke-Git @("-C", $GitSourceRoot, "lfs", "pull")
        }
    }

    $SourceFbx = Find-KayKitFbxDirectory $GitSourceRoot
}

if (-not $SourceFbx) {
    throw "Could not locate the KayKit FBX payload after archive extraction and pinned Git checkout."
}

$AssetRoot = $SourceFbx.Parent
Write-Host "[ARASH CC0] Using KayKit FBX directory: $($SourceFbx.FullName)"

$MissingRequired = New-Object System.Collections.Generic.List[string]
$InstalledAssets = New-Object System.Collections.Generic.List[object]

foreach ($Asset in $Manifest.assets) {
    $SourcePath = Join-Path $SourceFbx.FullName $Asset.source
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

$LicenseSearchRoot = if (Test-Path $GitSourceRoot) { $GitSourceRoot } else { $ExtractRoot }
$License = Get-ChildItem -Path $LicenseSearchRoot -File -Recurse |
    Where-Object { $_.Name -match '^LICENSE(\.txt)?$' } |
    Select-Object -First 1

if ($License) {
    Copy-Item -Path $License.FullName -Destination (Join-Path $StagingRoot "LICENSE-KAYKIT-CC0.txt") -Force
}

$InstallStamp = [ordered]@{
    installed_at_utc = [DateTime]::UtcNow.ToString("o")
    source = $Manifest.source
    source_fbx = $SourceFbx.FullName
    assets = $InstalledAssets
}

$InstallStamp | ConvertTo-Json -Depth 8 | Set-Content -Path $StampPath -Encoding UTF8

Write-Host "[ARASH CC0] Prepared $($InstalledAssets.Count) assets in:"
Write-Host "[ARASH CC0] $StagingRoot"
