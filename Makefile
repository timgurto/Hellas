SOURCES_SERVER := $(wildcard src/*.cpp) $(wildcard src/server/*.cpp) $(wildcard tinyxml/*.cpp)
SOURCES_CLIENT := $(wildcard src/*.cpp) $(wildcard src/client/*.cpp) $(wildcard tinyxml/*.cpp)
OBJECTS_SERVER := $(SOURCES_CLIENT:.cpp=.o)
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
	@$(CC) $(LIBRARY_PATHS) $(OBJECTS_SERVER) $(LD_FLAGS) -o $(PROGRAM_NAME_SERVER)

client : $(OBJECTS_CLIENT)
	@echo Linking client
	@$(CC) $(LIBRARY_PATHS) $(OBJECTS_CLIENT) $(LD_FLAGS) -o $(PROGRAM_NAME_CLIENT)

clean:
	rm -f $(PROGRAM_NAME_SERVER) $(PROGRAM_NAME_CLIENT)
	rm -f mmo/*.o

%.o: %.cpp
	@echo Compiling $<
	@$(CC) $(CXXFLAGS) $(INCLUDE_PATHS) -c -o $@ $<
