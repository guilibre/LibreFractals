import { Injectable } from "@angular/core";

interface WasmModule {
  compile_to_svg: (source: string) => string;
  get_diagnostics: (source: string) => string;
  get_hover: (source: string, line: number, col: number) => string;
  get_completions: (source: string, line: number, col: number) => string;
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
const dynamicImport = new Function("url", "return import(url)") as (
  url: string,
) => Promise<any>;

@Injectable({ providedIn: "root" })
export class FractalService {
  private modulePromise: Promise<WasmModule> | null = null;

  private load(): Promise<WasmModule> {
    if (!this.modulePromise) {
      const base = document.baseURI;
      const jsUrl = new URL("wasm/LibreFractals.js", base).href;
      this.modulePromise = dynamicImport(jsUrl).then(
        (mod: { default: (arg?: object) => Promise<WasmModule> }) =>
          mod.default({
            locateFile: (f: string) => new URL(`wasm/${f}`, base).href,
          }),
      );
    }
    return this.modulePromise;
  }

  async compileSvg(source: string): Promise<string> {
    const mod = await this.load();
    return mod.compile_to_svg(source);
  }

  async getDiagnostics(source: string): Promise<string> {
    const mod = await this.load();
    return mod.get_diagnostics(source);
  }

  async getHover(source: string, line: number, col: number): Promise<string> {
    const mod = await this.load();
    return mod.get_hover(source, line, col);
  }

  async getCompletions(
    source: string,
    line: number,
    col: number,
  ): Promise<string> {
    const mod = await this.load();
    return mod.get_completions(source, line, col);
  }
}
