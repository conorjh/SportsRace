Write-Host "Copying include folders from submodules"

if (Test-Path "include")    {	$null = Remove-Item -Path "include" -Force -Recurse	} 
$null = mkdir include

#copy over dependency include directories
Copy-Item -Path 'external\SDL\include\*' -Destination 'include' -recurse -Force
Copy-Item -Path 'external\SDL_image\include\*' -Destination 'include' -recurse -Force
Copy-Item -Path 'external\SDL_mixer\include\*' -Destination 'include' -recurse -Force
Copy-Item -Path 'external\SDL_net\include\*' -Destination 'include' -recurse -Force
Copy-Item -Path 'external\SDL_ttf\include\*' -Destination 'include' -recurse -Force
Copy-Item -Path 'external\spdlog\include\*' -Destination 'include' -recurse -Force
Copy-Item -Path 'external\tree.hh\src\tree.hh' -Destination 'include\tree.hh' -recurse -Force

