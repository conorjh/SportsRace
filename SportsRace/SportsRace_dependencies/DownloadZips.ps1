#clear old download folder, old unzipped repos
if (Test-Path "zip") 				{	Remove-Item "zip" -Force -Recurse } 
if (Test-Path "freetype-master")	{	Remove-Item "freetype-master" -Force -Recurse } 
if (Test-Path "harfbuzz-main")		{	Remove-Item "harfbuzz-main" -Force -Recurse } 
Remove-Item -Path "SDL*" -Force -Recurse

#download zip copies of all our dependencies
New-Item -Path 'zip' -ItemType Directory
$ProgressPreference = 'SilentlyContinue'    # Subsequent calls do not display UI.
Invoke-WebRequest 'https://github.com/gabime/spdlog/archive/refs/heads/v1.x.zip'  -OutFile "zip\spdlog-1.x.zip"   
Invoke-WebRequest 'https://github.com/libsdl-org/SDL/archive/refs/heads/main.zip'  -OutFile "zip\SDL-main.zip"   
Invoke-WebRequest 'https://github.com/libsdl-org/SDL_ttf/archive/refs/heads/main.zip'  -OutFile "zip\SDL_ttf-main.zip"   
Invoke-WebRequest 'https://github.com/libsdl-org/SDL_net/archive/refs/heads/main.zip'  -OutFile "zip\SDL_net-main.zip"   
Invoke-WebRequest 'https://github.com/libsdl-org/SDL_mixer/archive/refs/heads/main.zip' -OutFile "zip\SDL_mixer-main.zip"   
Invoke-WebRequest 'https://github.com/libsdl-org/SDL_image/archive/refs/heads/main.zip' -OutFile "zip\SDL_image-main.zip"   
Invoke-WebRequest 'https://github.com/harfbuzz/harfbuzz/archive/refs/heads/main.zip'   -OutFile "zip\harfbuzz-main.zip"   
Invoke-WebRequest 'https://github.com/freetype/freetype/archive/refs/heads/master.zip'   -OutFile "zip\freetype-master.zip"   

#unzip all dependencies
$ProgressPreference = 'SilentlyContinue'    # Subsequent calls do not display UI.
Expand-Archive -Path "zip\spdlog-1.x.zip"  -DestinationPath "." -Force
Expand-Archive -Path "zip\SDL-main.zip"     -DestinationPath "." -Force
Expand-Archive -Path "zip\SDL_ttf-main.zip"  -DestinationPath "." -Force
Expand-Archive -Path "zip\SDL_net-main.zip"  -DestinationPath "." -Force
Expand-Archive -Path "zip\SDL_mixer-main.zip"  -DestinationPath "." -Force
Expand-Archive -Path "zip\SDL_image-main.zip"  -DestinationPath "." -Force
Expand-Archive -Path "zip\harfbuzz-main.zip"  -DestinationPath "." -Force
Expand-Archive -Path "zip\freetype-master.zip"  -DestinationPath "." -Force
Remove-Item -Path "zip" -Force -Recurse

#copy the SDL project into the external/include/ folders of SDL_net, SDL_image etc
Rename-Item -Path "SDL-main" -NewName "SDL"
Copy-Item -Path 'SDL\*' -Destination 'SDL_image-main\external\SDL\' -recurse -Force
Copy-Item -Path 'SDL\*' -Destination 'SDL_mixer-main\external\SDL\' -recurse -Force
Copy-Item -Path 'SDL\*' -Destination 'SDL_ttf-main\external\SDL\' -recurse -Force
Copy-Item -Path 'freetype-master\*' -Destination 'SDL_ttf-main\external\freetype\' -recurse -Force
Copy-Item -Path 'harfbuzz-main\*' -Destination 'SDL_ttf-main\external\harfbuzz\' -recurse -Force

