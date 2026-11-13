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

- `GameLogic.h` — regras do jogo como template `GameLogic<N>`, sem depender de interface
- `Observer.h` — padrão Observer: estatísticas e log reagem aos eventos do jogo
- `Renderer.h` — interface `IRenderer`; `Renderer.cpp` (terminal) e `SDLRenderer.cpp` (SDL2) a implementam
- `Controller.cpp` / `SDLController.cpp` — loop de cada versão
- `Persistence.cpp` — save em arquivo texto com `<fstream>` e `std::optional`

## Compilar

```bash
# dependências (Ubuntu/Debian)
sudo apt install g++ make libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core

make          # versão terminal  → ./2048
make sdl      # versão gráfica   → ./2048_sdl
```

## Versão web (WebAssembly)

O mesmo código SDL2 é compilado para WebAssembly com [Emscripten](https://emscripten.org/)
(que já traz SDL2 e SDL2_ttf). As diferenças ficam isoladas em `#ifdef __EMSCRIPTEN__`:

- o loop `while` vira um quadro por `requestAnimationFrame` (`SDLController::step`)
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
