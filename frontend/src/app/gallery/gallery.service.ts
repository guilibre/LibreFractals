import { Injectable } from '@angular/core';
import { environment } from '../../environments/environment';

export interface GalleryEntry {
  hash: string;
  name: string;
  compile_ms: number;
  created_at: number;
}

export interface GalleryPage {
  entries: GalleryEntry[];
  total: number;
}

@Injectable({ providedIn: 'root' })
export class GalleryService {
  async getPage(page = 0, limit = 20): Promise<GalleryPage> {
    const res = await fetch(
      `${environment.backendUrl}/gallery?page=${String(page)}&limit=${String(limit)}`,
    );
    if (!res.ok) throw new Error(`Gallery fetch failed: ${String(res.status)}`);
    return res.json() as Promise<GalleryPage>;
  }

  async add(hash: string, name: string, compileMs: number): Promise<void> {
    const res = await fetch(`${environment.backendUrl}/gallery`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ hash, name, compile_ms: compileMs }),
    });
    if (!res.ok) throw new Error(`Gallery add failed: ${String(res.status)}`);
  }
}
