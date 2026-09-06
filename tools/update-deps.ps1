<#
update-deps.ps1
Helper script to bootstrap vcpkg and install core runtime dependencies for SportsRace on Windows.
Run from repository root: PowerShell -ExecutionPolicy Bypass -File .\tools\update-deps.ps1
#>
param(
    [switch]$BootstrapVcpkg = $true,
    [string]$VcpkgPath = "$PSScriptRoot\\vcpkg",
    [string]$Triplet = "x64-windows"
)

function Write-Log($msg){ Write-Host "[update-deps] $msg" }

Push-Location $PSScriptRoot.. | Out-Null
$RepoRoot = (Get-Location).ProviderPath
Write-Log "Repo root: $RepoRoot"

# Ensure git is available
if (-not (Get-Command git -ErrorAction SilentlyContinue)){
    Write-Error "git is required but not found in PATH. Install Git for Windows and re-run."
    exit 2
}

# Bootstrap vcpkg if requested and not present
if ($BootstrapVcpkg -and -not (Test-Path $VcpkgPath)){
    Write-Log "Cloning vcpkg into $VcpkgPath"
    git clone https://github.com/microsoft/vcpkg.git $VcpkgPath
    if ($LASTEXITCODE -ne 0){ Write-Error "Failed to clone vcpkg"; exit 3 }
    Push-Location $VcpkgPath
    Write-Log "Bootstrapping vcpkg (this may take a few minutes)"
    & .\bootstrap-vcpkg.bat
    if ($LASTEXITCODE -ne 0){ Write-Error "vcpkg bootstrap failed"; Pop-Location; exit 4 }
    Pop-Location
} else {
    Write-Log "Using existing vcpkg at $VcpkgPath"
}

# vcpkg executable path
$vcpkgExe = Join-Path $VcpkgPath "vcpkg.exe"
if (-not (Test-Path $vcpkgExe)){
    Write-Error "vcpkg.exe not found at $vcpkgExe. Rerun with -BootstrapVcpkg or check path."
    exit 5
}

# Packages to install (core runtime libs used by the project)
$pkgs = @(
    "sdl2",
    "sdl2-image",
    "sdl2-ttf",
    "sdl2-mixer",
    "spdlog"
)

foreach ($p in $pkgs){
    $spec = "$p:$Triplet"
    Write-Log "Installing $spec"
    & $vcpkgExe install $spec
    if ($LASTEXITCODE -ne 0){ Write-Error "Failed to install $spec"; exit 6 }
}

Write-Log "Integration helper: run `vcpkg integrate install` manually if you want global MSBuild integration"
Write-Log "Or set VCPKG_ROOT=$VcpkgPath and add $VcpkgPath\installed\$Triplet\bin to PATH for local usage."

Write-Log "Done. You can now retarget projects to use vcpkg libraries or update project AdditionalIncludeDirectories / Linker paths to point into $VcpkgPath\installed\$Triplet"
Pop-Location | Out-Null
