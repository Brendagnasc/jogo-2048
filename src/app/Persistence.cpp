/*
 * Persistence.cpp — Implementação de save/load com <fstream>
 * ═══════════════════════════════════════════════════════════
 *
 * CONCEITO: Por que arquivo de TEXTO e não binário aqui?
 * ───────────────────────────────────────────────────────
 * Arquivo texto:
 *   + Legível e editável num editor comum
 *   + Portável entre sistemas (Windows/Linux/Mac sem conversão)
 *   + Fácil de debugar: abre o arquivo e vê o valor
 *   + Funciona com >> e << (mesma sintaxe que cin/cout)
 *   - Parsing pode ser mais frágil (formatos inesperados)
 *
 * Arquivo binário (write/read com reinterpret_cast):
 *   + Exato: sem problemas de parsing ou formatação
 *   + Compacto: int ocupa exatos 4 bytes (não "1234\n")
 *   - Não legível por humanos
 *   - Depende do endianness e tamanho do int do sistema
 *   - Versões do compilador podem ter padding diferente em structs
 *
 * Para scores simples: texto é a escolha certa.
 * Para saves complexos (mapa de jogo, inventário): binário ou JSON/XML.
 *
 * FORMATO DO NOSSO ARQUIVO:
 * ────────────────────────
 *   2048SAVE          ← header de identificação (FILE_HEADER)
 *   version 1         ← versão do formato
 *   best 42000        ← campo: bestScore
 *   games 17          ← campo: totalGames
 *   merges 384        ← campo: totalMerges
 *   reached2048 1     ← campo: everReached2048 (0 ou 1)
 *
 * Chave + valor separados por espaço: fácil de ler com >> e de expandir.
 */

#include "Persistence.h"

#include <fstream>    // std::ofstream, std::ifstream
#include <iostream>   // std::cerr para mensagens de erro
#include <sstream>    // std::ostringstream
#include <optional>   // std::optional, std::nullopt

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// ── Construtor ────────────────────────────────────────────────────────────────
// std::move(path): transfere a string sem copiar (move semantics)
// O parâmetro 'path' é passado por valor propositalmente: o caller pode passar
// tanto uma lvalue (string existente) quanto uma rvalue (string temporária).
// std::move dentro do construtor transfere para o membro path_.
Persistence::Persistence(std::string path)
    : path_(std::move(path))
{}

// ── save(): grava SaveData no arquivo de texto ────────────────────────────────
bool Persistence::save(const SaveData& data) const {
    /*
     * CONCEITO: std::ofstream — escrita em arquivo
     * ─────────────────────────────────────────────
     * Construtor: std::ofstream out(caminho, modo);
     *
     * Modos mais comuns:
     *   ios::out            → padrão: cria ou sobrescreve
     *   ios::out | ios::app → append: adiciona ao final
     *
     * RAII em ação:
     *   'out' é uma variável local. Quando sair do escopo (return ou exceção),
     *   o destrutor de ofstream chama close() automaticamente.
     *   O arquivo é sempre fechado corretamente — sem risco de corrupção.
     */
    std::ofstream out(path_);   // abre para escrita (ios::out é o padrão)

    /*
     * CONCEITO: verificar se o arquivo abriu com sucesso
     * ──────────────────────────────────────────────────
     * Um ofstream pode falhar ao abrir se:
     *   - O diretório pai não existe
     *   - Não há permissão de escrita
     *   - O caminho é inválido
     *
     * 'if (!out)' é equivalente a 'if (out.fail())'
     * O stream converte para bool via operator bool() herdado de ios.
     */
    if (!out) {
        std::cerr << "[Persistence] Erro: nao foi possivel abrir para escrita: "
                  << path_ << "\n";
        return false;
    }

    /*
     * CONCEITO: escrever com o operador <<
     * ─────────────────────────────────────
     * ofstream herda de ostream, então << funciona igualmente ao cout.
     * Diferença: os bytes vão para o arquivo em vez do terminal.
     *
     * '\n' vs std::endl:
     *   '\n'        → insere apenas o caractere de nova linha (mais rápido)
     *   std::endl   → insere '\n' E chama flush() (esvazia o buffer para disco)
     *
     * O flush do ofstream acontece automaticamente ao fechar (destrutor).
     * Use std::endl apenas quando precisar garantir que os dados foram
     * gravados imediatamente (ex: logging em tempo real).
     */
    out << FILE_HEADER << '\n'
        << "version "     << SaveData::FORMAT_VERSION      << '\n'
        << "best "        << data.bestScore                << '\n'
        << "games "       << data.totalGames               << '\n'
        << "merges "      << data.totalMerges              << '\n'
        << "reached2048 " << (data.everReached2048 ? 1 : 0) << '\n';

    /*
     * CONCEITO: verificar se a escrita foi bem-sucedida
     * ──────────────────────────────────────────────────
     * Após escrever, o stream pode estar em estado de falha se:
     *   - Disco ficou cheio durante a escrita
     *   - Dispositivo foi removido (pendrive, rede)
     *
     * out.good() verifica que nenhum bit de erro está ativo.
     * É boa prática checar APÓS a escrita, não apenas ao abrir.
     */
    if (!out.good()) {
        std::cerr << "[Persistence] Erro durante a escrita em: " << path_ << "\n";
        return false;
    }
    out.close();   // garante os bytes no arquivo antes de sincronizar

#ifdef __EMSCRIPTEN__
    // No navegador o arquivo está num sistema de arquivos em memória; o
    // syncfs copia para o IndexedDB, onde sobrevive a recarregar a página.
    // Fica aqui (e não no controlador) porque é detalhe de onde o save mora.
    EM_ASM( FS.syncfs(false, function (err) { if (err) console.warn("syncfs:", err); }); );
#endif

    return true;
    // Aqui 'out' sai de escopo → destrutor é chamado → arquivo é fechado e
    // o buffer é descarregado para disco. RAII garantindo fechamento limpo.
}

