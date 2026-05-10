import { EditorView } from "@codemirror/view";
import { linter, type Diagnostic } from "@codemirror/lint";
import {
  autocompletion,
  type CompletionContext,
  type Completion,
} from "@codemirror/autocomplete";
import { FractalService } from "../fractal.service";
import {
  lineColToOffset,
  offsetToLineCol,
  showHoverTooltip,
} from "./lsp-utils";

export function diagnosticsExtension(fractal: FractalService) {
  return linter(async (view) => {
    const source = view.state.doc.toString();
    const raw = await fractal.getDiagnostics(source);
    if (!raw) return [];
    const items: Array<{
      message: string;
      from: { line: number; col: number };
      to: { line: number; col: number };
      severity: string;
    }> = JSON.parse(raw);
    const diags: Diagnostic[] = [];
    for (const d of items) {
      const fromOff = lineColToOffset(view.state.doc, d.from.line, d.from.col);
      const toOff = lineColToOffset(view.state.doc, d.to.line, d.to.col);
      if (fromOff < 0 || toOff < 0) continue;
      diags.push({
        from: fromOff,
        to: Math.max(fromOff + 1, toOff),
        severity: "error",
        message: d.message,
      });
    }
    return diags;
  });
}

export function completionsExtension(fractal: FractalService) {
  const source = async (ctx: CompletionContext) => {
    const doc = ctx.state.doc.toString();
    const { line, col } = offsetToLineCol(ctx.state.doc, ctx.pos);
    const raw = await fractal.getCompletions(doc, line, col);
    if (!raw) return null;
    const items: Array<{ label: string; detail: string; kind: string }> =
      JSON.parse(raw);
    const options: Completion[] = items.map((item) => ({
      label: item.label,
      detail: item.detail,
      type:
        item.kind === "keyword"
          ? "keyword"
          : item.kind === "builtin"
            ? "function"
            : "variable",
    }));
    return { from: ctx.pos, options };
  };
  return autocompletion({ override: [source] });
}

export function hoverExtension(fractal: FractalService) {
  return EditorView.domEventHandlers({
    mousemove: (event, view) => {
      const pos = view.posAtCoords({ x: event.clientX, y: event.clientY });
      if (pos == null) return;
      const source = view.state.doc.toString();
      const { line, col } = offsetToLineCol(view.state.doc, pos);
      fractal.getHover(source, line, col).then((raw) => {
        if (!raw || raw === "null") return;
        const parsed: { contents: string } = JSON.parse(raw);
        showHoverTooltip(view, pos, parsed.contents);
      });
    },
  });
}
