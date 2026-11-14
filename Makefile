# =============================================================================
#  Makefile  —  2048 C++  (terminal, SDL2, WebAssembly e testes)
# =============================================================================
#
#  make           →  versão terminal  →  ./2048
#  make sdl       →  versão gráfica   →  ./2048_sdl
#  make test      →  compila e roda os testes da lógica (core + app)
#  make web       →  versão WebAssembly → web/2048.js + web/2048.wasm
#  make run       →  compila + roda (terminal)
#  make run_sdl   →  compila + roda (SDL2)
#  make clean     →  remove build/ e binários
#
#  Dependências SDL2:
#    Ubuntu : sudo apt install libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core
#    macOS  : brew install sdl2 sdl2_ttf
#
#  Organização em camadas (src/):
#    core/          regras puras do jogo (sem I/O)        ← testado em tests/
#    app/           GameSession, comandos, save, observers
#    ui/terminal/   versão de terminal
#    ui/sdl/        versão SDL2 (desktop e web)
# =============================================================================

CXX      := g++
STD      := -std=c++17
WARN     := -Wall -Wextra -pedantic
OPT      := -O2
CXXFLAGS := $(STD) $(OPT) $(WARN)
DBGFLAGS := $(STD) -O0 -g $(WARN) -DDEBUG

SDL_CF := $(shell pkg-config --cflags sdl2 SDL2_ttf 2>/dev/null)
SDL_LF := $(shell pkg-config --libs   sdl2 SDL2_ttf 2>/dev/null)

BUILD := build

CORE_HDRS := $(wildcard src/core/*.h)
APP_HDRS  := $(wildcard src/app/*.h)
APP_SRCS  := src/app/GameSession.cpp src/app/Persistence.cpp

BIN_T  := 2048
SRCS_T := $(APP_SRCS) $(wildcard src/ui/terminal/*.cpp)

BIN_S  := 2048_sdl
SRCS_S := $(APP_SRCS) $(wildcard src/ui/sdl/*.cpp)

BIN_TEST  := $(BUILD)/tests
SRCS_TEST := $(APP_SRCS) tests/test_core.cpp

obj = $(patsubst %.cpp,$(BUILD)/%.o,$(1))

.PHONY: all sdl test run run_sdl debug clean web

all: $(BIN_T)

$(BIN_T): $(call obj,$(SRCS_T))
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "  ✓  ./$(BIN_T)"

sdl: $(BIN_S)

$(BIN_S): $(call obj,$(SRCS_S))
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SDL_LF)
	@echo "  ✓  ./$(BIN_S)"

test: $(BIN_TEST)
	./$(BIN_TEST)

$(BIN_TEST): $(call obj,$(SRCS_TEST))
	$(CXX) $(CXXFLAGS) -o $@ $^

# Objetos da UI SDL precisam dos cabeçalhos do SDL
$(BUILD)/src/ui/sdl/%.o: src/ui/sdl/%.cpp $(CORE_HDRS) $(APP_HDRS) $(wildcard src/ui/sdl/*.h)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(SDL_CF) -c $< -o $@

$(BUILD)/%.o: %.cpp $(CORE_HDRS) $(APP_HDRS) $(wildcard src/ui/terminal/*.h)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(BIN_T)
	./$(BIN_T)

run_sdl: $(BIN_S)
	./$(BIN_S)

debug: CXXFLAGS := $(DBGFLAGS)
debug: $(BIN_T)

clean:
	rm -rf $(BUILD) $(BIN_T) $(BIN_S) web/2048.js web/2048.wasm
	@echo "  ✓  limpo"

# ── Versão web (WebAssembly) ─────────────────────────────────────────────────
#  O mesmo código SDL2 roda no navegador; SDL2 e SDL2_ttf vêm do Emscripten.
#  As fontes DejaVu são embutidas no mesmo caminho do Linux, então
#  SDLRenderer.cpp as encontra sem mudança. FONT_DIR pode ser trocado
#  para compilar fora do Ubuntu.
EMXX      := em++
FONT_DIR  ?= /usr/share/fonts/truetype/dejavu
FONT_VDIR := /usr/share/fonts/truetype/dejavu
FONTS     := DejaVuSansMono-Bold.ttf DejaVuSans-Bold.ttf DejaVuSans.ttf
WEB_FLAGS := $(STD) -O2 -fexceptions -sUSE_SDL=2 -sUSE_SDL_TTF=2 -lidbfs.js \
             -sALLOW_MEMORY_GROWTH -sENVIRONMENT=web \
             -sEXPORTED_FUNCTIONS=_main,_web_iniciar,_web_tecla \
             -sEXPORTED_RUNTIME_METHODS=ccall \
             $(foreach f,$(FONTS),--embed-file $(FONT_DIR)/$(f)@$(FONT_VDIR)/$(f))

web: web/2048.js

web/2048.js: $(SRCS_S) $(CORE_HDRS) $(APP_HDRS) $(wildcard src/ui/sdl/*.h)
	$(EMXX) $(WEB_FLAGS) $(SRCS_S) -o $@
	@echo "  ✓  web/2048.js  (sirva a pasta web/ por um servidor local)"
