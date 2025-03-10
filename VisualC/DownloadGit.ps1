Write-Host "Downloading dependencies via git"

#clear old download folder, old unzipped repos, old include and lib folders
if (Test-Path "external"){	$null = Remove-Item -Path "external" -Force -Recurse	} 
if (Test-Path "temp")    {	$null = Remove-Item -Path "temp" -Force -Recurse	} 
$null = mkdir temp

cd temp
#download git copies of all our dependencies
git clone --filter=blob:none https://github.com/gabime/spdlog.git --recursive
git clone --filter=blob:none https://github.com/kpeeters/tree.hh.git --recursive
git clone --filter=blob:none https://github.com/libsdl-org/SDL.git --recursive
git clone --filter=blob:none https://github.com/libsdl-org/SDL_ttf.git --recursive
& powershell ".\SDL_ttf\external\Get-GitModules.ps1" 
git clone --filter=blob:none https://github.com/libsdl-org/SDL_net.git --recursive
git clone --filter=blob:none https://github.com/libsdl-org/SDL_mixer.git --recursive
& powershell ".\SDL_mixer\external\Get-GitModules.ps1"
git clone --filter=blob:none https://github.com/libsdl-org/SDL_image.git --recursive
& powershell ".\SDL_image\external\Get-GitModules.ps1"
cd ..

#rename temp to external
Rename-Item -Path 'temp' -NewName 'external'




