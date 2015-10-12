SOURCES_COMMON := $(wildcard src/*.cpp) $(wildcard tinyxml/*.cpp)
SOURCES_SERVER := $(wildcard src/server/*.cpp) src/client/Texture.cpp src/client/Renderer.cpp
SOURCES_CLIENT := $(wildcard src/client/*.cpp) $(wildcard src/client/ui/*.cpp) src/server/Item.cpp src/server/Object.cpp src/server/ObjectType.cpp src/server/Yield.cpp

OBJECTS_COMMON := $(SOURCES_COMMON:.cpp=.o)
OBJECTS_SERVER := $(SOURCES_SERVER:.cpp=.o)
OBJECTS_CLIENT := $(SOURCES_CLIENT:.cpp=.o)

CC := g++

INCLUDE_PATHS_COMMON = -Itinyxml \
	-ISDL2\SDL2-2.0.3\include\SDL2 \
	-ISDL2\SDL2_image-2.0.0\include \
	-ISDL2\SDL2_ttf-2.0.12\include
INCLUDE_PATHS_SERVER = $(INCLUDE_PATHS_COMMON)
INCLUDE_PATHS_CLIENT = $(INCLUDE_PATHS_COMMON) \
	-ISDL2\SDL2_mixer-2.0.0\include

LIBRARY_PATHS_COMMON = -LSDL2\SDL2-2.0.3\lib\x86 \
	-LSDL2\SDL2_image-2.0.0\lib\x86 \
	-LSDL2\SDL2_ttf-2.0.12\lib\x86
LIBRARY_PATHS_SERVER = $(LIBRARY_PATHS_COMMON)
LIBRARY_PATHS_CLIENT = $(LIBRARY_PATHS_COMMON) \
	-LSDL2\SDL2_mixer-2.0.0\lib\x86

# -Wl,-subsystem,windows gets rid of the console window
CXXFLAGS_COMMON = -Wall -Wpedantic -std=c++11 -DSINGLE_THREAD
CXXFLAGS_SERVER = $(CXXFLAGS_COMMON) -DSINGLE_THREAD
CXXFLAGS_CLIENT = $(CXXFLAGS_COMMON) -Wl,-subsystem,windows

LDFLAGS_COMMON = -lmingw32 -lws2_32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
LDFLAGS_SERVER = $(LDFLAGS_COMMON)
LDFLAGS_CLIENT = $(LDFLAGS_COMMON) -lSDL2_mixer 

PROGRAM_NAME_SERVER = mmo-server.exe
PROGRAM_NAME_CLIENT = mmo-client.exe

all : server client

server : $(OBJECTS_COMMON) $(OBJECTS_SERVER)
	@echo Linking server
	@$(CC) $(LIBRARY_PATHS_SERVER) $(OBJECTS_COMMON) $(OBJECTS_SERVER) $(LDFLAGS_SERVER) -o $(PROGRAM_NAME_SERVER)

client : $(OBJECTS_COMMON) $(OBJECTS_CLIENT)
	@echo Linking client
	@$(CC) $(LIBRARY_PATHS_CLIENT) $(OBJECTS_COMMON) $(OBJECTS_CLIENT) $(LDFLAGS_CLIENT) -o $(PROGRAM_NAME_CLIENT)

clean:
	rm -f $(PROGRAM_NAME_SERVER) $(PROGRAM_NAME_CLIENT)
	rm -f src/*.o src/server/*.o src/client/*.o src/client/ui/*.o

$(OBJECTS_COMMON) : %.o : %.cpp
	@echo Compiling $<
	@$(CC) $(CXXFLAGS_COMMON) $(INCLUDE_PATHS_COMMON) -c -o $@ $<

$(OBJECTS_SERVER) : %.o : %.cpp
	@echo Compiling $<
	@$(CC) $(CXXFLAGS_SERVER) $(INCLUDE_PATHS_SERVER) -c -o $@ $<

$(OBJECTS_CLIENT) : %.o : %.cpp
	@echo Compiling $<
	@$(CC) $(CXXFLAGS_CLIENT) $(INCLUDE_PATHS_CLIENT) -c -o $@ $<

