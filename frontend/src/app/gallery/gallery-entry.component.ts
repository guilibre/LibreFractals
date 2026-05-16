import { Component, input, output, inject, signal, OnInit } from '@angular/core';
import { DomSanitizer, SafeHtml } from '@angular/platform-browser';
import { FractalService } from '../fractal/fractal.service';
import { GalleryEntry, GalleryService, RelativeEntry } from './gallery.service';
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
  private galleryService = inject(GalleryService);
  private sanitizer = inject(DomSanitizer);

  svg = signal<SafeHtml>('');
  loading = signal(true);
  relatives = signal<RelativeEntry[]>([]);
  relativesLoading = signal(false);
  showRelatives = signal(false);

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

  toggleRelatives(event: Event): void {
    event.stopPropagation();
    if (this.showRelatives()) {
      this.showRelatives.set(false);
      return;
    }
    this.relativesLoading.set(true);
    void (async () => {
      try {
        const list = await this.galleryService.getRelatives(this.entry().hash);
        this.relatives.set(list);
        this.showRelatives.set(true);
      } catch {
        this.relatives.set([]);
        this.showRelatives.set(true);
      } finally {
        this.relativesLoading.set(false);
      }
    })();
  }

  loadRelative(hash: string, event: Event): void {
    event.stopPropagation();
    void (async () => {
      try {
        const source = await decompress(hash);
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
