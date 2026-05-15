import {
  Component,
  ElementRef,
  ViewChild,
  AfterViewInit,
  OnDestroy,
  Input,
  Output,
  EventEmitter,
  inject,
  OnChanges,
  SimpleChanges,
} from '@angular/core';
import { EditorState } from '@codemirror/state';
import { EditorView, keymap, tooltips } from '@codemirror/view';
import { defaultKeymap, history, historyKeymap } from '@codemirror/commands';
import { lintGutter } from '@codemirror/lint';
import { FractalService } from '../fractal/fractal.service';
import { diagnosticsExtension, completionsExtension, hoverExtension } from './extensions';

@Component({
  selector: 'app-editor',
  template: `<div #host class="cm-host"></div>`,
  styleUrl: './editor.scss',
})
export class EditorComponent implements AfterViewInit, OnDestroy, OnChanges {
  @ViewChild('host') hostRef!: ElementRef<HTMLElement>;
  @Input() value = '';
  @Output() valueChange = new EventEmitter<string>();

  private fractal = inject(FractalService);
  private view: EditorView | null = null;

  ngAfterViewInit() {
    const updateListener = EditorView.updateListener.of((update) => {
      if (update.docChanged) {
        this.valueChange.emit(update.state.doc.toString());
      }
    });

    this.view = new EditorView({
      state: EditorState.create({
        doc: this.value,
        extensions: [
          history(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          lintGutter(),
          diagnosticsExtension(this.fractal),
          completionsExtension(this.fractal),
          hoverExtension(this.fractal),
          updateListener,
          tooltips({ position: 'absolute' }),
          EditorView.lineWrapping,
        ],
      }),
      parent: this.hostRef.nativeElement,
    });
  }

  ngOnChanges(changes: SimpleChanges) {
    if (!this.view) return;
    const val = changes['value'].currentValue as string | undefined;
    if (val !== undefined && val !== this.view.state.doc.toString()) {
      this.view.dispatch({
        changes: { from: 0, to: this.view.state.doc.length, insert: val },
      });
    }
  }

  ngOnDestroy() {
    this.view?.destroy();
  }
}
