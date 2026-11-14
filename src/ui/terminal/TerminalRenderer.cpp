/*
 * Renderer.cpp — Implementação do TerminalRenderer
 * ──────────────────────────────────────────────────
 * CONCEITO: Arquivo .cpp de implementação
 *   Aqui ficam os corpos das funções declaradas no .h correspondente.
 *   Ao compilar, o compilador gera um "objeto" (Renderer.o) que será
 *   linkado com os outros objetos para formar o executável final.
 *
 *   Vantagem: se você mudar APENAS este arquivo, só ele precisa ser
 *   recompilado. Em projetos grandes, isso economiza MUITO tempo.
 *
 * CONCEITO: 'final' em classe
 *   TerminalRenderer não deve ser herdado — 'final' documenta e impõe isso.
 *   O compilador pode otimizar chamadas virtuais de classes 'final'
 *   porque sabe que não há subclasse que possa sobrescrever.
 *
 * CONCEITO: Namespace anônimo (unnamed namespace)
 *   'namespace { ... }' torna os símbolos visíveis APENAS neste .cpp.
 *   É o equivalente C++ de 'static' em funções C — evita colisões de nomes
 *   entre diferentes .cpp quando o linker junta os objetos.
 */

#include "TerminalApp.h"   // declara createTerminalRenderer()
#include "../../app/Renderer.h"

#include <iostream>
#include <iomanip>   // std::setw — largura mínima de campo na saída
#include <string>
#include <sstream>
#include <cstdlib>   // system()

// ── Namespace anônimo: helpers visíveis apenas neste .cpp ────────────────────
namespace {

// Limpa o terminal de forma portável
void cls() {
#ifdef _WIN32
    system("cls");
#else
    // Escape ANSI: move cursor para (0,0) e apaga a tela — mais suave que system("clear")
    std::cout << "\033[H\033[2J" << std::flush;
#endif
}

// Retorna a cor de fundo ANSI 256-colors para cada valor de tile
// '\033[48;5;Nm' = cor de fundo número N na paleta de 256 cores
std::string bgFor(int v) {
    switch (v) {
        case    0: return "\033[48;5;236m";  // Cinza escuro
        case    2: return "\033[48;5;255m";  // Branco quase puro
        case    4: return "\033[48;5;223m";  // Bege
        case    8: return "\033[48;5;209m";  // Laranja claro
        case   16: return "\033[48;5;202m";  // Laranja
        case   32: return "\033[48;5;196m";  // Vermelho
        case   64: return "\033[48;5;166m";  // Laranja escuro
        case  128: return "\033[48;5;227m";  // Amarelo dourado
        case  256: return "\033[48;5;226m";  // Amarelo
        case  512: return "\033[48;5;148m";  // Lima
        case 1024: return "\033[48;5;118m";  // Verde claro
        case 2048: return "\033[48;5;82m";   // Verde (vitória)
        default:   return "\033[48;5;57m";   // Roxo (> 2048)
    }
}

// Cor do texto: escuro para tiles claros, branco para tiles escuros
std::string fgFor(int v) {
    return (v <= 4) ? "\033[30m" : "\033[97m";
}

// Constantes de formatação
constexpr int CW = 8;   // Largura de cada célula (cell width)
const std::string RST  = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string DIM  = "\033[2m";
const std::string RED  = "\033[31m";
const std::string GRN  = "\033[32m";

// Imprime uma linha horizontal da borda/separador da grade
// Usa strings UTF-8 para os caracteres box-drawing (3 bytes cada)
void hline(const char* l, const char* m, const char* r, const char* fill, int n) {
    std::cout << "  " << l;
    for (int c = 0; c < n; c++) {
        std::cout << [&]{ std::string s; for(int i=0;i<CW;i++) s+=fill; return s; }();
        if (c < n-1) std::cout << m;
    }
    std::cout << r << "\n";
}

// Imprime uma linha de padding (acima ou abaixo do número) numa célula
void padRow(const Grid<BOARD_N>& g, int r) {
    std::cout << "  \xe2\x94\x82";  // UTF-8: caractere │
    for (int c = 0; c < BOARD_N; c++) {
        int v = g[r][c];
        std::cout << bgFor(v) << fgFor(v) << std::string(CW, ' ') << RST << "\xe2\x94\x82";
    }
    std::cout << "\n";
}

// Imprime a linha central de uma célula com o número centralizado
void numRow(const Grid<BOARD_N>& g, int r) {
    std::cout << "  \xe2\x94\x82";
    for (int c = 0; c < BOARD_N; c++) {
        int v = g[r][c];
        // Tile vazio: exibe "·" (U+00B7, UTF-8: 0xC2 0xB7) — mais elegante que " "
        std::string lbl = v ? std::to_string(v) : "\xc2\xb7";
        // Centralização: distribuir espaços uniformemente em CW colunas
        int lp = (CW - static_cast<int>(lbl.size())) / 2;
        int rp = CW - static_cast<int>(lbl.size()) - lp;
        std::cout << bgFor(v) << fgFor(v)
                  << std::string(lp, ' ')
                  << BOLD << lbl << RST
                  << bgFor(v) << fgFor(v)
                  << std::string(rp, ' ')
                  << RST << "\xe2\x94\x82";
    }
    std::cout << "\n";
}

} // fim do namespace anônimo

