/// <reference lib="webworker" />

interface WasmModule {
  compile_to_wav: (source: string) => string;
  compile_to_svg: (source: string) => string;
  get_diagnostics: (source: string) => string;
  get_hover: (source: string, line: number, col: number) => string;
  get_completions: (source: string, line: number, col: number) => string;
}

// eslint-disable-next-line @typescript-eslint/no-implied-eval, @typescript-eslint/no-explicit-any
const dynamicImport = new Function('url', 'return import(url)') as (url: string) => Promise<any>;

let modulePromise: Promise<WasmModule> | null = null;

function load(baseUrl: string): Promise<WasmModule> {
  if (!modulePromise) {
    const jsUrl = new URL('wasm/LibreFractals.js', baseUrl).href;
    modulePromise = dynamicImport(jsUrl).then(
      (mod: { default: (arg?: object) => Promise<WasmModule> }) =>
        mod.default({
          locateFile: (f: string) => new URL(`wasm/${f}`, baseUrl).href,
        }),
    );
  }
  return modulePromise;
}

addEventListener('message', ({ data }) => {
  void (async () => {
    const { id, fn, args, baseUrl } = data as {
      id: number;
      fn: string;
      args: unknown[];
      baseUrl: string;
    };
    try {
      const mod = await load(baseUrl);
      // eslint-disable-next-line @typescript-eslint/no-explicit-any, @typescript-eslint/no-unsafe-call, @typescript-eslint/no-unsafe-member-access
      const result = (mod as any)[fn](...args) as string;
      postMessage({ id, result });
    } catch (error) {
      postMessage({ id, error: String(error) });
    }
  })();
});
