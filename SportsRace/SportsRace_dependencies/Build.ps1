$VsPath = "$Env:ProgramFiles\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe"
#build SDL 
& "$VsPath" ".\SDL\VisualC\SDL.sln" /build "Debug"
& "$VsPath" ".\SDL\VisualC\SDL.sln" /build "Release"
#output is in freetype-master\objs

#build SDL_image
& "$VsPath" ".\SDL_image\VisualC\SDL_image.sln" /build "Debug"
& "$VsPath" ".\SDL_image\VisualC\SDL_image.sln" /build "Release"

#build SDL_mixer
& "$VsPath" ".\SDL_mixer\VisualC\SDL_mixer.sln" /build "Debug"
& "$VsPath" ".\SDL_mixer\VisualC\SDL_mixer.sln" /build "Release"

#build SDL_net
& "$VsPath" ".\SDL_net\VisualC\SDL_net.sln" /build "Debug"
& "$VsPath" ".\SDL_net\VisualC\SDL_net.sln" /build "Release"

#build SDL_ttf
& "$VsPath" ".\SDL_ttf\VisualC\SDL_ttf.sln" /build "Debug"
& "$VsPath" ".\SDL_ttf\VisualC\SDL_ttf.sln" /build "Release"
