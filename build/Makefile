SRC_DIR = ../src
CC =  gcc
PYTHON3 = python
CXFREEZE =${PYTHON3} c:/Python33/Scripts/cxfreeze --base-name=Win32GUI

CFLAGS_OPTIONS = -Wall -g -O0
LDFLAGS_OPTIONS =

GLEW_CFLAGS =
FIAL_CFLAGS =
SDL_IMAGE_CFLAGS =

GLEW_LDFLAGS =
FIAL_LDFLAGS =
SDL_IMAGE_LDFLAGS=

SDL_CFLAGS  =     `bash sdl2-config --cflags`
FREETYPE_CFLAGS = `bash freetype-config --cflags`
SDL_LIBS =        `bash sdl2-config --libs`
FREETYPE_LIBS =   `bash freetype-config --libs`

CFLAGS =   ${CFLAG_OPTIONS} $(GLEW_CFLAGS) ${FIAL_CFLAGS} ${SDL_CFLAGS} ${FREETYPE_CFLAGS} ${SDL_IMAGE_CFLAGS}
LDFLAGS  =  ${LDFLAGS_OPTIONS} ${GLEW_LDFLAGS} ${FIAL_LDFLAGS} ${SDL_IMAGE_LDFLAGS}
LDLIBS     = -lglew32 -lglu32 -lopengl32 ${SDL_LIBS} ${FREETYPE_LIBS} -lfial -lSDL2_image

OBJECTS = main.o bcve.o texture.o button.o shader.o fi.o text.o
EXECUTABLE = bcve.exe
.PHONY: all package tags

all: ${EXECUTABLE}

%.o: ${SRC_DIR}/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXECUTABLE): $(OBJECTS)
	$(CXX)  -o $@  $(LDFLAGS) $(OBJECTS) ${LDLIBS}

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)

PKG_ROOT="./package"
PKG_BIN=${PKG_ROOT}
PKG_FONT=${PKG_BIN}
PKG_DOC=${PKG_ROOT}/doc
PKG_SHADERS=${PKG_ROOT}/shaders
PKG_SCRIPTS=${PKG_ROOT}/scripts

LAUNCHER_EXEC=${PKG_BIN}/launcher.exe
${LAUNCHER_EXEC}: ${SRC_DIR}/launcher.py
	${CXFREEZE} ${SRC_DIR}/launcher.py --target-dir=${PKG_BIN}

package:
	mkdir ${PKG_ROOT}
	mkdir ${PKG_DOC}
	mkdir ${PKG_SHADERS}
	mkdir ${PKG_SCRIPTS}
	cp ${EXECUTABLE} ${PKG_BIN}
	cp 7za.exe ${PKG_BIN}
	cp 7z.exe ${PKG_BIN}
	cp *.dll ${PKG_BIN}
	cp *.ttf ${PKG_FONT}
	cp ../shaders/* ${PKG_SHADERS}
	cp ../scripts/* ${PKG_SCRIPTS}
	${CXFREEZE} ../src/launcher.py --target-dir=${PKG_BIN}
