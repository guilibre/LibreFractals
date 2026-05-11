import { Injectable } from "@angular/core";

const TIMEOUT_MS = 10_000;

interface Pending {
  resolve: (v: string) => void;
  reject: (e: Error) => void;
}

@Injectable({ providedIn: "root" })
export class FractalService {
  private worker: Worker | null = null;
  private pending = new Map<number, Pending>();
  private nextId = 0;

  private getWorker(): Worker {
    if (!this.worker) {
      this.worker = new Worker(new URL("./compiler.worker", import.meta.url), {
        type: "module",
      });
      this.worker.onmessage = ({
        data,
      }: MessageEvent<{ id: number; result?: string; error?: string }>) => {
        const p = this.pending.get(data.id);
        if (!p) return;
        this.pending.delete(data.id);
        if (data.error !== undefined) p.reject(new Error(data.error));
        else p.resolve(data.result ?? "");
      };
    }
    return this.worker;
  }

  private call(fn: string, args: unknown[]): Promise<string> {
    return new Promise((resolve, reject) => {
      const id = this.nextId++;
      const timer = setTimeout(() => {
        this.pending.delete(id);
        this.worker?.terminate();
        this.worker = null;
        reject(
          new Error(
            "Timeout: recursão muito profunda ou expressão muito complexa.",
          ),
        );
      }, TIMEOUT_MS);

      this.pending.set(id, {
        resolve: (v) => {
          clearTimeout(timer);
          resolve(v);
        },
        reject: (e) => {
          clearTimeout(timer);
          reject(e);
        },
      });

      this.getWorker().postMessage({ id, fn, args, baseUrl: document.baseURI });
    });
  }

  compileWav(source: string): Promise<string> {
    return this.call("compile_to_wav", [source]);
  }

  compileSvg(source: string): Promise<string> {
    return this.call("compile_to_svg", [source]);
  }

  getDiagnostics(source: string): Promise<string> {
    return this.call("get_diagnostics", [source]);
  }

  getHover(source: string, line: number, col: number): Promise<string> {
    return this.call("get_hover", [source, line, col]);
  }

  getCompletions(source: string, line: number, col: number): Promise<string> {
    return this.call("get_completions", [source, line, col]);
  }
}
