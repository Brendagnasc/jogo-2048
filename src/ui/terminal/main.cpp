/*
 * main.cpp — Ponto de entrada da versão de terminal
 * ──────────────────────────────────────────────────
 * main() só monta os objetos de mais alto nível e inicia o loop.
 */
#include "TerminalApp.h"

#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
    try {
        TerminalApp app(createTerminalRenderer());
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "\n  Erro fatal: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
