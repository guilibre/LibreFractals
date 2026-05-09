import { Injectable } from "@angular/core";

interface WasmModule {
  compile_to_svg: (source: string) => string;
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
}
