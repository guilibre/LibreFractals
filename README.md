# LibreFractals

A fractal description language with a compiler targeting SVG, with a web playground powered by WebAssembly.

**Live demo:** https://guilibre.github.io/LibreFractals/

## Overview

LibreFractals provides a small domain-specific language (`.frac`) for describing L-system fractals. The compiler parses the language, expands rewrite rules, and renders the result as an SVG via a turtle graphics renderer. The same compiler is compiled to WebAssembly and embedded in an Angular web app so fractals can be generated directly in the browser.

## Repository structure

```
compiler/      C++23 compiler (parser, codegen, SVG renderer)
website/       Angular frontend that loads the WASM module
build-wasm.sh  Builds the compiler to WASM and copies it into the website
```

## Language

A `.frac` file has three parts:

| Line                         | Meaning                   |
| ---------------------------- | ------------------------- |
| `!<n>;`                      | Number of expansion steps |
| `@<symbols>;`                | Axiom (initial string)    |
| `<symbol> -> <replacement>;` | Rewrite rule              |

Draw commands:

| Symbol    | Effect                                                  |
| --------- | ------------------------------------------------------- |
| `F(<n>)`  | Draw forward by `n` units                               |
| `B(<n>)`  | Step forward by `n` units without drawing               |
| `+(<n>)`  | Rotate left (counter-clockwise) by `n` degrees          |
| `-(<n>)`  | Rotate right (clockwise) by `n` degrees                 |
| `*(<n>)`  | Multiply current scale by `n`                           |
| `%(<n>)`  | Multiply current stroke width by `n`                    |
| `H(<n>)`  | Shift hue by `n` degrees (0–360)                        |
| `S(<n>)`  | Add `n` to saturation (percentage)                      |
| `V(<n>)`  | Add `n` to value/brightness (percentage)                |
| `A(<n>)`  | Add `n` to alpha/opacity (percentage)                   |
| `[` / `]` | Push / pop turtle state (position, angle, scale, color) |

Program-level directives:

| Directive                                  | Meaning                                   |
| ------------------------------------------ | ----------------------------------------- |
| `!<n>;`                                    | Number of expansion steps                 |
| `@<symbols>;`                              | Axiom (initial string)                    |
| `?<n>;`                                    | Random seed (for stochastic rules)        |
| `<var> -> <symbols>;`                      | Deterministic rewrite rule                |
| `<var> -> <p>~<symbols> \| <p>~<symbols>;` | Stochastic rule with branch probabilities |

**Example:**

```
!32;
@V(100) S(70) H(200) a;
?5;

a -> 0.9 ~ F(1) +(7) %(0.9) a
   | 0.1 ~ [+(40) H(50) *(0.78) b] [-(40) H(-35) *(0.78) b] F(1) a;

b -> 0.88 ~ H(10) F(1) -(6) *(0.96) %(0.9) b
   | 0.12 ~ [+(38) *(0.82) a] [-(38) H(70) *(0.82) a];
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

The output (`LibreFractals.js` / `LibreFractals.wasm`) is placed in `website/public/wasm/`.

### Web frontend

Requires Node.js and the Angular CLI.

```bash
cd website
npm install
npm start       # dev server at http://localhost:4200
npm run build   # production build
npm run deploy  # deploy to GitHub Pages
```

## Usage (CLI)

```
librefractals [options] <input>

  <input>              Fractal expression string or path to a .frac file

Options:
  -f, --file           Treat <input> as a file path
  -o, --output <path>  Write output to file instead of stdout
  --svg                Output SVG instead of turtle IR
  -h, --help           Show this help message
```

**Generate an SVG from a file:**

```bash
./librefractals -f --svg a.frac -o out.svg
```

## License

See [LICENSE](LICENSE).
