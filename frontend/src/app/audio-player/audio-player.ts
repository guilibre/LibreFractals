import {
  Component,
  Input,
  OnChanges,
  OnDestroy,
  ViewChild,
  ElementRef,
  signal,
  NgZone,
} from '@angular/core';

function fft(re: Float64Array, im: Float64Array): void {
  const n = re.length;
  for (let i = 1, j = 0; i < n; i++) {
    let bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) {
      [re[i], re[j]] = [re[j], re[i]];
      [im[i], im[j]] = [im[j], im[i]];
    }
  }
  for (let len = 2; len <= n; len <<= 1) {
    const ang = (-2 * Math.PI) / len;
    const wRe = Math.cos(ang);
    const wIm = Math.sin(ang);
    for (let i = 0; i < n; i += len) {
      let uRe = 1;
      let uIm = 0;
      const half = len >> 1;
      for (let j = 0; j < half; j++) {
        const idx = i + j + half;
        const vRe = re[idx] * uRe - im[idx] * uIm;
        const vIm = re[idx] * uIm + im[idx] * uRe;
        re[idx] = re[i + j] - vRe;
        im[idx] = im[i + j] - vIm;
        re[i + j] += vRe;
        im[i + j] += vIm;
        const nr = uRe * wRe - uIm * wIm;
        uIm = uRe * wIm + uIm * wRe;
        uRe = nr;
      }
    }
  }
}

function buildSpectrogram(samples: Float32Array, sampleRate: number, canvasH: number): ImageData {
  const N = 2048;
  const HOP = 512;
  const frames = Math.floor((samples.length - N) / HOP) + 1;
  const width = Math.max(1, frames);
  const img = new ImageData(width, canvasH);

  const hann = new Float64Array(N);
  for (let i = 0; i < N; i++) hann[i] = 0.5 * (1 - Math.cos((2 * Math.PI * i) / (N - 1)));

  const re = new Float64Array(N);
  const im = new Float64Array(N);

  const minF = Math.log2(20);
  const maxF = Math.log2(sampleRate / 2);

  for (let f = 0; f < frames; f++) {
    const offset = f * HOP;
    for (let i = 0; i < N; i++) {
      re[i] = (samples[offset + i] ?? 0) * hann[i];
      im[i] = 0;
    }
    fft(re, im);

    for (let py = 0; py < canvasH; py++) {
      const freq = Math.pow(2, minF + ((canvasH - 1 - py) / (canvasH - 1)) * (maxF - minF));
      const bin = Math.round((freq / (sampleRate / 2)) * (N / 2));
      const k = Math.max(0, Math.min(N / 2 - 1, bin));
      const mag = Math.sqrt(re[k] * re[k] + im[k] * im[k]) / N;
      const db = 20 * Math.log10(Math.max(mag, 1e-6));
      const t = Math.max(0, Math.min(1, (db + 80) / 80));
      const v = Math.round(t * 255);
      const idx = (py * width + f) * 4;
      img.data[idx] = v;
      img.data[idx + 1] = v;
      img.data[idx + 2] = v;
      img.data[idx + 3] = 255;
    }
  }

  return img;
}

@Component({
  selector: 'app-audio-player',
  templateUrl: './audio-player.html',
  styleUrl: './audio-player.scss',
})
export class AudioPlayerComponent implements OnChanges, OnDestroy {
  @Input() wavBase64 = '';
  @ViewChild('canvas') canvasRef!: ElementRef<HTMLCanvasElement>;
  @ViewChild('playheadLine') playheadLineRef!: ElementRef<HTMLDivElement>;

  playing = signal(false);
  currentTime = signal(0);
  duration = signal(0);
  spectrogramReady = signal(false);

  private audioCtx: AudioContext | null = null;
  private audioBuffer: AudioBuffer | null = null;
  private source: AudioBufferSourceNode | null = null;
  private startedAt = 0;
  private offsetAt = 0;
  private rafId = 0;

  private generation = 0;

  constructor(private zone: NgZone) {}

