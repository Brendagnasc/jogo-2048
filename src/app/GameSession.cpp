/*
 * GameSession.cpp — Implementação dos casos de uso do jogo
 */
#include "GameSession.h"

#include <algorithm>

GameSession::GameSession() : GameSession(Options{}) {}

GameSession::GameSession(Options opts)
    : game_(opts.seed)
    , persist_(std::move(opts.savePath))
{
    game_.addObserver(stats_);
    if (opts.logEvents) game_.addObserver(log_);
}

std::string GameSession::loadProgress() {
    if (auto loaded = persist_.load()) {
        save_ = *loaded;
        return "Recorde anterior: " + std::to_string(save_.bestScore);
    }
    return "Primeiro jogo! Bom divertimento.";
}

GameSession::Outcome GameSession::handle(Command cmd) {
    Outcome out;

    if (auto dir = directionOf(cmd)) {
        out.turn = game_.move(*dir);
        if (!out.turn) {
            out.message = "Nenhuma peca se moveu.";
            out.isError = true;
        } else {
            if (out.turn->justWon) save_.everReached2048 = true;
            saveBestIfImproved();
        }
        return out;
    }

    switch (cmd) {
        case Command::Undo:
            if (!game_.undo()) {
                out.message = "Nada para desfazer.";
                out.isError = true;
            }
            break;

        case Command::Restart:
            out.finishedGame = GameSummary{ game_.score(), game_.moves(), game_.hasWon() };
            recordFinishedGame();
            persist_.save(save_);
            game_.reset();
            out.message = "Reiniciado! Progresso salvo.";
            break;

        case Command::Quit:
            out.quit = true;
            break;

        default:
            break;
    }
    return out;
}

void GameSession::finish() {
    recordFinishedGame();
    persist_.save(save_);
}

GameView GameSession::view() const {
    GameView v;
    v.grid  = &game_.grid();
    v.score = game_.score();
    v.best  = best();
    v.moves = game_.moves();
    v.won   = game_.hasWon();
    v.over  = game_.isOver();
    return v;
}

int GameSession::best() const {
    return std::max(save_.bestScore, game_.score());
}

void GameSession::recordFinishedGame() {
    save_.bestScore = best();
    save_.totalGames++;
    save_.totalMerges += stats_.game().merges;
    if (game_.hasWon()) save_.everReached2048 = true;
    stats_.startNewGame();   // zera o contador da partida: não soma duas vezes
}

// Grava assim que o recorde sobe. No navegador a página pode ser fechada a
// qualquer momento sem passar por finish(); no desktop protege contra queda.
void GameSession::saveBestIfImproved() {
    if (game_.score() > save_.bestScore) {
        save_.bestScore = game_.score();
        persist_.save(save_);
    }
}
