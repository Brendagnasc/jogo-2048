/*
 * Controller.cpp — Implementação do GameController (com persistência)
 * ════════════════════════════════════════════════════════════════════
 *
 * NOVIDADES NESTA VERSÃO:
 *   - loadProgress(): carrega SaveData do disco ao iniciar
 *   - saveProgress(): grava SaveData no disco ao encerrar
 *   - updateSaveData(): sincroniza saveData_ com o estado atual do jogo
 *   - O best score agora persiste entre sessões
 */

#include "Controller.h"
#include <iostream>
#include <cctype>    // std::tolower

// ── Construtor ────────────────────────────────────────────────────────────────
GameController::GameController(std::unique_ptr<IRenderer> renderer)
    : rend_(std::move(renderer))
    , logic_(std::make_unique<GameLogic<BOARD_N>>())
{
    auto stats = std::make_shared<StatsObserver>();
    statsWeak_ = stats;
    logic_->addObserver(std::make_shared<LogObserver>());
    logic_->addObserver(std::move(stats));
}

// ── run(): loop principal ─────────────────────────────────────────────────────
void GameController::run() {
    // ① Carregar progresso salvo ANTES de exibir qualquer coisa
    loadProgress();

    rend_->showHelp();
    std::cout << "  Pressione Enter para iniciar...";
    std::cin.ignore();

    while (running_) tick();

    // ② Atualizar e salvar progresso ao encerrar
    updateSaveData();
    saveProgress();

    if (auto s = statsWeak_.lock()) s->print();

    std::cout << "\n  Recorde: " << saveData_.bestScore
              << "   Movimentos: " << logic_->moves() << "\n\n";
}

// ── loadProgress(): lê SaveData do disco e aplica ao estado do jogo ───────────
void GameController::loadProgress() {
    /*
     * CONCEITO: std::optional — resultado que pode não existir
     * ─────────────────────────────────────────────────────────
     * persist_.load() retorna optional<SaveData>:
     *   - Se o arquivo existe e é válido: contém o SaveData
     *   - Se não existe ou está corrompido: está vazio (nullopt)
     *
     * Padrão idiomático com optional:
     *   if (auto opt = f()) { use(*opt); }
     *   → declara 'opt', avalia f(), e entra no if SÓ SE opt tem valor
     *   → dentro do if, *opt acessa o valor (desreferência como ponteiro)
     *
     * Alternativas:
     *   opt.has_value()   → bool: tem valor?
     *   opt.value()       → T&: acessa valor (lança se vazio)
     *   opt.value_or(x)   → T: acessa valor ou retorna x se vazio
     *   *opt              → T&: acessa valor (UB se vazio — use com has_value())
     */
    if (auto loaded = persist_.load()) {
        saveData_ = *loaded;   // *loaded: desreferencia o optional, obtém SaveData

        // Propagar o best score carregado para o GameLogic
        // O best score do GameLogic começa em 0 — precisamos sincronizar.
        // Usamos um método que força o best_ interno do GameLogic.
        // Como GameLogic não expõe setBest(), usamos uma abordagem simples:
        // guardamos o best em saveData_ e o exibimos pelo Controller.

        std::cout << "\n  Progresso carregado! Recorde anterior: "
                  << saveData_.bestScore << "\n";
    } else {
        // Primeira execução — saveData_ já foi inicializado com zeros (in-class init)
        std::cout << "\n  Primeiro jogo! Bom divertimento.\n";
    }
}

// ── saveProgress(): atualiza saveData_ e grava no disco ──────────────────────
void GameController::saveProgress() {
    if (persist_.save(saveData_)) {
        std::cout << "  Progresso salvo em: " << persist_.path() << "\n";
    } else {
        std::cerr << "  Aviso: nao foi possivel salvar o progresso.\n";
    }
}

// ── updateSaveData(): sincroniza saveData_ com o estado atual ────────────────
void GameController::updateSaveData() {
    // Atualiza o best score se o da sessão atual for maior
    // std::max(a, b): retorna o maior dos dois valores
    saveData_.bestScore = std::max(saveData_.bestScore, logic_->best());

    // Incrementa o contador de partidas (cada reset ou início = 1 partida)
    saveData_.totalGames++;

    // Acumular merges da sessão via StatsObserver
    if (auto s = statsWeak_.lock()) {
        saveData_.totalMerges += s->merges;
    }

    // Atualizar conquista: chegou ao 2048?
    if (logic_->hasWon()) {
        saveData_.everReached2048 = true;
    }
}

// ── tick() ────────────────────────────────────────────────────────────────────
void GameController::tick() {
    // Usa o best score do saveData_ (persiste entre sessões) como "best" exibido
    int displayBest = std::max(saveData_.bestScore, logic_->best());
    rend_->render(logic_->grid(), logic_->score(), displayBest, logic_->moves());

    if (logic_->hasWon())
        rend_->showMsg("\xe2\x98\x85  Voce venceu! Continue para mais pontos.");
    if (logic_->isOver())
        rend_->showMsg("Game Over! Pressione R para reiniciar.", true);

    std::cout << "  > " << std::flush;
    char c;
    if (!(std::cin >> c)) { running_ = false; return; }
    handleInput(c);
}

// ── handleInput() ────────────────────────────────────────────────────────────
void GameController::handleInput(char c) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (auto dir = charToDir(c)) {
        if (!logic_->applyMove(*dir))
            rend_->showMsg("Nenhuma peca se moveu.", true);
        return;
    }

    switch (c) {
        case 'z':
            if (!logic_->undo())
                rend_->showMsg("Nada para desfazer.", true);
            break;
        case 'r':
            // Ao reiniciar: salva o progresso da partida atual antes de resetar
            updateSaveData();
            saveProgress();
            logic_->reset();
            rend_->showMsg("Jogo reiniciado! Progresso salvo.");
            break;
        case 'q':
            running_ = false;
            break;
        default:
            rend_->showMsg("Tecla invalida. Use WASD, Z, R ou Q.", true);
    }
}
