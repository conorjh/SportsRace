

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
