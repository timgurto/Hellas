SOURCES := $(wildcard mmo/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)

CC := g++

INCLUDE_PATHS = -ILib\SDL2-2.0.3\include\SDL2 \
		-ILib\SDL2_image-2.0.0\include \
		-ILib\SDL2_ttf-2.0.12\include \
		-ILib\SDL2_mixer-2.0.0\include

LIBRARY_PATHS = -LLib\SDL2-2.0.3\lib \
		-LLib\SDL2_image-2.0.0\lib\x86 \
		-LLib\SDL2_ttf-2.0.12\lib\x86 \
		-LLib\SDL2_mixer-2.0.0\lib\x86

# -Wl,-subsystem,windows gets rid of the console window
CXXFLAGS = -Wl,-subsystem,windows -std=c++11 $(INCLUDE_PATHS)

LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -lws2_32

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
