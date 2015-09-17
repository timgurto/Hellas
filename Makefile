#OBJS specifies which files to compile as part of the project
OBJS = $(wildcard mmo/*.cpp)

#CC specifies which compiler we're using
CC = g++ 

#INCLUDE_PATHS specifies the additional include paths we'll need
INCLUDE_PATHS = -ISDL\SDL2-2.0.3\include\SDL2 \
		-ISDL\SDL2_image-2.0.0\include \
		-ISDL\SDL2_ttf-2.0.12\include \
		-ISDL\SDL2_mixer-2.0.0\include

#LIBRARY_PATHS specifies the additional library paths we'll need
LIBRARY_PATHS = -LSDL\SDL2-2.0.3\lib \
		-LSDL\SDL2_image-2.0.0\lib\x86 \
		-LSDL\SDL2_ttf-2.0.12\lib\x86 \
		-LSDL\SDL2_mixer-2.0.0\lib\x86

#COMPILER_FLAGS specifies the additional compilation options we're using 
# -w suppresses all warnings 
#  # -Wl,-subsystem,windows gets rid of the console window
COMPILER_FLAGS = -Wall -Werror -O3 -Wl,-subsystem,windows -std=c++11 

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = mmo 

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
