#clear old download folder, old unzipped repos
if (Test-Path "zip") 				{	Remove-Item "zip" -Force -Recurse } 
if (Test-Path "freetype-master")	{	Remove-Item "freetype-master" -Force -Recurse } 
if (Test-Path "harfbuzz-main")		{	Remove-Item "harfbuzz-main" -Force -Recurse } 
Remove-Item -Path "SDL*" -Force -Recurse

#download git copies of all our dependencies
git clone --filter=blob:none https://github.com/gabime/spdlog.git --recursive
git clone --filter=blob:none https://github.com/libsdl-org/SDL.git --recursive
git clone --filter=blob:none https://github.com/libsdl-org/SDL_ttf.git --recursive
& powershell ".\SDL_ttf\external\Get-GitModules.ps1"
git clone --filter=blob:none https://github.com/libsdl-org/SDL_net.git --recursive
git clone --filter=blob:none https://github.com/libsdl-org/SDL_mixer.git --recursive
& powershell ".\SDL_mixer\external\Get-GitModules.ps1"
git clone --filter=blob:none https://github.com/libsdl-org/SDL_image.git --recursive
& powershell ".\SDL_image\external\Get-GitModules.ps1"
