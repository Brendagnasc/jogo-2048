/*
 * main.cpp  —  Ponto de entrada da versão gráfica SDL2 (desktop e web)
 *
 * BUILD:   make sdl        →  gera ./2048_sdl
 * RODAR:   make run_sdl    →  compila e abre a janela
 * WEB:     make web        →  gera web/2048.js + web/2048.wasm (Emscripten)
 *
 * INSTALAR SDL2 (Ubuntu/Debian):
 *   sudo apt install libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core
 */
#include "SDLApp.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>

// No navegador o app precisa sobreviver ao fim de main(): o loop é
// dirigido pelo requestAnimationFrame, não por um while bloqueante.
static SDLApp* g_app = nullptr;

static void quadro(void*) {
    if (g_app && !g_app->step()) emscripten_cancel_main_loop();
}

extern "C" {

// Chamado pelo JavaScript depois que o IndexedDB foi carregado em /save.
EMSCRIPTEN_KEEPALIVE void web_iniciar() {
    try {
        GameSession::Options opts;
        opts.logEvents = false;   // sem log no console do navegador
        g_app = new SDLApp(std::move(opts));
        g_app->start();
        emscripten_set_main_loop_arg(quadro, nullptr, 0, false);
    } catch (const std::exception& e) {
        std::cerr << "Erro ao iniciar o jogo: " << e.what() << "\n";
    }
}

// Botões da página (para celular): mesmas teclas do teclado (w a s d z r).
EMSCRIPTEN_KEEPALIVE void web_tecla(char c) {
    if (!g_app) return;
    auto cmd = commandFromKey(c);
    if (cmd && *cmd != Command::Quit) g_app->handle(*cmd);   // na web não há "sair"
}

}

int main() {
    // Monta o IndexedDB em /save (para o recorde persistir) e só então inicia.
    EM_ASM(
        FS.mkdir('/save');
        FS.mount(IDBFS, {}, '/save');
        FS.syncfs(true, function (err) {
            if (err) console.warn("syncfs:", err);
            Module.ccall('web_iniciar', null, [], []);
        });
    );
    return 0;
}

#else

int main() {
    try {
        SDLApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "\nErro ao iniciar o jogo: " << e.what() << "\n\n"
                  << "Certifique-se de que o SDL2 esta instalado:\n"
                  << "  sudo apt install libsdl2-dev libsdl2-ttf-dev\n\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#endif
