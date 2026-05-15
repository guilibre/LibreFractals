# LibreFractals

A domain-specific language for L-system fractals, with a C++23 compiler that outputs SVG and audio, and a browser playground powered by WebAssembly.

**Live demo:** https://guilibre.github.io/LibreFractals/

## Language

A `.frac` file is made of directives, optional aliases, and rewrite rules.

### Directives

| Directive                                      | Meaning                                                       |
| ---------------------------------------------- | ------------------------------------------------------------- |
| `!<n>;`                                        | Number of expansion steps                                     |
| `@<symbols>;`                                  | Axiom — the initial symbol string                             |
| `?<n>;`                                        | Random seed (required for stochastic rules)                   |
| `$<n>;`                                        | Minimum note duration in seconds (audio output)               |
| `&<n>;`                                        | Glissando fraction — pitch slide as a fraction of note [0, 1] |
| `<var> = <symbols>;`                           | Alias — shorthand that expands before rewriting               |
| `<var> -> <symbols>;`                          | Deterministic rewrite rule                                    |
| `<var> -> <p> ~ <symbols> \| <p> ~ <symbols>;` | Stochastic rule — branches chosen by probability              |

### Draw commands

| Symbol   | Effect                                            |
| -------- | ------------------------------------------------- |
| `F(<n>)` | Draw forward `n` units                            |
| `B(<n>)` | Move forward `n` units without drawing            |
| `+(<n>)` | Rotate left (counter-clockwise) by `n` degrees    |
| `-(<n>)` | Rotate right (clockwise) by `n` degrees           |
| `*(<n>)` | Multiply the current scale by `n`                 |
| `%(<n>)` | Multiply the current stroke width by `n`          |
| `H(<n>)` | Shift hue by `n` degrees                          |
| `S(<n>)` | Add `n` to saturation (%)                         |
| `V(<n>)` | Add `n` to value/brightness (%)                   |
| `A(<n>)` | Add `n` to opacity (%)                            |
| `[`      | Push turtle state (position, angle, scale, color) |
| `]`      | Pop turtle state                                  |

### Example — Dragon curve

```
!14;
@a;

a -> a +(90) b;
b -> a -(90) b;
```

### Example — Stochastic tree

```
!32;
@V(100) S(70) H(200) a;
?5;

a -> 0.9 ~ F(1) +(7) %(0.9) a
   | 0.1 ~ [+(40) H(50) *(0.78) b] [-(40) H(-35) *(0.78) b] F(1) a;

b -> 0.88 ~ H(10) F(1) -(6) *(0.96) %(0.9) b
   | 0.12 ~ [+(38) *(0.82) a] [-(38) H(70) *(0.82) a];
```

### Example — Musical fractal

```
!5;
@a;
a=F(1); b=B(2); p=+(20); m=-(20);

a -> abp[*(2)%(0.5)+(100)a]bma;
b -> bma[*(2)%(0.5)-(100)b]pab;
```

Turtle forward/backward motion is mapped to pitch and note duration, producing WAV audio.

## Features

- **Visual output** — SVG rendered from turtle graphics with full color and stroke control
- **Audio output** — L-system traversal mapped to MIDI-like notes, rendered to WAV with optional glissando
- **Browser playground** — editor with real-time LSP diagnostics, hover tooltips, and autocompletion (CodeMirror 6 + WASM)
- **Shareable URLs** — fractal source compressed with deflate-raw and encoded in the URL hash
- **Gallery** — community gallery backed by a C++ REST API

## Repository structure

```
compiler/      C++23 compiler — parser, rewriter, SVG renderer, MIDI/WAV renderer, LSP server
backend/       C++ REST API for the gallery (Drogon framework)
frontend/      Angular frontend that loads the WASM module
build-wasm.sh  Builds the compiler to WASM and copies output into the frontend
```

## Building

### Native compiler

Requires CMake ≥ 4.2 and a C++23-capable compiler.

```bash
cmake -S compiler -B compiler/build -DCMAKE_BUILD_TYPE=Release
cmake --build compiler/build -j$(nproc)
```

### WebAssembly module

Requires [Emscripten](https://emscripten.org/).

```bash
./build-wasm.sh
```

Output (`LibreFractals.js` / `LibreFractals.wasm`) is placed in `frontend/public/wasm/`.

### Backend

Requires CMake ≥ 4.2 and a C++23-capable compiler. Dependencies (Drogon, yaml-cpp, jsoncpp) are fetched automatically via CMake FetchContent.

```bash
cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release
cmake --build backend/build -j$(nproc)
```

### Web frontend

Requires Node.js ≥ 18 and the Angular CLI.

```bash
cd frontend
npm install
npm start       # dev server at http://localhost:4200
npm run build   # production build
npm run deploy  # deploy to GitHub Pages
```

## CLI usage

```
librefractals [options] <input>

  <input>              Fractal expression string or path to a .frac file

Options:
  -f, --file           Treat <input> as a file path
  -o, --output <path>  Write output to file instead of stdout
  --svg                Output SVG
  -h, --help           Show this help message
```

```bash
./librefractals -f --svg a.frac -o out.svg
```

## License

See [LICENSE](LICENSE).
