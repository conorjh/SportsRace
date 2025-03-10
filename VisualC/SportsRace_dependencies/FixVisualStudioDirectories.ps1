
#change the output directory of the SDL solution to point to our dependencies solution output folder
(Get-Content "SdL\VisualC\SDL\SDL.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)$(Platform)\$(Configuration)\</OutDir>', '$(SolutionDir)..\..\$(Platform)\$(Configuration)\</OutDir>')} | 
Set-Content "SDL_image\VisualC\SDL_image.vcxproj"

#replace references to the SolutionDir with ProjectDir in SDL_image... because we made our own solution and 
#included SDL_image project in it, the original SolutionDir references in dependencies can be incorrect
(Get-Content "SDL_image\VisualC\SDL_image.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\external\SDL\include')} | 
Set-Content "SDL_image\VisualC\SDL_image.vcxproj"

#same for mixer
(Get-Content "SDL_mixer\VisualC\SDL_mixer.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\include;$(SolutionDir)..\external\SDL\include;', '$(ProjectDir)..\include;$(ProjectDir)..\external\SDL\include;')} |
Set-Content "SDL_mixer\VisualC\SDL_mixer.vcxproj"

#and sdl_mixers dependencies - native_midi
(Get-Content "SDL_mixer\VisualC\native_midi\native_midi.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\..\external\SDL\include')} | 
Set-Content "SDL_mixer\VisualC\native_midi\native_midi.vcxproj"

#and sdl_mixers dependencies - timidity
(Get-Content "SDL_mixer\VisualC\timidity\timidity.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\..\external\SDL\include')} | 
Set-Content "SDL_mixer\VisualC\timidity\timidity.vcxproj" 

#and sdl_mixers dependencies - playmus
(Get-Content "SDL_mixer\VisualC\playmus\playmus.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\..\external\SDL\include')} | 
Foreach-Object {$_.replace('$(SolutionDir)..\include', '$(ProjectDir)..\include')} | 
Set-Content "SDL_mixer\VisualC\playmus\playmus.vcxproj"

#and sdl_mixers dependencies - playwave
(Get-Content "SDL_mixer\VisualC\playwave\playwave.vcxproj") | 
Foreach-Object {$_.replace('$(SolutionDir)..\external\SDL\include', '$(ProjectDir)..\..\external\SDL\include')} | 
Foreach-Object {$_.replace('$(SolutionDir)..\include', '$(ProjectDir)..\include')} | 
Set-Content "SDL_mixer\VisualC\playwave\playwave.vcxproj"

#same for SDL_ttf
(Get-Content "SDL_ttf\VisualC\SDL_ttf.vcxproj") | 
Foreach-Object {$_.replace('..\include;..\external\freetype\include;..\external\harfbuzz\src;$(SolutionDir)..\..\SDL\include;', '$(ProjectDir)..\include;$(ProjectDir)..\external\freetype\include;$(ProjectDir)..\external\harfbuzz\src;$(ProjectDir)..\..\SDL\include;')} | 
Foreach-Object {$_.replace('$(SolutionDir)..\..\SDL\VisualC\$(PlatformName)\$(Configuration);', '$(SolutionDir)$(PlatformName)\$(Configuration);')} | 
Set-Content "SDL_ttf\VisualC\SDL_ttf.vcxproj"

#same for SDL_net
(Get-Content "SDL_net\VisualC\SDL_net.vcxproj") | 
Foreach-Object {$_.replace('external\freetype\include;external\harfbuzz\src;$(SolutionDir)SDL\include', '$(ProjectDir)external\freetype\include;$(ProjectDir)external\harfbuzz\src;$(ProjectDir)SDL\include')} | 
Foreach-Object {$_.replace('$(ProjectDir)..\..\SDL\VisualC\$(PlatformName)\$(Configuration);', '$(SolutionDir)$(PlatformName)\$(Configuration);')} | 
Set-Content "SDL_net\VisualC\SDL_net.vcxproj"
