/*
 * main_sdl.cpp  —  Ponto de entrada da versão gráfica SDL2
 *
 * BUILD:   make sdl        →  gera ./2048_sdl
 * RODAR:   make run_sdl    →  compila e abre a janela
 *
 * INSTALAR SDL2 (Ubuntu/Debian):
 *   sudo apt install libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core
 */
#include "SDLController.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main() {
    try {
        SDLController ctrl;
        ctrl.run();
    } catch (const std::exception& e) {
        std::cerr << "\nErro ao iniciar o jogo: " << e.what() << "\n\n"
                  << "Certifique-se de que o SDL2 esta instalado:\n"
                  << "  sudo apt install libsdl2-dev libsdl2-ttf-dev\n\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
