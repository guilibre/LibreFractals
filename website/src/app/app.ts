import {
  Component,
  inject,
  signal,
  ElementRef,
  ViewChild,
  AfterViewInit,
  OnDestroy,
  HostListener,
} from "@angular/core";
import { DomSanitizer, SafeHtml } from "@angular/platform-browser";
import { FractalService } from "./fractal.service";
import { EditorComponent } from "./editor/editor";

const DEFAULT_SOURCE = `!6;
@S(50)V(50)mx;

x=F(1);
f=F(1);
p=-(25);
m=+(25);

x -> H(10)fp[[x]mx]mf[mfx]px;
f -> ff;`;

@Component({
  selector: "app-root",
  imports: [EditorComponent],
  templateUrl: "./app.html",
  styleUrl: "./app.scss",
})
export class App implements AfterViewInit, OnDestroy {
  private fractal = inject(FractalService);
  private sanitizer = inject(DomSanitizer);

  @ViewChild("layout") layoutRef!: ElementRef<HTMLElement>;
  @ViewChild("inputPanel") inputPanelRef!: ElementRef<HTMLElement>;

  private static readonly STORAGE_KEY = "librefractals_source";
  private _source = localStorage.getItem(App.STORAGE_KEY) ?? DEFAULT_SOURCE;

  get source() {
    return this._source;
  }
  set source(v: string) {
    this._source = v;
    localStorage.setItem(App.STORAGE_KEY, v);
  }
  svg = signal<SafeHtml>("");
  loading = signal(false);
  error = signal("");

  private isVertical = false;
  private resizing = false;
  private startPos = 0;
  private startSize = 0;

  private onMouseMove = (e: MouseEvent) => {
    if (!this.resizing) return;
    const panel = this.inputPanelRef.nativeElement;
    if (this.isVertical) {
      const delta = e.clientY - this.startPos;
      const newH = Math.max(
        120,
        Math.min(window.innerHeight * 0.7, this.startSize + delta),
      );
      panel.style.height = newH + "px";
    } else {
      const delta = e.clientX - this.startPos;
      const newW = Math.max(
        160,
        Math.min(window.innerWidth * 0.7, this.startSize + delta),
      );
      panel.style.width = newW + "px";
    }
  };

  private onMouseUp = () => {
    if (!this.resizing) return;
    this.resizing = false;
    document.body.style.cursor = "";
    document.body.style.userSelect = "";
  };

  ngAfterViewInit() {
    this.updateOrientation();
    window.addEventListener("mousemove", this.onMouseMove);
    window.addEventListener("mouseup", this.onMouseUp);
  }

  ngOnDestroy() {
    window.removeEventListener("mousemove", this.onMouseMove);
    window.removeEventListener("mouseup", this.onMouseUp);
  }

  @HostListener("window:resize")
  onWindowResize() {
    this.updateOrientation();
  }

  private updateOrientation() {
    const vertical = window.innerHeight > window.innerWidth;
    if (vertical !== this.isVertical) {
      this.isVertical = vertical;
      const layout = this.layoutRef.nativeElement;
      const panel = this.inputPanelRef.nativeElement;
      layout.classList.toggle("vertical", vertical);
      panel.style.width = "";
      panel.style.height = "";
    }
  }

  startResize(e: MouseEvent) {
    this.resizing = true;
    const panel = this.inputPanelRef.nativeElement;
    if (this.isVertical) {
      this.startPos = e.clientY;
      this.startSize = panel.offsetHeight;
      document.body.style.cursor = "row-resize";
    } else {
      this.startPos = e.clientX;
      this.startSize = panel.offsetWidth;
      document.body.style.cursor = "col-resize";
    }
    document.body.style.userSelect = "none";
    e.preventDefault();
  }

  async render() {
    this.loading.set(true);
    this.error.set("");
    try {
      const result = await this.fractal.compileSvg(this.source);
      if (!result) {
        this.error.set("No output — check your expression.");
        this.svg.set("");
      } else {
        this.svg.set(this.sanitizer.bypassSecurityTrustHtml(result));
      }
    } catch (e) {
      this.error.set(String(e));
      this.svg.set("");
    } finally {
      this.loading.set(false);
    }
  }
}