// ── load(): lê SaveData do arquivo de texto ───────────────────────────────────
std::optional<SaveData> Persistence::load() const {
    /*
     * CONCEITO: std::ifstream — leitura de arquivo
     * ─────────────────────────────────────────────
     * Construtor: std::ifstream in(caminho);
     *   → abre para leitura (ios::in é o padrão do ifstream)
     *
     * Se o arquivo não existe: o stream abre em estado de falha.
     * Verificamos com 'if (!in)' antes de tentar ler qualquer coisa.
     */
    std::ifstream in(path_);

    if (!in) {
        // Arquivo não existe — retorna nullopt (não é um erro grave)
        // Na primeira execução do jogo, não há arquivo de save ainda.
        return std::nullopt;
    }

    // Verificar o header de identificação
    // >> lê uma "palavra" (até espaço/newline) para a string 'token'
    std::string token;

    /*
     * CONCEITO: leitura com >>
     * ─────────────────────────
     * in >> token  →  lê a próxima "palavra" (tokens separados por whitespace)
     * Whitespace (espaços, tabs, newlines) é ignorado entre tokens.
     *
     * int n; in >> n  →  lê um inteiro, fazendo a conversão automaticamente
     * Se o próximo token não for um inteiro válido: in entra em failbit.
     *
     * Encadeamento: in >> a >> b  →  lê dois tokens em sequência
     * Se o primeiro falha, o segundo também falha (propagação de erros).
     */
    if (!(in >> token) || token != FILE_HEADER) {
        std::cerr << "[Persistence] Header invalido ou arquivo corrompido: "
                  << path_ << "\n";
        return std::nullopt;
    }

    // Verificar versão do formato
    // Lemos a chave "version" e depois o número
    std::string key;
    int version = 0;
    if (!(in >> key >> version) || key != "version") {
        std::cerr << "[Persistence] Formato de versao invalido.\n";
        return std::nullopt;
    }

    // Versão desconhecida: poderia fazer migração aqui em versões futuras
    if (version != SaveData::FORMAT_VERSION) {
        std::cerr << "[Persistence] Versao incompativel: esperado "
                  << SaveData::FORMAT_VERSION << ", encontrado " << version << "\n";
        return std::nullopt;
    }

    /*
     * CONCEITO: leitura de campos chave-valor com verificação
     * ────────────────────────────────────────────────────────
     * Cada linha tem formato: "chave valor"
     * Lemos a chave (string) e o valor (int) separadamente.
     *
     * Lambda de leitura:
     *   Usamos uma lambda para encapsular a lógica repetida de
     *   "ler chave, verificar chave, ler valor".
     *
     * CONCEITO: Lambda (função anônima, C++11)
     *   [&](params) -> ReturnType { corpo }
     *   [&] = captura TODAS as variáveis do escopo por referência.
     *   Permite usar 'in', 'key', etc. sem passá-los como parâmetros.
     *
     *   Lambda é uma forma compacta de criar um callable (algo chamável)
     *   localmente, sem precisar declarar uma função separada.
     */
    auto readField = [&](const std::string& expectedKey, auto& outVal) -> bool {
        // 'auto& outVal': parâmetro genérico — aceita int& ou bool&
        // Template implícito de lambda (C++20, mas amplamente suportado em C++17)
        if (!(in >> key >> outVal) || key != expectedKey) {
            std::cerr << "[Persistence] Campo ausente ou invalido: '"
                      << expectedKey << "'\n";
            return false;
        }
        return true;
    };

    // Ler todos os campos na ordem em que foram escritos
    SaveData data;
    int reached = 0;  // Lemos como int (0 ou 1) e convertemos para bool

    if (!readField("best",        data.bestScore)    ||
        !readField("games",       data.totalGames)   ||
        !readField("merges",      data.totalMerges)  ||
        !readField("reached2048", reached))
    {
        return std::nullopt;   // Algum campo falhou
    }

    data.everReached2048 = (reached != 0);

    // Retorna o SaveData preenchido dentro de um optional
    // std::optional<SaveData>(data) seria equivalente, mas 'data' é mais limpo
    return data;
    // Aqui 'in' sai de escopo → arquivo é fechado automaticamente (RAII)
}
