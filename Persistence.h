#pragma once
/*
 * Persistence.h — Salvamento e carregamento de dados em arquivo
 * ══════════════════════════════════════════════════════════════
 *
 * CONCEITO CENTRAL: <fstream> — File Stream
 * ─────────────────────────────────────────
 * Em C++, tudo que lida com dados sequenciais é modelado como um "stream"
 * (fluxo). Um stream é uma abstração de "sequência de bytes" que pode vir
 * de qualquer fonte ou ir para qualquer destino.
 *
 * A biblioteca <fstream> oferece três classes:
 *
 *   std::ofstream  (output file stream)
 *     → Abre um arquivo para ESCREVER.
 *     → Se o arquivo não existe, cria. Se existe, sobrescreve por padrão.
 *     → Herda de std::ostream — funciona com o operador <<
 *
 *   std::ifstream  (input file stream)
 *     → Abre um arquivo para LER.
 *     → Se o arquivo não existe, fica em estado de falha (failbit).
 *     → Herda de std::istream — funciona com o operador >>
 *
 *   std::fstream   (file stream bidirecional)
 *     → Abre para ler E escrever ao mesmo tempo.
 *     → Menos comum; útil para editar partes de um arquivo existente.
 *
 * HIERARQUIA DE HERANÇA (para entender como funcionam):
 *
 *   std::ios_base
 *       └── std::ios
 *               ├── std::istream  ← cin é um istream
 *               │       └── std::ifstream
 *               ├── std::ostream  ← cout é um ostream
 *               │       └── std::ofstream
 *               └── std::iostream
 *                       └── std::fstream
 *
 * Isso significa que tudo que funciona com cout funciona com ofstream,
 * e tudo que funciona com cin funciona com ifstream.
 *
 * ─────────────────────────────────────────
 * MODOS DE ABERTURA (flags de ios::openmode)
 * ─────────────────────────────────────────
 * Passados como segundo argumento ao construtor ou a open():
 *
 *   ios::in      → abrir para leitura
 *   ios::out     → abrir para escrita (cria ou sobrescreve)
 *   ios::app     → append: escreve sempre no FIM (não apaga conteúdo)
 *   ios::trunc   → truncate: apaga o conteúdo ao abrir (padrão de out)
 *   ios::binary  → modo binário: não faz conversão de '\n' (importante no Windows)
 *   ios::ate     → at end: posiciona o cursor no final ao abrir
 *
 * Podem ser combinados com | (bitwise OR):
 *   ios::out | ios::app     → escreve ao final sem apagar
 *   ios::in  | ios::binary  → lê em modo binário
 *
 * ─────────────────────────────────────────
 * FORMATOS DE ARQUIVO: TEXTO vs BINÁRIO
 * ─────────────────────────────────────────
 * Texto  → legível por humanos, usa << e >>. Simples mas pode ter
 *           ambiguidade (ex: "12 34" pode ser "1 2 3 4" ou "12 34").
 *
 * Binário → bytes brutos, usa write()/read(). Rápido e sem ambiguidade,
 *            mas não é legível em editores de texto.
 *
 * Para dados simples como high score, texto é a escolha correta:
 * você pode abrir o arquivo num editor para verificar/corrigir.
 *
 * ─────────────────────────────────────────
 * VERIFICAÇÃO DE ERROS EM STREAM
 * ─────────────────────────────────────────
 * Streams têm bits de estado que indicam o que aconteceu:
 *
 *   stream.good()  → tudo ok
 *   stream.eof()   → chegou ao fim do arquivo (end of file)
 *   stream.fail()  → operação falhou (ex: formato errado)
 *   stream.bad()   → erro grave de I/O (ex: disco cheio)
 *
 * A forma idiomática de verificar é usar o stream como bool:
 *   if (stream) { ... }  ←→  if (stream.good()) { ... }
 *
 * ─────────────────────────────────────────
 * RAII E FECHAMENTO AUTOMÁTICO
 * ─────────────────────────────────────────
 * O destrutor de ifstream/ofstream fecha o arquivo automaticamente.
 * Criar o stream como variável local garante fechamento ao sair do escopo.
 * Nunca precisamos chamar close() explicitamente (embora seja válido fazê-lo).
 */

