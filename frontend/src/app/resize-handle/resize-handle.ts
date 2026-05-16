import { Component, OnInit, OnDestroy, input, output } from '@angular/core';

@Component({
  selector: 'app-resize-handle',
  host: { '[attr.direction]': 'direction()' },
  template: `<div
    class="handle"
    (mousedown)="onStart($event)"
    (touchstart)="onStart($event)"
  ></div>`,
  styleUrl: './resize-handle.scss',
})
export class ResizeHandleComponent implements OnInit, OnDestroy {
  readonly direction = input.required<'horizontal' | 'vertical'>();
  readonly dragDelta = output<number>();

  private dragging = false;
  private startPos = 0;

  onStart(e: MouseEvent | TouchEvent) {
    this.dragging = true;
    const pos = 'touches' in e ? e.touches.item(0) : e;
    this.startPos = this.direction() === 'horizontal' ? (pos?.clientX ?? 0) : (pos?.clientY ?? 0);
    document.body.style.userSelect = 'none';
    document.body.style.cursor = this.direction() === 'horizontal' ? 'col-resize' : 'row-resize';
    e.preventDefault();
  }

  private onMove = (e: MouseEvent | TouchEvent) => {
    if (!this.dragging) return;
    const isTouch = 'touches' in e;
    const pos = isTouch ? e.touches.item(0) : e;
    if (!pos) return;
    if (isTouch) e.preventDefault();
    const current = this.direction() === 'horizontal' ? pos.clientX : pos.clientY;
    const delta = current - this.startPos;
    this.startPos = current;
    this.dragDelta.emit(delta);
  };

  private onEnd = () => {
    if (!this.dragging) return;
    this.dragging = false;
    document.body.style.userSelect = '';
    document.body.style.cursor = '';
  };

  ngOnInit() {
    window.addEventListener('mousemove', this.onMove);
    window.addEventListener('mouseup', this.onEnd);
    window.addEventListener('touchmove', this.onMove, { passive: false });
    window.addEventListener('touchend', this.onEnd);
  }

  ngOnDestroy() {
    window.removeEventListener('mousemove', this.onMove);
    window.removeEventListener('mouseup', this.onEnd);
    window.removeEventListener('touchmove', this.onMove);
    window.removeEventListener('touchend', this.onEnd);
  }
}
