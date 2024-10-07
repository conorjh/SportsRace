#clear old download folder, old unzipped repos
Remove-Item -Path "..\zip\" -Force -Recurse
Remove-Item -Path "..\SDL*" -Force -Recurse
Remove-Item -Path "..\freetype-master" -Force -Recurse
Remove-Item -Path "..\harfbuzz-main" -Force -Recurse

#download zip copies of all our dependencies
New-Item -Path '..\zip' -ItemType Directory
$ProgressPreference = 'SilentlyContinue'    # Subsequent calls do not display UI.
Invoke-WebRequest 'https://github.com/gabime/spdlog/archive/refs/heads/v1.x.zip'  -OutFile "..\zip\spdlog-1.x.zip"   
Invoke-WebRequest 'https://github.com/libsdl-org/SDL/archive/refs/heads/main.zip'  -OutFile "..\zip\SDL-main.zip"   
Invoke-WebRequest 'https://github.com/libsdl-org/SDL_ttf/archive/refs/heads/main.zip'  -OutFile "..\zip\SDL_ttf-main.zip"   
Invoke-WebRequest 'https://github.com/libsdl-org/SDL_net/archive/refs/heads/main.zip'  -OutFile "..\zip\SDL_net-main.zip"   
Invoke-WebRequest 'https://github.com/libsdl-org/SDL_mixer/archive/refs/heads/main.zip' -OutFile "..\zip\SDL_mixer-main.zip"   
Invoke-WebRequest 'https://github.com/libsdl-org/SDL_image/archive/refs/heads/main.zip' -OutFile "..\zip\SDL_image-main.zip"   
Invoke-WebRequest 'https://github.com/harfbuzz/harfbuzz/archive/refs/heads/main.zip'   -OutFile "..\zip\harfbuzz-main.zip"   
Invoke-WebRequest 'https://github.com/freetype/freetype/archive/refs/heads/master.zip'   -OutFile "..\zip\freetype-master.zip"   

#unzip all dependencies
$ProgressPreference = 'SilentlyContinue'    # Subsequent calls do not display UI.
Expand-Archive -Path "..\zip\spdlog-1.x.zip"  -DestinationPath "..\." -Force
Expand-Archive -Path "..\zip\SDL-main.zip"     -DestinationPath "..\." -Force
Expand-Archive -Path "..\zip\SDL_ttf-main.zip"  -DestinationPath "..\." -Force
Expand-Archive -Path "..\zip\SDL_net-main.zip"  -DestinationPath "..\." -Force
Expand-Archive -Path "..\zip\SDL_mixer-main.zip"  -DestinationPath "..\." -Force
Expand-Archive -Path "..\zip\SDL_image-main.zip"  -DestinationPath "..\." -Force
Expand-Archive -Path "..\zip\harfbuzz-main.zip"  -DestinationPath "..\." -Force
Expand-Archive -Path "..\zip\freetype-master.zip"  -DestinationPath "..\." -Force

#replace references to the SolutionDir with ProjectDir in SDL_image... because we made our own solution and 
#included SDL_image project in it, the original SolutionDir references in dependencies can be incorrect
(Get-Content "..\SDL_image-main\VisualC\SDL_image.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\external\SDL\include')} | 
Set-Content "..\SDL_image-main\VisualC\SDL_image.vcxproj"

#same for mixer
(Get-Content "..\SDL_mixer-main\VisualC\SDL_mixer.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\external\SDL\include')} | 
Foreach-Object {$_.replace('$(SolutionDir)..\..\SDL\VisualC\$(PlatformName)\$(Configuration);', '$(SolutionDir)$(PlatformName)\$(Configuration);')} | 
Set-Content "..\SDL_mixer-main\VisualC\SDL_mixer.vcxproj"

#and sdl_mixers dependencies - native_midi
(Get-Content "..\SDL_mixer-main\VisualC\native_midi\native_midi.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\..\external\SDL\include')} | 
Set-Content "..\SDL_mixer-main\VisualC\native_midi\native_midi.vcxproj"

#and sdl_mixers dependencies - timidity
(Get-Content "..\SDL_mixer-main\VisualC\timidity\timidity.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\..\external\SDL\include')} | 
Set-Content "..\SDL_mixer-main\VisualC\timidity\timidity.vcxproj"

#and sdl_mixers dependencies - playmus
(Get-Content "..\SDL_mixer-main\VisualC\playmus\playmus.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\..\external\SDL\include')} | 
Foreach-Object {$_.replace('$(SolutionDir)..\include', '$(ProjectDir)..\include')} | 
Set-Content "..\SDL_mixer-main\VisualC\playmus\playmus.vcxproj"

#and sdl_mixers dependencies - playwave
(Get-Content "..\SDL_mixer-main\VisualC\playwave\playwave.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\..\external\SDL\include')} | 
Foreach-Object {$_.replace('$(SolutionDir)..\include', '$(ProjectDir)..\include')} | 
Set-Content "..\SDL_mixer-main\VisualC\playwave\playwave.vcxproj"

#same for SDL_ttf
(Get-Content "..\SDL_ttf-main\VisualC\SDL_ttf.vcxproj") | 
Foreach-Object {$_.replace('..\external\freetype\include;..\external\harfbuzz\src;$(SolutionDir)..\..\SDL\include', '$(ProjectDir)..\external\freetype\include;$(ProjectDir)..\external\harfbuzz\src;$(ProjectDir)..\..\SDL\include')} | 
Foreach-Object {$_.replace('$(SolutionDir)..\..\SDL\VisualC\$(PlatformName)\$(Configuration);', '$(SolutionDir)$(PlatformName)\$(Configuration);')} | 
Set-Content "..\SDL_ttf-main\VisualC\SDL_ttf.vcxproj"

#same for SDL_net
(Get-Content "..\SDL_net-main\VisualC\SDL_net.vcxproj") | 
Foreach-Object {$_.replace('..\external\freetype\include;..\external\harfbuzz\src;$(SolutionDir)..\..\SDL\include', '$(ProjectDir)..\external\freetype\include;$(ProjectDir)..\external\harfbuzz\src;$(ProjectDir)..\..\SDL\include')} | 
Foreach-Object {$_.replace('$(ProjectDir)..\..\SDL\VisualC\$(PlatformName)\$(Configuration);', '$(SolutionDir)$(PlatformName)\$(Configuration);')} | 
Set-Content "..\SDL_net-main\VisualC\SDL_net.vcxproj"

#copy the SDL project into the external/include/ folders of SDL_net, SDL_image etc
New-Item -Path '..\SDL' -ItemType Directory
Copy-Item -Path '..\SDL-main\*' -Destination '..\SDL\' -recurse -Force
Copy-Item -Path '..\SDL-main\*' -Destination '..\SDL_image-main\external\SDL\' -recurse -Force
Copy-Item -Path '..\SDL-main\*' -Destination '..\SDL_mixer-main\external\SDL\' -recurse -Force
Copy-Item -Path '..\SDL-main\*' -Destination '..\SDL_ttf-main\external\SDL\' -recurse -Force
Copy-Item -Path '..\freetype-master\*' -Destination '..\SDL_ttf-main\external\freetype\' -recurse -Force
Copy-Item -Path '..\harfbuzz-main\*' -Destination '..\SDL_ttf-main\external\harfbuzz\' -recurse -Force

