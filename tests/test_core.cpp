/*
 * test_core.cpp — Testes das regras do jogo (core) e da sessão (app)
 * ───────────────────────────────────────────────────────────────────
 * Sem framework externo: um CHECK simples que conta falhas, para rodar em
 * qualquer lugar com 'make test' (e no GitHub Actions a cada push).
 *
 * Só é possível testar assim porque a lógica não faz I/O e o gerador
 * aleatório recebe uma semente: o resultado de cada partida é previsível.
 */
#include "../src/app/GameSession.h"
#include "../src/core/Board.h"
#include "../src/core/Game.h"

#include <cstdio>
#include <iostream>
#include <string>

namespace {

int g_checks = 0;
int g_fails  = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            ++g_fails;                                                       \
            std::cerr << "  FALHOU " << __FILE__ << ":" << __LINE__          \
                      << "  " #cond "\n";                                    \
        }                                                                    \
    } while (0)

using G = Grid<4>;

G row(std::array<int, 4> r0) { G g{}; g[0] = r0; return g; }

// ── board::slide ──────────────────────────────────────────────────────────────

void testSlideBasico() {
    auto r = board::slide<4>(row({ 2, 2, 0, 0 }), Direction::Left);
    CHECK((r.grid[0] == std::array<int, 4>{ 4, 0, 0, 0 }));
    CHECK(r.gained == 4);
    CHECK(r.merges == 1);
    CHECK(r.moved);
}

void testFundeUmaVezPorMovimento() {
    // a peça resultante de uma fusão não se funde de novo no mesmo movimento
    auto a = board::slide<4>(row({ 2, 2, 2, 2 }), Direction::Left);
    CHECK((a.grid[0] == std::array<int, 4>{ 4, 4, 0, 0 }));
    CHECK(a.gained == 8);

    auto b = board::slide<4>(row({ 2, 2, 4, 0 }), Direction::Left);
    CHECK((b.grid[0] == std::array<int, 4>{ 4, 4, 0, 0 }));
    CHECK(b.gained == 4);
}

void testFusaoComecaPelaBordaDeDestino() {
    auto l = board::slide<4>(row({ 2, 2, 2, 0 }), Direction::Left);
    CHECK((l.grid[0] == std::array<int, 4>{ 4, 2, 0, 0 }));
    auto r = board::slide<4>(row({ 2, 2, 2, 0 }), Direction::Right);
    CHECK((r.grid[0] == std::array<int, 4>{ 0, 0, 2, 4 }));
}

void testColunas() {
    G g{};
    g[0][1] = 2; g[1][1] = 2; g[3][1] = 4;
    auto up = board::slide<4>(g, Direction::Up);
    CHECK(up.grid[0][1] == 4 && up.grid[1][1] == 4 && up.grid[2][1] == 0);
    auto down = board::slide<4>(g, Direction::Down);
    CHECK(down.grid[3][1] == 4 && down.grid[2][1] == 4 && down.grid[0][1] == 0);
}

void testSemMovimento() {
    auto r = board::slide<4>(row({ 2, 4, 8, 16 }), Direction::Left);
    CHECK(!r.moved);
    CHECK(r.tiles.empty());
    CHECK(r.gained == 0);
}

void testCaminhoDasPecas() {
    // [0,2,0,2] ← : a peça de (0,1) vai para (0,0); a de (0,3) se funde em (0,0)
    auto r = board::slide<4>(row({ 0, 2, 0, 2 }), Direction::Left);
    CHECK(r.tiles.size() == 2);
    CHECK((r.tiles[0].from == Cell{ 0, 1 }) && (r.tiles[0].to == Cell{ 0, 0 }) && !r.tiles[0].merged);
    CHECK((r.tiles[1].from == Cell{ 0, 3 }) && (r.tiles[1].to == Cell{ 0, 0 }) && r.tiles[1].merged);
}

void testCanMove() {
    G cheioSemJogada{{ { 2, 4, 2, 4 }, { 4, 2, 4, 2 }, { 2, 4, 2, 4 }, { 4, 2, 4, 2 } }};
    CHECK(!board::canMove<4>(cheioSemJogada));
    G cheioComJogada = cheioSemJogada;
    cheioComJogada[3][3] = 4;   // fica igual ao vizinho de cima
    CHECK(board::canMove<4>(cheioComJogada));
}

// ── Game<N> ───────────────────────────────────────────────────────────────────

void testSementeReproduzivel() {
    Game<4> a(42), b(42);
    CHECK(a.grid() == b.grid());
    a.move(Direction::Left); b.move(Direction::Left);
    CHECK(a.grid() == b.grid());
}

void testNovaPartidaTemDuasPecas() {
    Game<4> g(7);
    int pecas = 0;
    for (const auto& r : g.grid()) for (int v : r) if (v) ++pecas;
    CHECK(pecas == 2);
}

