param(
    [ValidateSet("2k", "4k", "8k")]
    [string]$Resolution = "4k"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$OutputRoot = Join-Path $ProjectRoot "ArtSource\PolyHaven"
$UserAgent = "ARASH-ArtPipeline/0.1 (github.com/hsdarestani/ARASH)"
$Headers = @{ "User-Agent" = $UserAgent }

$Assets = @(
    @{ Id = "red_sandstone_wall"; Type = "texture"; Role = "weathered warm wall stone" },
    @{ Id = "sandstone_blocks_05"; Type = "texture"; Role = "large architectural sandstone blocks" },
    @{ Id = "marble_01"; Type = "texture"; Role = "light court floor / polished accents" },
    @{ Id = "metal_plate_02"; Type = "texture"; Role = "weathered metal base for bronze/gold trims" },
    @{ Id = "dikhololo_sunset"; Type = "hdri"; Role = "warm low-contrast sunset lighting reference" }
)

function Get-LeafDownloads {
    param(
        [Parameter(Mandatory = $true)] $Node,
        [string]$Path = ""
    )

    $Results = @()
    if ($null -eq $Node) { return $Results }

    if ($Node -is [System.Management.Automation.PSCustomObject]) {
        $Names = @($Node.PSObject.Properties.Name)
        if ($Names -contains "url") {
            return ,([PSCustomObject]@{
                Path = $Path
                Url  = [string]$Node.url
                Size = if ($Names -contains "size") { [long]$Node.size } else { 0 }
                Md5  = if ($Names -contains "md5") { [string]$Node.md5 } else { "" }
            })
        }

        foreach ($Property in $Node.PSObject.Properties) {
            $NextPath = if ($Path) { "$Path/$($Property.Name)" } else { $Property.Name }
            $Results += Get-LeafDownloads -Node $Property.Value -Path $NextPath
        }
    }
    elseif ($Node -is [System.Collections.IDictionary]) {
        foreach ($Key in $Node.Keys) {
            $NextPath = if ($Path) { "$Path/$Key" } else { [string]$Key }
            $Results += Get-LeafDownloads -Node $Node[$Key] -Path $NextPath
        }
    }
    elseif (($Node -is [System.Collections.IEnumerable]) -and -not ($Node -is [string])) {
        $Index = 0
        foreach ($Item in $Node) {
            $Results += Get-LeafDownloads -Node $Item -Path "$Path/$Index"
            $Index++
        }
    }

    return $Results
}

function Save-Download {
    param(
        [Parameter(Mandatory = $true)] $Leaf,
        [Parameter(Mandatory = $true)] [string]$Folder
    )

    $Uri = [Uri]$Leaf.Url
    $FileName = [IO.Path]::GetFileName($Uri.AbsolutePath)
    $Destination = Join-Path $Folder $FileName

    if (Test-Path $Destination) {
        if ($Leaf.Md5) {
            $ExistingHash = (Get-FileHash -Algorithm MD5 -Path $Destination).Hash.ToLowerInvariant()
            if ($ExistingHash -eq $Leaf.Md5.ToLowerInvariant()) {
                Write-Host "  OK  $FileName (cached)"
                return $Destination
            }
        }
        elseif ((Get-Item $Destination).Length -eq $Leaf.Size -and $Leaf.Size -gt 0) {
            Write-Host "  OK  $FileName (cached)"
            return $Destination
        }
    }

    Write-Host "  GET $FileName"
    Invoke-WebRequest -Uri $Leaf.Url -OutFile $Destination -Headers $Headers

    if ($Leaf.Md5) {
        $DownloadedHash = (Get-FileHash -Algorithm MD5 -Path $Destination).Hash.ToLowerInvariant()
        if ($DownloadedHash -ne $Leaf.Md5.ToLowerInvariant()) {
            Remove-Item $Destination -Force
            throw "Checksum mismatch for $FileName"
        }
    }

    return $Destination
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
$Manifest = @()

Write-Host "ARASH CC0 art bootstrap"
Write-Host "Powered by Poly Haven - https://polyhaven.com"
Write-Host "Resolution: $Resolution"
Write-Host "Output: $OutputRoot"
Write-Host ""

foreach ($Asset in $Assets) {
    $Id = $Asset.Id
    $AssetFolder = Join-Path $OutputRoot $Id
    New-Item -ItemType Directory -Force -Path $AssetFolder | Out-Null

    Write-Host "[$Id] $($Asset.Role)"
    $Files = Invoke-RestMethod -Uri "https://api.polyhaven.com/files/$Id" -Headers $Headers
    $Leaves = @(Get-LeafDownloads -Node $Files)
    $Selected = @()

    if ($Asset.Type -eq "hdri") {
        $Selected = @($Leaves | Where-Object {
            $_.Path -eq "hdri/$Resolution/hdr" -or
            ($_.Url -match "_${Resolution}\.hdr$")
        } | Select-Object -First 1)
    }
    else {
        # Unreal prefers DirectX normals. ARM is ideal when available; the fallbacks
        # keep AO/Rough/Metal as separate maps for materials that do not ship ARM.
        $Tokens = @("diff", "nor_dx", "arm", "ao", "rough", "metal", "disp")
        foreach ($Token in $Tokens) {
            $Candidate = $Leaves | Where-Object {
                $_.Url -match "_${Token}_${Resolution}\.(jpg|png|exr)$"
            } | Sort-Object @{ Expression = {
                if ($_.Url.EndsWith(".jpg")) { 0 }
                elseif ($_.Url.EndsWith(".png")) { 1 }
                else { 2 }
            }} | Select-Object -First 1

            if ($Candidate) { $Selected += $Candidate }
        }
    }

    if (-not $Selected -or $Selected.Count -eq 0) {
        throw "No suitable $Resolution downloads found for $Id"
    }

    $Downloaded = @()
    foreach ($Leaf in ($Selected | Sort-Object Url -Unique)) {
        $Downloaded += Save-Download -Leaf $Leaf -Folder $AssetFolder
    }

    $Manifest += [PSCustomObject]@{
        id = $Id
        type = $Asset.Type
        role = $Asset.Role
        source = "https://polyhaven.com/a/$Id"
        license = "CC0"
        resolution = $Resolution
        files = @($Downloaded | ForEach-Object { [IO.Path]::GetFileName($_) })
    }

    Write-Host ""
}

$ManifestPath = Join-Path $OutputRoot "ARASH_CC0_MANIFEST.json"
$Manifest | ConvertTo-Json -Depth 6 | Set-Content -Path $ManifestPath -Encoding UTF8

Write-Host "Done."
Write-Host "Manifest: $ManifestPath"
Write-Host "Raw downloads stay outside Git; imported Unreal assets should be committed with Git LFS."