// ── TerminalRenderer: implementação concreta de IRenderer ────────────────────
// Esta classe herda de IRenderer publicamente e implementa todos os métodos
// puramente virtuais. 'final' impede que seja herdada novamente.
class TerminalRenderer final : public IRenderer {
public:
    void render(const GameView& view) override {
        const Grid<BOARD_N>& g = *view.grid;
        const int score = view.score, best = view.best, moves = view.moves;
        cls();
        // Cabeçalho
        std::cout << "\n  " << BOLD << "2048" << RST
                  << "  " << DIM << "C++ Edition" << RST << "\n\n"
                  << "  Score: " << BOLD << std::setw(6) << score << RST
                  << "   Best: " << BOLD << std::setw(6) << best  << RST
                  << "   " << DIM << "Moves: " << moves << RST << "\n\n";

        // Grade: borda superior
        hline("\xe2\x94\x8c", "\xe2\x94\xac", "\xe2\x94\x90", "\xe2\x94\x80", BOARD_N);

        for (int r = 0; r < BOARD_N; r++) {
            padRow(g, r);   // Espaço acima do número
            numRow(g, r);   // O número em si
            padRow(g, r);   // Espaço abaixo do número
            if (r < BOARD_N-1)
                hline("\xe2\x94\x9c", "\xe2\x94\xbc", "\xe2\x94\xa4", "\xe2\x94\x80", BOARD_N);
        }

        // Borda inferior
        hline("\xe2\x94\x94", "\xe2\x94\xb4", "\xe2\x94\x98", "\xe2\x94\x80", BOARD_N);

        std::cout << "\n  " << DIM
                  << "WASD: mover  \xe2\x94\x82  Z: undo  \xe2\x94\x82  R: restart  \xe2\x94\x82  Q: quit"
                  << RST << "\n\n";
    }

    void showMsg(const std::string& msg, bool err) override {
        std::cout << "\n  " << (err ? RED : GRN) << BOLD << msg << RST << "\n";
    }

    void showHelp() override {
        std::cout << "\n"
            << "  \xe2\x94\x8c\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x90\n"
            << "  \xe2\x94\x82      2048  \xe2\x80\x94  C++ Edition      \xe2\x94\x82\n"
            << "  \xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\xa4\n"
            << "  \xe2\x94\x82  W / A / S / D  \xe2\x86\x92  Mover          \xe2\x94\x82\n"
            << "  \xe2\x94\x82  Z              \xe2\x86\x92  Desfazer (Undo)\xe2\x94\x82\n"
            << "  \xe2\x94\x82  R              \xe2\x86\x92  Reiniciar      \xe2\x94\x82\n"
            << "  \xe2\x94\x82  Q              \xe2\x86\x92  Sair           \xe2\x94\x82\n"
            << "  \xe2\x94\x94\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x98\n\n";
    }
};

// ── Factory function: createTerminalRenderer ──────────────────────────────────
// CONCEITO: Factory Function
//   Retorna um unique_ptr<IRenderer> apontando para um TerminalRenderer.
//   O chamador (main.cpp / GameController) só conhece a interface IRenderer —
//   não precisa de '#include "Renderer.cpp"' nem saber que existe TerminalRenderer.
//   Isso é "programming to an interface, not an implementation".
//
// Por que unique_ptr?
//   - Ownership exclusiva: ao sair do escopo, o objeto é destruído automaticamente
//   - Sem 'delete' manual → sem vazamento de memória
//   - make_unique<T>(): forma segura de criar unique_ptr (sem 'new' explícito)
#include <memory>  // std::make_unique
std::unique_ptr<IRenderer> createTerminalRenderer() {
    return std::make_unique<TerminalRenderer>();
}
