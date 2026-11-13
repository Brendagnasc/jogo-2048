#pragma once
/*
 * Types.h — Tipos compartilhados por todo o projeto
 * ─────────────────────────────────────────────────
 * CONCEITO: '#pragma once'
 *   Substitui o guard clássico (#ifndef / #define / #endif).
 *   Garante que o compilador inclua este header APENAS UMA VEZ
 *   por unidade de compilação, evitando redefinição de símbolos.
 *
 * CONCEITO: 'header (.h) vs implementação (.cpp)'
 *   .h  → declarações: "existe algo chamado X com essa assinatura"
 *   .cpp → definições:  "e aqui está o corpo/implementação de X"
 *   Separar assim permite recompilar apenas os .cpp que mudaram.
 *
 * CONCEITO: Por que colocar tipos num header próprio?
 *   Direction, Grid<N> e GameEvent são usados por GameLogic, Observer
 *   e Controller. Se cada um definisse seu próprio, haveria conflito.
 *   Um header central elimina duplicação (princípio DRY).
 */

#include <array>
#include <optional>   // std::optional — valor que pode ou não existir (C++17)
#include <string>

// ── Tamanho do tabuleiro ──────────────────────────────────────────────────────
// 'template<int N>' + 'constexpr': o valor é resolvido em compile-time.
// Trocar BOARD_N para 5 gera um jogo 5×5 sem alterar mais nada.
inline constexpr int BOARD_N  = 4;
inline constexpr int WIN_TILE = 2048;

// ── Grid<N>: alias de tipo template ──────────────────────────────────────────
// 'using' cria um alias. Grid<4> = array<array<int,4>, 4> = matriz 4×4.
// std::array (ao contrário de int[][]) conhece seu tamanho, é copiável
// e tem iteradores — pode ser usado em algoritmos da STL.
template <int N>
using Grid = std::array<std::array<int, N>, N>;

// ── Direction: enum class (scoped enum, C++11) ────────────────────────────────
// 'enum class' NÃO polui o namespace externo nem converte para int.
//   Errado (enum antigo):  if (dir == 0) ...  ← compila mas é frágil
//   Correto (enum class):  if (dir == Direction::Left) ...  ← explícito
enum class Direction { Up, Down, Left, Right, None };

// ── charToDir: retorna std::optional<Direction> ───────────────────────────────
// optional<T>: ou tem um valor T, ou está "vazio" (nullopt).
// Evita valores sentinela (-1, nullptr, etc.) para indicar "inválido".
// 'inline': permite definição no header sem causar "multiple definition".
inline std::optional<Direction> charToDir(char c) {
    switch (std::tolower(static_cast<unsigned char>(c))) {
        case 'w': return Direction::Up;
        case 's': return Direction::Down;
        case 'a': return Direction::Left;
        case 'd': return Direction::Right;
        default:  return std::nullopt;
    }
}

// ── GameEvent: evento emitido pela lógica do jogo ─────────────────────────────
// Usado pelo padrão Observer: a lógica emite eventos; os observers reagem.
// Struct (vs class) por convenção: dados sem comportamento, tudo público.
struct GameEvent {
    // Enum class aninhado: Type é um tipo próprio dentro de GameEvent.
    // Acesso: GameEvent::Type::Merge
    enum class Type { Move, Merge, Win, GameOver, NewTile, Undo };

    Type type;
    int  value{0};   // Inicialização in-class (C++11): valor padrão = 0
    int  score{0};
};

// ── Snapshot<N>: estado completo do jogo para o sistema de Undo ───────────────
// Padrão Command: cada movimento salva um snapshot antes de executar.
// A pilha (stack) de snapshots implementa desfazer (undo) ilimitado.
template <int N>
struct Snapshot {
    Grid<N> grid;
    int     score{0};
    int     moves{0};
};
