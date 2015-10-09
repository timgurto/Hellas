SOURCES := $(wildcard mmo/*.cpp) $(wildcard tinyxml/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)

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
CXXFLAGS = -Wall -Wpedantic -Wl,-subsystem,windows -std=c++11 $(INCLUDE_PATHS)

LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer -lws2_32

PROGRAM_NAME = mmo.exe

all : $(OBJECTS)
	@echo Linking
	@$(CC) $(LIBRARY_PATHS) $(OBJECTS) $(LDFLAGS) -o $(PROGRAM_NAME)

clean:
	rm -f $(PROGRAM_NAME)
	rm -f mmo/*.o

%.o: %.cpp
	@echo Compiling $<
	@$(CC) $(CXXFLAGS) $(INCLUDE_PATHS) -c -o $@ $<
