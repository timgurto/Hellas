SOURCES := $(wildcard mmo/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)

CC := g++

INCLUDE_PATHS = -ISDL\SDL2-2.0.3\include\SDL2 \
		-ISDL\SDL2_image-2.0.0\include \
		-ISDL\SDL2_ttf-2.0.12\include \
		-ISDL\SDL2_mixer-2.0.0\include

LIBRARY_PATHS = -LSDL\SDL2-2.0.3\lib \
		-LSDL\SDL2_image-2.0.0\lib\x86 \
		-LSDL\SDL2_ttf-2.0.12\lib\x86 \
		-LSDL\SDL2_mixer-2.0.0\lib\x86

# -Wl,-subsystem,windows gets rid of the console window
CXXFLAGS = -Wl,-subsystem,windows -std=c++11 $(INCLUDE_PATHS)

LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image

PROGRAM_NAME = mmo.exe

all : $(OBJECTS)
	$(CC) $(LIBRARY_PATHS) $(OBJECTS) $(LDFLAGS) -o $(PROGRAM_NAME)
