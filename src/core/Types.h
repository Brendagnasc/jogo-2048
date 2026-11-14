#pragma once
/*
 * Types.h — Tipos básicos do domínio (camada core)
 * ─────────────────────────────────────────────────
 * A camada core é o "coração" do jogo: regras puras, sem entrada/saída,
 * sem SDL e sem arquivos. Tudo aqui pode ser testado isoladamente.
 *
 * Dependências permitidas:  core  ←  app  ←  ui
 *   core não inclui nada de app/ nem de ui/.
 */

#include <array>
#include <vector>

// ── Tamanho do tabuleiro ──────────────────────────────────────────────────────
// 'constexpr': resolvido em compile-time. Trocar BOARD_N para 5 gera um jogo 5×5.
inline constexpr int BOARD_N  = 4;
inline constexpr int WIN_TILE = 2048;

// ── Grid<N>: matriz N×N de valores (0 = vazio) ────────────────────────────────
template <int N>
using Grid = std::array<std::array<int, N>, N>;

// ── Direction: enum class (scoped enum) ───────────────────────────────────────
enum class Direction { Up, Down, Left, Right };

// ── Cell: coordenada de uma célula ────────────────────────────────────────────
struct Cell {
    int row{0};
    int col{0};
    bool operator==(const Cell& o) const { return row == o.row && col == o.col; }
};

// ── TileMove: o que aconteceu com UMA peça num movimento ──────────────────────
// A lógica registra o caminho exato de cada peça. Assim a interface anima o
// que realmente aconteceu, em vez de tentar adivinhar comparando grades.
struct TileMove {
    Cell from;
    Cell to;
    int  value{0};        // valor da peça ANTES de mover
    bool merged{false};   // true se esta peça se fundiu ao chegar em 'to'
};

// ── MoveResult<N>: resultado completo de deslizar o tabuleiro ─────────────────
template <int N>
struct MoveResult {
    Grid<N>               grid{};      // tabuleiro depois de deslizar (sem a peça nova)
    int                   gained{0};   // pontos ganhos com fusões
    int                   merges{0};   // quantas fusões aconteceram
    bool                  moved{false};
    std::vector<TileMove> tiles;       // peças que mudaram de lugar ou se fundiram
};

// ── Spawn: peça nova que surgiu depois do movimento ───────────────────────────
struct Spawn {
    Cell at;
    int  value{0};
};
