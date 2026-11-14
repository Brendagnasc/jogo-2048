# 2048 em C++

O jogo 2048 em C++17, com duas interfaces sobre a mesma lógica: uma versão de
terminal e uma versão gráfica em SDL2 com animações.

**Jogue no navegador:** https://brendagnasc.github.io/jogo-2048/
(faz parte do [menu de jogos](https://brendagnasc.github.io/jogos/))

## Funcionalidades

- Animações de surgimento, fusão e deslizamento das peças
- Desfazer jogada (`Z`) e reiniciar (`R`)
- Histórico das últimas partidas e recorde salvo em arquivo
- Controle por teclado (WASD ou setas); na versão web, também deslizando o dedo ou o mouse

## Arquitetura

O código é dividido em três camadas. Cada uma só depende das que estão abaixo:

```
ui/terminal ──┐
              ├──► app (GameSession) ──► core (regras puras)
ui/sdl ───────┘          │
                         └──► Persistence (arquivo / IndexedDB)
```

| Camada | Arquivos | Responsabilidade |
|---|---|---|
| `src/core` | `Board.h`, `Game.h`, `Types.h`, `Observer.h` | Regras do jogo, sem I/O. `board::slide` é uma função pura que devolve o caminho de cada peça; `Game<N>` guarda o estado, o undo e um gerador aleatório com semente |
| `src/app` | `GameSession`, `Command.h`, `Observers.h`, `Persistence`, `Renderer.h` | Casos de uso compartilhados pelas duas interfaces: o que cada comando faz, quando salvar, estatísticas |
| `src/ui/terminal` | `TerminalApp`, `TerminalRenderer` | Lê teclas, traduz para `Command` e desenha no terminal |
| `src/ui/sdl` | `SDLApp`, `SDLRenderer`, `main.cpp` | Janela SDL2 (desktop e web) com animações |

Padrões usados: **Observer** (estatísticas e log reagem aos eventos do jogo),
**Strategy** (`IRenderer`), **Command/Memento** (pilha de undo) e injeção de
dependência (semente do gerador, caminho do save, renderer).

### Testes

`tests/test_core.cpp` cobre as regras (fusão uma vez por movimento, ordem das
fusões, caminho das peças, fim de jogo, vitória e undo) e a sessão (save e
estatísticas). Como o gerador aceita semente, as partidas são reproduzíveis.

```bash
make test
```

O GitHub Actions roda os testes e compila as versões desktop a cada push; a
demo só é publicada se tudo passar.

## Compilar

```bash
# dependências (Ubuntu/Debian)
sudo apt install g++ make libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core

make          # versão terminal  → ./2048
make sdl      # versão gráfica   → ./2048_sdl
make test     # testes da lógica
```

## Versão web (WebAssembly)

O mesmo código SDL2 é compilado para WebAssembly com [Emscripten](https://emscripten.org/)
(que já traz SDL2 e SDL2_ttf). As diferenças ficam isoladas em `#ifdef __EMSCRIPTEN__`:

- o loop `while` vira um quadro por `requestAnimationFrame` (`SDLApp::step`)
- o recorde é salvo no IndexedDB do navegador (IDBFS montado em `/save`)
- gesto de deslizar e botões na página para jogar no celular

```bash
make web                       # requer em++ no PATH
python3 -m http.server -d web  # abra http://localhost:8000
```

A cada push na `main`, o GitHub Actions compila e publica a demo no GitHub Pages
(`.github/workflows/pages.yml`).

## Autora

Brenda Gabrielle Alves Nascimento — Ciência da Computação, UFOP