void testMovimentoInvalidoNaoMuda() {
    Game<4> g(1);
    g.setState(row({ 2, 4, 8, 16 }));
    CHECK(!g.move(Direction::Left).has_value());
    CHECK(g.moves() == 0);
    CHECK(!g.canUndo());
}

void testMovimentoPontuaENasceUmaPeca() {
    Game<4> g(3);
    g.setState(row({ 2, 2, 0, 0 }));
    auto turn = g.move(Direction::Left);
    CHECK(turn.has_value());
    CHECK(g.score() == 4);
    CHECK(g.moves() == 1);
    CHECK(turn->spawn.has_value());
    const Spawn s = *turn->spawn;
    CHECK(g.grid()[s.at.row][s.at.col] == s.value);
    CHECK(s.value == 2 || s.value == 4);
}

void testUndoRestauraTudo() {
    Game<4> g(5);
    G inicio = row({ 1024, 1024, 0, 0 });
    g.setState(inicio, 100);
    auto turn = g.move(Direction::Left);
    CHECK(turn && turn->justWon);
    CHECK(g.hasWon());
    CHECK(g.undo());
    CHECK(g.grid() == inicio);
    CHECK(g.score() == 100);
    CHECK(!g.hasWon());   // antes o undo mantinha a vitória
    CHECK(!g.undo());
}

void testFimDeJogo() {
    // depois de ← a única casa vazia é (3,3), cercada por 8 e 16:
    // nasça um 2 ou um 4, não sobra jogada
    G g{{ { 2, 4, 2, 4 }, { 4, 2, 4, 2 }, { 2, 4, 2, 8 }, { 4, 2, 0, 16 } }};
    Game<4> game(9);
    game.setState(g);
    auto turn = game.move(Direction::Left);
    CHECK(turn && turn->gameOver);
    CHECK(game.isOver());
    CHECK(!game.move(Direction::Right).has_value());
}

// ── GameSession (app) ─────────────────────────────────────────────────────────

void testEstatisticasNaoDuplicamAoReiniciar() {
    const std::string path = "test_save.dat";
    std::remove(path.c_str());

    int fusoesDaPrimeira = 0;
    int recorde = 0;
    {
        GameSession::Options opts;
        opts.savePath  = path;
        opts.logEvents = false;
        opts.seed      = 11;
        GameSession s(opts);
        s.loadProgress();

        // joga até acontecer pelo menos uma fusão
        const Command ciclo[] = { Command::MoveLeft, Command::MoveDown,
                                  Command::MoveRight, Command::MoveUp };
        for (int i = 0; i < 200 && s.stats().game().merges == 0; i++)
            s.handle(ciclo[i % 4]);
        fusoesDaPrimeira = s.stats().game().merges;
        recorde = s.view().best;
        CHECK(fusoesDaPrimeira > 0);

        auto out = s.handle(Command::Restart);
        CHECK(out.finishedGame.has_value());
        CHECK(s.stats().game().merges == 0);   // partida nova começa do zero
        s.handle(Command::Restart);
        s.finish();

        // 3 partidas (2 reinícios + a final); as fusões contadas uma vez só.
        // Antes: o save somava as fusões da sessão inteira a cada reinício.
        CHECK(s.saveData().totalGames == 3);
        CHECK(s.saveData().totalMerges == fusoesDaPrimeira);
    }
    {
        GameSession::Options opts;
        opts.savePath  = path;
        opts.logEvents = false;
        GameSession s(opts);
        s.loadProgress();
        CHECK(s.saveData().bestScore == recorde);
        CHECK(s.saveData().totalMerges == fusoesDaPrimeira);
    }
    std::remove(path.c_str());
}

void testComandosInvalidosDaoMensagem() {
    GameSession::Options opts;
    opts.savePath  = "test_save2.dat";
    opts.logEvents = false;
    opts.seed      = 2;
    GameSession s(opts);
    auto out = s.handle(Command::Undo);
    CHECK(out.isError && !out.message.empty());
    auto quit = s.handle(Command::Quit);
    CHECK(quit.quit);
    std::remove("test_save2.dat");
}

} // namespace

int main() {
    testSlideBasico();
    testFundeUmaVezPorMovimento();
    testFusaoComecaPelaBordaDeDestino();
    testColunas();
    testSemMovimento();
    testCaminhoDasPecas();
    testCanMove();
    testSementeReproduzivel();
    testNovaPartidaTemDuasPecas();
    testMovimentoInvalidoNaoMuda();
    testMovimentoPontuaENasceUmaPeca();
    testUndoRestauraTudo();
    testFimDeJogo();
    testEstatisticasNaoDuplicamAoReiniciar();
    testComandosInvalidosDaoMensagem();

    std::cout << (g_fails ? "\n  " : "  ") << g_checks - g_fails << "/" << g_checks
              << " verificações passaram\n";
    return g_fails ? 1 : 0;
}
