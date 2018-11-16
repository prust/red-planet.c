redplanetmake:
ifeq ($(OS),Windows_NT)
	gcc -o red-planet.exe red-planet.c -I /c/msys64/usr/lib/sdl2/x86_64-w64-mingw32/include/SDL2 -L /c/msys64/usr/lib/sdl2/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2
else
	gcc -o red-planet red-planet.c -L/usr/local/lib -I/Library/Frameworks/SDL2.framework/Headers -I/Library/Frameworks/SDL2_image.framework/Headers -I/Library/Frameworks/SDL2_mixer.framework/Headers -F/Library/Frameworks -framework SDL2 -framework SDL2_image -framework SDL2_mixer
endif

redplanetdebug:
	gcc -g -o red-planet red-planet.c -L/usr/local/lib -I/Library/Frameworks/SDL2.framework/Headers -I/Library/Frameworks/SDL2_image.framework/Headers -I/Library/Frameworks/SDL2_mixer.framework/Headers -F/Library/Frameworks -framework SDL2 -framework SDL2_image -framework SDL2_mixer