import { Component, input, output, inject, signal, OnInit } from '@angular/core';
import { DomSanitizer, SafeHtml } from '@angular/platform-browser';
import { FractalService } from '../fractal/fractal.service';
import { GalleryEntry } from './gallery.service';
import { decompress } from '../utils/url-state';

@Component({
  selector: 'app-gallery-entry',
  templateUrl: './gallery-entry.component.html',
  styleUrl: './gallery-entry.component.scss',
})
export class GalleryEntryComponent implements OnInit {
  entry = input.required<GalleryEntry>();
  load = output<string>();

  private fractal = inject(FractalService);
  private sanitizer = inject(DomSanitizer);

  svg = signal<SafeHtml>('');
  loading = signal(true);

  ngOnInit(): void {
    void (async () => {
      try {
        const source = await decompress(this.entry().hash);
        const result = await this.fractal.compileSvg(source);
        if (result) this.svg.set(this.sanitizer.bypassSecurityTrustHtml(result));
      } catch {
        // silent — broken entries show no preview
      } finally {
        this.loading.set(false);
      }
    })();
  }

  onClick(): void {
    void (async () => {
      try {
        const source = await decompress(this.entry().hash);
        this.load.emit(source);
      } catch {
        // ignore
      }
    })();
  }

  formatDate(ts: number): string {
    const d = new Date(ts * 1000);
    return d.toLocaleDateString(undefined, { month: 'short', day: 'numeric' });
  }
}
