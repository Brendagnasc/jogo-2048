#pragma once
/*
 * Board.h — Regras do tabuleiro como funções puras (camada core)
 * ───────────────────────────────────────────────────────────────
 * Funções puras: a saída depende só da entrada, sem estado escondido e sem
 * aleatoriedade. São as regras mais importantes do jogo e as mais fáceis de
 * testar (veja tests/test_core.cpp).
 *
 * ESTRATÉGIA: "linhas de varredura"
 *   Para qualquer direção, o tabuleiro é visto como N linhas. Cada linha é
 *   uma lista de coordenadas que começa na borda para onde as peças vão.
 *     Left  → linha r: (r,0) (r,1) ... (r,N-1)
 *     Right → linha r: (r,N-1) ... (r,0)
 *     Up    → coluna c: (0,c) (1,c) ... (N-1,c)
 *     Down  → coluna c: (N-1,c) ... (0,c)
 *   O mesmo algoritmo de "deslizar para o início" vale para as 4 direções,
 *   e como trabalhamos com coordenadas reais, sabemos de onde veio cada peça.
 */

#include "Types.h"

namespace board {

// Coordenada da posição 'i' da linha 'line', percorrendo na direção 'dir'
template <int N>
constexpr Cell cellAt(Direction dir, int line, int i) {
    switch (dir) {
        case Direction::Left:  return { line, i };
        case Direction::Right: return { line, N - 1 - i };
        case Direction::Up:    return { i, line };
        case Direction::Down:  return { N - 1 - i, line };
    }
    return { line, i };
}

// Desliza o tabuleiro inteiro na direção 'dir'.
// Regra do 2048: cada peça se funde no máximo uma vez por movimento, e a
// fusão acontece a partir da borda de destino ([2,2,2,0] ← vira [4,2,0,0]).
template <int N>
MoveResult<N> slide(const Grid<N>& src, Direction dir) {
    MoveResult<N> res;
    // começa vazio; cada peça é escrita no seu destino
    for (auto& row : res.grid) row.fill(0);

    for (int line = 0; line < N; line++) {
        int  dest       = 0;      // próxima posição livre na linha
        bool destMerged = false;  // a peça em dest-1 já resultou de fusão?

        for (int i = 0; i < N; i++) {
            const Cell from = cellAt<N>(dir, line, i);
            const int  v    = src[from.row][from.col];
            if (v == 0) continue;

            // tenta fundir com a última peça colocada nesta linha
            if (dest > 0 && !destMerged) {
                const Cell prev = cellAt<N>(dir, line, dest - 1);
                if (res.grid[prev.row][prev.col] == v) {
                    res.grid[prev.row][prev.col] = v * 2;
                    res.gained += v * 2;
                    res.merges++;
                    destMerged = true;
                    res.tiles.push_back({ from, prev, v, true });
                    continue;
                }
            }

            const Cell to = cellAt<N>(dir, line, dest);
            res.grid[to.row][to.col] = v;
            destMerged = false;
            if (!(to == from)) res.tiles.push_back({ from, to, v, false });
            dest++;
        }
    }

    res.moved = !res.tiles.empty();
    return res;
}

// Existe alguma célula vazia?
template <int N>
bool hasEmptyCell(const Grid<N>& g) {
    for (const auto& row : g)
        for (int v : row)
            if (v == 0) return true;
    return false;
}

// Ainda existe alguma jogada possível? (célula vazia ou vizinhos iguais)
template <int N>
bool canMove(const Grid<N>& g) {
    if (hasEmptyCell<N>(g)) return true;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++) {
            if (c + 1 < N && g[r][c] == g[r][c + 1]) return true;
            if (r + 1 < N && g[r][c] == g[r + 1][c]) return true;
        }
    return false;
}

// Alguma peça atingiu (ou passou de) 'target'?
template <int N>
bool hasTile(const Grid<N>& g, int target) {
    for (const auto& row : g)
        for (int v : row)
            if (v >= target) return true;
    return false;
}

// Lista das células vazias, em ordem de leitura
template <int N>
std::vector<Cell> emptyCells(const Grid<N>& g) {
    std::vector<Cell> out;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            if (g[r][c] == 0) out.push_back({ r, c });
    return out;
}

} // namespace board