  ngOnChanges(): void {
    void (async () => {
      const gen = ++this.generation;
      this.stop();
      if (!this.wavBase64) return;

      const normalized = this.wavBase64.replace(/[\s]/g, '').replace(/-/g, '+').replace(/_/g, '/');
      const arrayBuffer = await fetch(`data:audio/wav;base64,${normalized}`).then((r) =>
        r.arrayBuffer(),
      );
      if (gen !== this.generation) return;

      if (!this.audioCtx) this.audioCtx = new AudioContext();
      const decoded = await this.audioCtx.decodeAudioData(arrayBuffer);
      if (gen !== this.generation) return;

      this.audioBuffer = decoded;
      this.duration.set(this.audioBuffer.duration);
      this.currentTime.set(0);
      this.offsetAt = 0;

      this.spectrogramReady.set(false);
      setTimeout(() => {
        this.drawSpectrogram();
      }, 0);
    })();
  }

  ngOnDestroy() {
    this.stop();
    void this.audioCtx?.close();
    cancelAnimationFrame(this.rafId);
  }

  private drawSpectrogram() {
    const canvas = this.canvasRef.nativeElement;
    if (!this.audioBuffer) return;

    const canvasH = canvas.clientHeight || canvas.offsetHeight || 120;
    const samples = this.audioBuffer.getChannelData(0);
    const img = buildSpectrogram(samples, this.audioBuffer.sampleRate, canvasH);

    canvas.width = img.width;
    canvas.height = canvasH;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;
    ctx.putImageData(img, 0, 0);

    this.zone.run(() => {
      this.spectrogramReady.set(true);
    });
  }

  private updatePlayheadLine(t: number) {
    const line = this.playheadLineRef.nativeElement;
    if (!this.audioBuffer) return;
    const pct = (t / this.audioBuffer.duration) * 100;
    line.style.left = `${String(pct)}%`;
    line.style.display = 'block';
  }

  private hidePlayheadLine() {
    this.playheadLineRef.nativeElement.style.display = 'none';
  }

  togglePlay() {
    if (this.playing()) {
      this.pause();
    } else {
      this.play();
    }
  }

  stopAndReset() {
    this.stop();
    this.hidePlayheadLine();
  }

  seekTouch(event: TouchEvent) {
    event.preventDefault();
    const touch = event.touches.item(0);
    if (!touch) return;
    this.seek({ clientX: touch.clientX } as unknown as MouseEvent);
  }

  seek(event: MouseEvent) {
    if (!this.audioBuffer) return;
    const canvas = this.canvasRef.nativeElement;
    const rect = canvas.getBoundingClientRect();
    const ratio = (event.clientX - rect.left) / rect.width;
    const newTime = ratio * this.audioBuffer.duration;
    const wasPlaying = this.playing();
    if (wasPlaying) this.pause();
    this.offsetAt = newTime;
    this.currentTime.set(newTime);
    this.updatePlayheadLine(newTime);
    if (wasPlaying) this.play();
  }

  private play() {
    if (!this.audioCtx || !this.audioBuffer) return;
    this.source = this.audioCtx.createBufferSource();
    this.source.buffer = this.audioBuffer;
    this.source.connect(this.audioCtx.destination);
    this.startedAt = this.audioCtx.currentTime - this.offsetAt;
    this.source.start(0, this.offsetAt);
    this.source.onended = () => {
      if (this.playing()) {
        this.zone.run(() => {
          this.source = null;
          this.playing.set(false);
          this.offsetAt = 0;
          this.currentTime.set(0);
          cancelAnimationFrame(this.rafId);
          this.hidePlayheadLine();
        });
      }
    };
    this.playing.set(true);
    this.tick();
  }

  private pause() {
    if (!this.audioCtx) return;
    this.offsetAt = this.audioCtx.currentTime - this.startedAt;
    if (this.source) {
      this.source.onended = null;
      try {
        this.source.stop();
      } catch {
        /* already ended */
      }
      this.source = null;
    }
    this.playing.set(false);
    cancelAnimationFrame(this.rafId);
  }

  private stop() {
    if (this.playing()) this.pause();
    this.offsetAt = 0;
    this.currentTime.set(0);
    cancelAnimationFrame(this.rafId);
  }

  private tick() {
    this.rafId = requestAnimationFrame(() => {
      if (!this.playing() || !this.audioCtx) return;
      const t = this.audioCtx.currentTime - this.startedAt;
      this.updatePlayheadLine(t);
      this.zone.run(() => {
        this.currentTime.set(t);
      });
      this.tick();
    });
  }

  formatTime(s: number): string {
    const m = Math.floor(s / 60);
    const sec = Math.floor(s % 60);
    return `${String(m)}:${sec.toString().padStart(2, '0')}`;
  }
}
