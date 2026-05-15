import {
  Component,
  inject,
  signal,
  ElementRef,
  ViewChild,
  AfterViewInit,
  OnDestroy,
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
import { compress, decompress } from './utils/url-state';

@Component({
  selector: 'app-root',
  imports: [SidebarComponent, SvgViewerComponent, AudioPlayerComponent, GalleryComponent],
  templateUrl: './app.html',
  styleUrl: './app.scss',
})
export class App implements AfterViewInit, OnDestroy {
  private fractal = inject(FractalService);
  private galleryService = inject(GalleryService);
  private sanitizer = inject(DomSanitizer);
  private cdr = inject(ChangeDetectorRef);

  @ViewChild('layout') layoutRef!: ElementRef<HTMLElement>;
  @ViewChild('inputPanel') inputPanelRef!: ElementRef<HTMLElement>;
  @ViewChild('audioStrip') audioStripRef?: ElementRef<HTMLElement>;

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

  private isVertical = false;
  private resizing = false;
  private startPos = 0;
  private startSize = 0;

  private audioResizing = false;
  private audioStartY = 0;
  private audioStartH = 0;

  private onAudioMouseMove = (e: MouseEvent) => {
    if (!this.audioResizing) return;
    const strip = this.audioStripRef?.nativeElement;
    if (!strip) return;
    const delta = this.audioStartY - e.clientY;
    const newH = Math.max(60, Math.min(window.innerHeight * 0.6, this.audioStartH + delta));
    strip.style.height = `${String(newH)}px`;
  };

  private onAudioMouseUp = () => {
    if (!this.audioResizing) return;
    this.audioResizing = false;
    document.body.style.cursor = '';
    document.body.style.userSelect = '';
  };

  private onMouseMove = (e: MouseEvent) => {
    if (!this.resizing) return;
    const panel = this.inputPanelRef.nativeElement;
    if (this.isVertical) {
      const delta = e.clientY - this.startPos;
      const newH = Math.max(120, Math.min(window.innerHeight * 0.7, this.startSize + delta));
      panel.style.height = `${String(newH)}px`;
    } else {
      const delta = e.clientX - this.startPos;
      const newW = Math.max(160, Math.min(window.innerWidth * 0.7, this.startSize + delta));
      panel.style.width = `${String(newW)}px`;
    }
  };

  private onMouseUp = () => {
    if (!this.resizing) return;
    this.resizing = false;
    document.body.style.cursor = '';
    document.body.style.userSelect = '';
  };

  ngAfterViewInit() {
    this.updateOrientation();
    window.addEventListener('mousemove', this.onMouseMove);
    window.addEventListener('mouseup', this.onMouseUp);
    window.addEventListener('mousemove', this.onAudioMouseMove);
    window.addEventListener('mouseup', this.onAudioMouseUp);
  }

  ngOnDestroy() {
    window.removeEventListener('mousemove', this.onMouseMove);
    window.removeEventListener('mouseup', this.onMouseUp);
    window.removeEventListener('mousemove', this.onAudioMouseMove);
    window.removeEventListener('mouseup', this.onAudioMouseUp);
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

  startAudioResize(e: MouseEvent) {
    const strip = this.audioStripRef?.nativeElement;
    if (!strip) return;
    this.audioResizing = true;
    this.audioStartY = e.clientY;
    this.audioStartH = strip.offsetHeight;
    document.body.style.cursor = 'row-resize';
    document.body.style.userSelect = 'none';
    e.preventDefault();
  }

  startResize(e: MouseEvent) {
    this.resizing = true;
    const panel = this.inputPanelRef.nativeElement;
    if (this.isVertical) {
      this.startPos = e.clientY;
      this.startSize = panel.offsetHeight;
      document.body.style.cursor = 'row-resize';
    } else {
      this.startPos = e.clientX;
      this.startSize = panel.offsetWidth;
      document.body.style.cursor = 'col-resize';
    }
    document.body.style.userSelect = 'none';
    e.preventDefault();
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

  copyShareUrl() {
    void compress(this._source).then((c) => {
      location.hash = c;
      void navigator.clipboard.writeText(location.href);
    });
  }

  async addToGallery(name: string) {
    this.galleryAdding.set(true);
    this.error.set('');
    try {
      const t0 = performance.now();
      await this.fractal.compileSvg(this._source);
      const compileMs = Math.round(performance.now() - t0);
      const hash = await compress(this._source);
      await this.galleryService.add(hash, name, compileMs);
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
