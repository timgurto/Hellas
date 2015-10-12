SOURCES_COMMON := $(wildcard src/*.cpp) $(wildcard tinyxml/*.cpp)
SOURCES_SERVER := $(SOURCES_COMMON) $(wildcard src/server/*.cpp) src/client/Log.cpp src/client/Renderer.cpp src/client/Texture.cpp
SOURCES_CLIENT := $(SOURCES_COMMON) $(wildcard src/client/*.cpp) $(wildcard src/client/ui/*.cpp) src/server/User.cpp src/server/Item.cpp src/server/Server.cpp src/server/Object.cpp src/server/ObjectType.cpp src/server/Yield.cpp
OBJECTS_SERVER := $(SOURCES_SERVER:.cpp=.o)
OBJECTS_CLIENT := $(SOURCES_CLIENT:.cpp=.o)

CC := g++

INCLUDE_PATHS = -ISDL2\SDL2-2.0.3\include\SDL2 \
		-ISDL2\SDL2_image-2.0.0\include \
		-ISDL2\SDL2_ttf-2.0.12\include \
		-ISDL2\SDL2_mixer-2.0.0\include \
		-Itinyxml

LIBRARY_PATHS = -LSDL2\SDL2-2.0.3\lib\x86 \
		-LSDL2\SDL2_image-2.0.0\lib\x86 \
		-LSDL2\SDL2_ttf-2.0.12\lib\x86 \
		-LSDL2\SDL2_mixer-2.0.0\lib\x86

# -Wl,-subsystem,windows gets rid of the console window
CXXFLAGS = -Wall -Wpedantic -Wl,-subsystem,windows -std=c++11 $(INCLUDE_PATHS) -DSINGLE_THREAD

LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer -lws2_32

PROGRAM_NAME_SERVER = mmo-server.exe
PROGRAM_NAME_CLIENT = mmo-client.exe

all : server client

server : $(OBJECTS_SERVER)
	@echo Linking server
	@$(CC) $(LIBRARY_PATHS) $(OBJECTS_SERVER) $(LDFLAGS) -o $(PROGRAM_NAME_SERVER)

client : $(OBJECTS_CLIENT)
	@echo Linking client
	@$(CC) $(LIBRARY_PATHS) $(OBJECTS_CLIENT) $(LDFLAGS) -o $(PROGRAM_NAME_CLIENT)

clean:
	rm -f $(PROGRAM_NAME_SERVER) $(PROGRAM_NAME_CLIENT)
	rm -f src/*.o src/server/*.o src/client/*.o src/client/ui/*.o

%.o: %.cpp
	@echo Compiling $<
	@$(CC) $(CXXFLAGS) $(INCLUDE_PATHS) -c -o $@ $<