#include <string>
#include <optional>      // std::optional, std::nullopt (C++17)
#include <filesystem>    // std::filesystem::path (C++17) — manipulação de caminhos

// ── SaveData: estrutura de dados persistidos ──────────────────────────────────
// CONCEITO: por que uma struct separada e não salvar diretamente do GameLogic?
//   Separar os dados a persistir do estado completo do jogo tem vantagens:
//   1. Versionamento: podemos adicionar campos sem quebrar saves antigos
//   2. Portabilidade: só persistimos o que o jogador "ganhou", não o estado volátil
//   3. Testabilidade: podemos testar Persistence sem instanciar GameLogic
struct SaveData {
    int  bestScore{0};       // Melhor pontuação de todas as partidas
    int  totalGames{0};      // Total de partidas iniciadas
    int  totalMerges{0};     // Total de fusões feitas na carreira
    bool everReached2048{false}; // Conquista: já chegou ao 2048?

    // Versão do formato do arquivo — permite detectar saves de versões antigas
    // e fazer migração se necessário (técnica comum em jogos)
    static constexpr int FORMAT_VERSION = 1;
};

// ── Persistence: responsável por ler e gravar SaveData em disco ───────────────
// CONCEITO: classe com responsabilidade única (SRP)
//   Esta classe só sabe uma coisa: persistir SaveData num arquivo de texto.
//   Não sabe nada de lógica de jogo, rendering, nem de observers.
class Persistence {
public:
    // ── Construtor: recebe o caminho do arquivo de save ───────────────────
    // std::string path: o nome/caminho do arquivo (ex: "2048_save.dat")
    // 'explicit': evita conversão implícita de string para Persistence
    explicit Persistence(std::string path);

    // ── save(): grava SaveData em disco ───────────────────────────────────
    // CONCEITO: 'const SaveData&'
    //   Recebe por referência constante: sem cópia + garante não modificar data.
    //   Para uma struct pequena como SaveData, a diferença de performance é
    //   mínima, mas é boa prática e documenta a intenção.
    //
    // RETORNO: bool — true se gravou com sucesso, false se houve erro de I/O.
    //   Prefira retornar bool a lançar exceção para erros de arquivo:
    //   não conseguir salvar o high score não é um erro fatal.
    bool save(const SaveData& data) const;

    // ── load(): lê SaveData do disco ──────────────────────────────────────
    // RETORNO: std::optional<SaveData>
    //   Retorna o dado se o arquivo existir e for válido.
    //   Retorna std::nullopt se o arquivo não existir ou estiver corrompido.
    //   Isso é preferível a retornar um SaveData "vazio" com zero em tudo,
    //   pois o chamador pode distinguir "sem arquivo" de "score é 0".
    //
    // CONCEITO: 'const' no método
    //   load() não modifica o objeto Persistence — apenas lê do disco.
    //   Marcar como const é correto e documenta isso.
    std::optional<SaveData> load() const;

    // ── Accessor para o caminho ───────────────────────────────────────────
    const std::string& path() const { return path_; }

private:
    std::string path_;   // Caminho do arquivo de save no sistema de arquivos

    // ── Constante interna: marcador de início do arquivo ──────────────────
    // CONCEITO: 'static constexpr' em classe
    //   'static': existe UMA cópia, compartilhada por todos os objetos.
    //   'constexpr': valor resolvido em compile-time, zero custo em runtime.
    //   Não precisa ser definida num .cpp separado (ao contrário de static não-constexpr).
    static constexpr const char* FILE_HEADER = "2048SAVE";
};

// ── Função auxiliar: defaultSavePath ──────────────────────────────────────────
// Retorna o caminho padrão do arquivo de save.
// Função livre (não membro de classe) porque não precisa de estado.
// 'inline': definida no header sem causar "multiple definition" no linker.
inline std::string defaultSavePath() {
#ifdef __EMSCRIPTEN__
    // No navegador, /save é montado sobre o IndexedDB (IDBFS) em main_sdl.cpp,
    // então o recorde continua salvo entre visitas à página.
    return "/save/2048_save.dat";
#else
    return "2048_save.dat";
#endif
}
