import { EditorView } from '@codemirror/view';
import type { EditorState } from '@codemirror/state';

export function lineColToOffset(doc: EditorState['doc'], line: number, col: number): number {
  const lineNum = Math.max(1, Math.min(line + 1, doc.lines));
  const lineObj = doc.line(lineNum);
  return Math.min(lineObj.from + col, lineObj.to);
}

export function offsetToLineCol(
  doc: EditorState['doc'],
  offset: number,
): { line: number; col: number } {
  const lineObj = doc.lineAt(offset);
  return { line: lineObj.number - 1, col: offset - lineObj.from };
}

let activeTooltipEl: HTMLElement | null = null;
let activeTooltipTimeout: ReturnType<typeof setTimeout> | null = null;

export function showHoverTooltip(view: EditorView, pos: number, contents: string) {
  if (activeTooltipTimeout) clearTimeout(activeTooltipTimeout);
  if (activeTooltipEl) {
    activeTooltipEl.remove();
    activeTooltipEl = null;
  }

  const dom = document.createElement('div');
  dom.className = 'cm-tooltip cm-hover-tooltip';
  dom.textContent = contents.replace(/\*\*(.*?)\*\*/g, '$1').replace(/`(.*?)`/g, '$1');
  dom.style.cssText =
    'position:fixed;z-index:9999;pointer-events:none;background:#222;color:#ddd;' +
    'border:1px solid #444;border-radius:4px;padding:4px 8px;font-family:monospace;' +
    'font-size:0.8rem;max-width:320px;white-space:pre-wrap;';

  const coords = view.coordsAtPos(pos);
  if (!coords) return;
  dom.style.left = `${String(coords.left)}px`;
  dom.style.top = `${String(coords.bottom + 4)}px`;
  document.body.appendChild(dom);
  activeTooltipEl = dom;

  activeTooltipTimeout = setTimeout(() => {
    dom.remove();
    activeTooltipEl = null;
  }, 3000);
}
