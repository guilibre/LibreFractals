import {
  Component,
  inject,
  signal,
  ElementRef,
  ViewChild,
  AfterViewInit,
  HostListener,
  ChangeDetectorRef,
} from '@angular/core';
import { DomSanitizer, SafeHtml } from '@angular/platform-browser';
import { FractalService } from './fractal/fractal.service';
import { GalleryService } from './gallery/gallery.service';
import { GalleryComponent } from './gallery/gallery.component';
import { SidebarComponent } from './sidebar/sidebar';
import { SvgViewerComponent } from './svg-viewer/svg-viewer';
import { AudioPlayerComponent } from './audio-player/audio-player';
import { ResizeHandleComponent } from './resize-handle/resize-handle';
import { compress, decompress } from './utils/url-state';

@Component({
  selector: 'app-root',
  imports: [
    SidebarComponent,
    SvgViewerComponent,
    AudioPlayerComponent,
    GalleryComponent,
    ResizeHandleComponent,
  ],
  templateUrl: './app.html',
  styleUrl: './app.scss',
})
export class App implements AfterViewInit {
  private fractal = inject(FractalService);
  private galleryService = inject(GalleryService);
  private sanitizer = inject(DomSanitizer);
  private cdr = inject(ChangeDetectorRef);

  @ViewChild('layout', { read: ElementRef }) layoutRef!: ElementRef<HTMLElement>;
  @ViewChild('inputPanel', { read: ElementRef }) inputPanelRef!: ElementRef<HTMLElement>;
  @ViewChild('audioStrip', { read: ElementRef }) audioStripRef?: ElementRef<HTMLElement>;
  @ViewChild('galleryPanel', { read: ElementRef }) galleryPanelRef?: ElementRef<HTMLElement>;

  private static readonly STORAGE_KEY = 'librefractals_source';
  private _source = localStorage.getItem(App.STORAGE_KEY) ?? '';

  constructor() {
    const hash = location.hash.slice(1);
    if (hash) {
      decompress(hash)
        .then((src) => {
          this._source = src;
          this.cdr.markForCheck();
        })
        .catch(() => {});
    }
  }

  get source() {
    return this._source;
  }
  set source(v: string) {
    this._source = v;
    localStorage.setItem(App.STORAGE_KEY, v);
    void compress(v).then((c) => (location.hash = c));
  }

  svg = signal<SafeHtml>('');
  loading = signal(false);
  audioLoading = signal(false);
  wavBase64 = signal('');
  error = signal('');
  galleryOpen = signal(false);
  galleryAdding = signal(false);

  isVertical = false;

  ngAfterViewInit() {
    this.updateOrientation();
  }

  @HostListener('window:resize')
  onWindowResize() {
    this.updateOrientation();
  }

  private updateOrientation() {
    const vertical = window.innerHeight > window.innerWidth;
    if (vertical !== this.isVertical) {
      this.isVertical = vertical;
      const layout = this.layoutRef.nativeElement;
      const panel = this.inputPanelRef.nativeElement;
      layout.classList.toggle('vertical', vertical);
      panel.style.width = '';
      panel.style.height = '';
    }
  }

  onSidebarResize(delta: number) {
    const panel = this.inputPanelRef.nativeElement;
    const galleryW = this.galleryPanelRef?.nativeElement.offsetWidth ?? 0;
    const galleryH = this.galleryPanelRef?.nativeElement.offsetHeight ?? 0;
    const audioH = this.audioStripRef?.nativeElement.offsetHeight ?? 0;
    const minMain = 150;
    if (this.isVertical) {
      const maxH = Math.min(
        window.innerHeight * 0.7,
        window.innerHeight - galleryH - audioH - minMain,
      );
      const newH = Math.max(100, Math.min(maxH, panel.offsetHeight + delta));
      panel.style.height = `${String(newH)}px`;
    } else {
      const maxW = Math.min(window.innerWidth * 0.7, window.innerWidth - galleryW - minMain);
      const newW = Math.max(140, Math.min(maxW, panel.offsetWidth + delta));
      panel.style.width = `${String(newW)}px`;
    }
  }

  onAudioResize(delta: number) {
    const strip = this.audioStripRef?.nativeElement;
    if (!strip) return;
    const sidebarH = this.isVertical ? this.inputPanelRef.nativeElement.offsetHeight : 0;
    const galleryH = this.isVertical ? (this.galleryPanelRef?.nativeElement.offsetHeight ?? 0) : 0;
    const maxH = Math.min(window.innerHeight * 0.6, window.innerHeight - sidebarH - galleryH - 100);
    const newH = Math.max(60, Math.min(maxH, strip.offsetHeight - delta));
    strip.style.height = `${String(newH)}px`;
  }

  onGalleryResize(delta: number) {
    const panel = this.galleryPanelRef?.nativeElement;
    if (!panel) return;
    const sidebarW = this.inputPanelRef.nativeElement.offsetWidth;
    const sidebarH = this.inputPanelRef.nativeElement.offsetHeight;
    const audioH = this.audioStripRef?.nativeElement.offsetHeight ?? 0;
    const minMain = 150;
    if (this.isVertical) {
      const maxH = Math.min(
        window.innerHeight * 0.6,
        window.innerHeight - sidebarH - audioH - minMain,
      );
      const newH = Math.max(80, Math.min(maxH, panel.offsetHeight - delta));
      panel.style.height = `${String(newH)}px`;
    } else {
      const maxW = Math.min(window.innerWidth * 0.5, window.innerWidth - sidebarW - minMain);
      const newW = Math.max(140, Math.min(maxW, panel.offsetWidth - delta));
      panel.style.width = `${String(newW)}px`;
    }
  }

  async listen() {
    this.audioLoading.set(true);
    this.wavBase64.set('');
    this.error.set('');
    try {
      const b64 = await this.fractal.compileWav(this.source);
      if (!b64) {
        this.error.set('No audio output — check your expression.');
      } else {
        this.wavBase64.set(b64);
      }
    } catch (e) {
      this.error.set(String(e));
    } finally {
      this.audioLoading.set(false);
    }
  }

  async render() {
    this.loading.set(true);
    this.error.set('');
    try {
      const result = await this.fractal.compileSvg(this.source);
      if (!result) {
        this.error.set('No output — check your expression.');
        this.svg.set('');
      } else {
        this.svg.set(this.sanitizer.bypassSecurityTrustHtml(result));
      }
    } catch (e) {
      this.error.set(String(e));
      this.svg.set('');
    } finally {
      this.loading.set(false);
    }
  }

  async addToGallery(name: string) {
    this.galleryAdding.set(true);
    this.error.set('');
    try {
      const svg = await this.fractal.compileSvg(this._source);
      const svgBytes = svg ? new TextEncoder().encode(svg).length : 0;
      if (svgBytes > 524288) {
        this.error.set('SVG too large (> 512 KB) — simplify the fractal before adding to gallery.');
        return;
      }
      const hash = await compress(this._source);
      const topo = await this.fractal.getTopoString(this._source);
      await this.galleryService.add(hash, name, svgBytes, topo);
    } catch (e) {
      this.error.set(String(e));
    } finally {
      this.galleryAdding.set(false);
    }
  }

  async loadFromGallery(source: string) {
    this.source = source;
    await this.render();
  }
}
