
$ErrorActionPreference = 'Stop'

$CMakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
if ($CMakeCommand -ne $null)
{
	$CMakePath = $CMakeCommand.Source
}
else
{
	$VsWherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
	if (!(Test-Path -LiteralPath $VsWherePath))
	{
		throw 'CMake was not found on PATH and vswhere.exe is unavailable.'
	}

	$VisualStudioPath = & $VsWherePath -latest -prerelease -products '*' -property installationPath
	$CMakePath = Join-Path $VisualStudioPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
	if (!(Test-Path -LiteralPath $CMakePath))
	{
		throw 'CMake was not found in the latest Visual Studio installation.'
	}
}

$Generator = 'Visual Studio 18 2026'
$SourceRoot = Join-Path $PSScriptRoot 'external'
$BuildRoot = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'SportsRace\SDL-static'

function Invoke-CMake
{
	param([Parameter(Mandatory = $true)][string[]]$Arguments)
	& $script:CMakePath @Arguments
	if ($LASTEXITCODE -ne 0)
	{
		throw "CMake failed with exit code $LASTEXITCODE."
	}
}

function Configure-StaticProject
{
	param(
		[string]$Source,
		[string]$Build,
		[string]$Platform,
		[string[]]$Options
	)
	$OutputRoot = Join-Path $PSScriptRoot "external\static\$Platform"
	$Arguments = @(
		'-S', $Source,
		'-B', $Build,
		'-G', $script:Generator,
		'-A', $Platform,
		"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG=$(Join-Path $OutputRoot 'Debug')",
		"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE=$(Join-Path $OutputRoot 'Release')"
	) + $Options
	Invoke-CMake -Arguments $Arguments
}

function Build-StaticTarget
{
	param([string]$Build, [string]$Target)
	foreach ($Configuration in @('Debug', 'Release'))
	{
		Invoke-CMake -Arguments @('--build', $Build, '--config', $Configuration, '--target', $Target, '--parallel')
	}
}

$SDLOptions = @(
	'-DSDL_SHARED=OFF',
	'-DSDL_STATIC=ON',
	'-DSDL_INSTALL=OFF',
	'-DSDL_EXAMPLES=OFF',
	'-DSDL_TESTS=OFF',
	'-DSDL_DEPS_SHARED=OFF'
)
$SDLImageOptions = @(
	'-DBUILD_SHARED_LIBS=OFF',
	'-DSDLIMAGE_VENDORED=ON',
	'-DSDLIMAGE_BACKEND_STB=ON',
	'-DSDLIMAGE_SAMPLES=OFF',
	'-DSDLIMAGE_TESTS=OFF',
	'-DSDLIMAGE_AVIF=OFF',
	'-DSDLIMAGE_JXL=OFF',
	'-DSDLIMAGE_SVG=OFF',
	'-DSDLIMAGE_TIF=OFF',
	'-DSDLIMAGE_WEBP=OFF',
	'-DSDLIMAGE_DEPS_SHARED=OFF'
)
$SDLTTFOptions = @(
	'-DBUILD_SHARED_LIBS=OFF',
	'-DSDLTTF_VENDORED=ON',
	'-DSDLTTF_SAMPLES=OFF',
	'-DSDLTTF_HARFBUZZ=OFF',
	'-DSDLTTF_PLUTOSVG=OFF',
	'-DCMAKE_POLICY_VERSION_MINIMUM=3.5'
)
$SDLMixerOptions = @(
	'-DBUILD_SHARED_LIBS=OFF',
	'-DSDLMIXER_VENDORED=ON',
	'-DSDLMIXER_SAMPLES=OFF',
	'-DSDLMIXER_FLAC=OFF',
	'-DSDLMIXER_GME=OFF',
	'-DSDLMIXER_MOD=OFF',
	'-DSDLMIXER_MOD_XMP=OFF',
	'-DSDLMIXER_MP3=ON',
	'-DSDLMIXER_MP3_DRMP3=ON',
	'-DSDLMIXER_MP3_MPG123=OFF',
	'-DSDLMIXER_MIDI=OFF',
	'-DSDLMIXER_OPUS=OFF',
	'-DSDLMIXER_VORBIS=OFF',
	'-DSDLMIXER_WAVE=ON',
	'-DSDLMIXER_WAVPACK=OFF',
	'-DSDLMIXER_DEPS_SHARED=OFF'
)

foreach ($Platform in @('x64', 'Win32'))
{
	$PlatformBuildRoot = Join-Path $BuildRoot $Platform
	$SDLBuild = Join-Path $PlatformBuildRoot 'SDL'
	$SDLImageBuild = Join-Path $PlatformBuildRoot 'SDL_image'
	$SDLTTFBuild = Join-Path $PlatformBuildRoot 'SDL_ttf'
	$SDLMixerBuild = Join-Path $PlatformBuildRoot 'SDL_mixer'

	Write-Host "Configuring static SDL libraries for $Platform"
	Configure-StaticProject -Source (Join-Path $SourceRoot 'SDL') -Build $SDLBuild -Platform $Platform -Options $SDLOptions
	Build-StaticTarget -Build $SDLBuild -Target 'SDL3-static'

	Configure-StaticProject -Source (Join-Path $SourceRoot 'SDL_image') -Build $SDLImageBuild -Platform $Platform `
		-Options ($SDLImageOptions + "-DSDL3_DIR=$SDLBuild")
	Build-StaticTarget -Build $SDLImageBuild -Target 'SDL3_image-static'

	Configure-StaticProject -Source (Join-Path $SourceRoot 'SDL_ttf') -Build $SDLTTFBuild -Platform $Platform `
		-Options ($SDLTTFOptions + "-DSDL3_DIR=$SDLBuild")
	Build-StaticTarget -Build $SDLTTFBuild -Target 'SDL3_ttf-static'

	Configure-StaticProject -Source (Join-Path $SourceRoot 'SDL_mixer') -Build $SDLMixerBuild -Platform $Platform `
		-Options ($SDLMixerOptions + "-DSDL3_DIR=$SDLBuild")
	Build-StaticTarget -Build $SDLMixerBuild -Target 'SDL3_mixer-static'

	foreach ($Configuration in @('Debug', 'Release'))
	{
		$OutputDirectory = Join-Path (Join-Path $PSScriptRoot "external\static\$Platform") $Configuration
		foreach ($Archive in @('SDL3-static.lib', 'SDL3_image-static.lib', 'SDL3_ttf-static.lib', 'SDL3_mixer-static.lib'))
		{
			if (!(Test-Path -LiteralPath (Join-Path $OutputDirectory $Archive)))
			{
				throw "Expected static library '$Archive' was not produced for $Platform $Configuration."
			}
		}
	}
	Write-Host "Static SDL libraries are ready for $Platform."
}
