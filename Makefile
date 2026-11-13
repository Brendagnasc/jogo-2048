# =============================================================================
#  Makefile  —  2048 C++  (versão terminal + versão SDL2)
# =============================================================================
#
#  make           →  versão terminal  →  ./2048
#  make sdl       →  versão gráfica   →  ./2048_sdl
#  make run       →  compila + roda (terminal)
#  make run_sdl   →  compila + roda (SDL2)
#  make clean     →  remove binários e .o
#
#  Dependências SDL2:
#    Ubuntu : sudo apt install libsdl2-dev libsdl2-ttf-dev
#    macOS  : brew install sdl2 sdl2_ttf
# =============================================================================

CXX      := g++
STD      := -std=c++17
WARN     := -Wall -Wextra -pedantic
OPT      := -O2
CXXFLAGS := $(STD) $(OPT) $(WARN)
DBGFLAGS := $(STD) -O0 -g -Wall -Wextra -DDEBUG

SDL_CF := $(shell pkg-config --cflags sdl2 SDL2_ttf 2>/dev/null)
SDL_LF := $(shell pkg-config --libs   sdl2 SDL2_ttf 2>/dev/null)

BIN_T  := 2048
SRCS_T := main.cpp Renderer.cpp Controller.cpp Persistence.cpp
OBJS_T := $(SRCS_T:.cpp=.o)

BIN_S  := 2048_sdl
SRCS_S := main_sdl.cpp SDLRenderer.cpp SDLController.cpp Renderer.cpp Persistence.cpp
OBJS_S := $(SRCS_S:.cpp=.o)

HDRS   := Types.h Observer.h GameLogic.h Renderer.h Controller.h Persistence.h

.PHONY: all sdl run run_sdl debug clean

all: $(BIN_T)

$(BIN_T): $(OBJS_T)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "  ✓  ./$(BIN_T)"

$(BIN_S): $(OBJS_S)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SDL_LF)
	@echo "  ✓  ./$(BIN_S)"

sdl: $(BIN_S)

SDLRenderer.o: SDLRenderer.cpp SDLRenderer.h $(HDRS)
	$(CXX) $(CXXFLAGS) $(SDL_CF) -c $< -o $@
	@echo "  ·  $<"

SDLController.o: SDLController.cpp SDLController.h SDLRenderer.h $(HDRS)
	$(CXX) $(CXXFLAGS) $(SDL_CF) -c $< -o $@
	@echo "  ·  $<"

main_sdl.o: main_sdl.cpp SDLController.h
	$(CXX) $(CXXFLAGS) $(SDL_CF) -c $< -o $@
	@echo "  ·  $<"

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "  ·  $<"

run: $(BIN_T)
	./$(BIN_T)

run_sdl: $(BIN_S)
	./$(BIN_S)

debug: CXXFLAGS := $(DBGFLAGS)
debug: $(BIN_T)

clean:
	rm -f $(OBJS_T) $(OBJS_S) $(BIN_T) $(BIN_S)
	@echo "  ✓  limpo"
