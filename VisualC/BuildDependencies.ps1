Write-Host "Create lib directory"
if (Test-Path "lib") 	{Remove-Item -Path "lib" -Force -Recurse}
$null = mkdir external\lib\Win32\Debug
$null = mkdir external\lib\x64\Debug
$null = mkdir external\lib\Win32\Release
$null = mkdir external\lib\x64\Release
$null = mkdir lib

$VsPath = "$Env:ProgramFiles\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe"

Write-Host "Building SDL"
& "$VsPath" ".\external\SDL\VisualC\SDL.sln" /build "Debug|Win32"  | Out-Null
& "$VsPath" ".\external\SDL\VisualC\SDL.sln" /build "Release|Win32" | Out-Null
& "$VsPath" ".\external\SDL\VisualC\SDL.sln" /build "Debug|x64"  | Out-Null
& "$VsPath" ".\external\SDL\VisualC\SDL.sln" /build "Release|x64" | Out-Null
#output is in freetype-master\objs

Write-Host "Building SDL_image"
& "$VsPath" ".\external\SDL_image\VisualC\SDL_image.sln" /build "Debug|Win32"  | Out-Null
& "$VsPath" ".\external\SDL_image\VisualC\SDL_image.sln" /build "Release|Win32" | Out-Null
& "$VsPath" ".\external\SDL_image\VisualC\SDL_image.sln" /build "Debug|x64"  | Out-Null
& "$VsPath" ".\external\SDL_image\VisualC\SDL_image.sln" /build "Release|x64" | Out-Null

Write-Host "Building SDL_mixer"
& "$VsPath" ".\external\SDL_mixer\VisualC\SDL_mixer.sln" /build "Debug|Win32"  | Out-Null
& "$VsPath" ".\external\SDL_mixer\VisualC\SDL_mixer.sln" /build "Release|Win32" | Out-Null
& "$VsPath" ".\external\SDL_mixer\VisualC\SDL_mixer.sln" /build "Debug|x64"  | Out-Null
& "$VsPath" ".\external\SDL_mixer\VisualC\SDL_mixer.sln" /build "Release|x64" | Out-Null

Write-Host "Building SDL_net"
& "$VsPath" ".\external\SDL_net\VisualC\SDL_net.sln" /build "Debug|Win32"  | Out-Null
& "$VsPath" ".\external\SDL_net\VisualC\SDL_net.sln" /build "Release|Win32" | Out-Null
& "$VsPath" ".\external\SDL_net\VisualC\SDL_net.sln" /build "Debug|x64"  | Out-Null
& "$VsPath" ".\external\SDL_net\VisualC\SDL_net.sln" /build "Release|x64" | Out-Null

Write-Host "Building SDL_ttf"

& "$VsPath" ".\external\SDL_ttf\VisualC\SDL_ttf.sln" /build "Debug|Win32"  | Out-Null
& "$VsPath" ".\external\SDL_ttf\VisualC\SDL_ttf.sln" /build "Release|Win32" | Out-Null
& "$VsPath" ".\external\SDL_ttf\VisualC\SDL_ttf.sln" /build "Debug|x64"  | Out-Null
& "$VsPath" ".\external\SDL_ttf\VisualC\SDL_ttf.sln" /build "Release|x64" | Out-Null

Write-Host "Moving output to lib"
$null = move external\SDL\VisualC\Win32\Debug\*.* external\lib\Win32\Debug
$null = move external\SDL\VisualC\x64\Debug\*.* external\lib\x64\Debug
$null = move external\SDL\VisualC\Win32\Release\*.* external\lib\Win32\Release
$null = move external\SDL\VisualC\x64\Release\*.* external\lib\x64\Release

$null = move external\SDL_image\VisualC\Win32\Debug\*.* external\lib\Win32\Debug
$null = move external\SDL_image\VisualC\x64\Debug\*.* external\lib\x64\Debug
$null = move external\SDL_image\VisualC\Win32\Release\*.* external\lib\Win32\Release
$null = move external\SDL_image\VisualC\x64\Release\*.* external\lib\x64\Release

$null = move external\SDL_mixer\VisualC\Win32\Debug\*.* external\lib\Win32\Debug
$null = move external\SDL_mixer\VisualC\x64\Debug\*.* external\lib\x64\Debug
$null = move external\SDL_mixer\VisualC\Win32\Release\*.* external\lib\Win32\Release
$null = move external\SDL_mixer\VisualC\x64\Release\*.* external\lib\x64\Release

$null = move external\SDL_net\VisualC\Win32\Debug\*.* external\lib\Win32\Debug
$null = move external\SDL_net\VisualC\x64\Debug\*.* external\lib\x64\Debug
$null = move external\SDL_net\VisualC\Win32\Release\*.* external\lib\Win32\Release
$null = move external\SDL_net\VisualC\x64\Release\*.* external\lib\x64\Release

$null = move external\SDL_ttf\VisualC\Win32\Debug\*.* external\lib\Win32\Debug
$null = move external\SDL_ttf\VisualC\x64\Debug\*.* external\lib\x64\Debug
$null = move external\SDL_ttf\VisualC\Win32\Release\*.* external\lib\Win32\Release
$null = move external\SDL_ttf\VisualC\x64\Release\*.* external\lib\x64\Release

Move-Item  -Path 'external\lib\*' -Destination 'lib' 
Remove-Item -Path "external\lib" -Force -Recurse