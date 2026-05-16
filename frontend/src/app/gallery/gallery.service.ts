import { Injectable } from '@angular/core';
import { environment } from '../../environments/environment';

export interface GalleryEntry {
  hash: string;
  name: string;
  svg_bytes: number;
  created_at: number;
}

export interface GalleryPage {
  entries: GalleryEntry[];
  total: number;
}

export interface RelativeEntry {
  hash: string;
  similarity: number;
  relation: 'duplicate' | 'derived' | 'similar';
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

  async add(hash: string, name: string, svgBytes: number, topo: string): Promise<void> {
    const res = await fetch(`${environment.backendUrl}/gallery`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ hash, name, svg_bytes: svgBytes, topo }),
    });
    if (!res.ok) throw new Error(`Gallery add failed: ${String(res.status)}`);
  }

  async getRelatives(hash: string): Promise<RelativeEntry[]> {
    const res = await fetch(
      `${environment.backendUrl}/gallery/${encodeURIComponent(hash)}/relatives`,
    );
    if (!res.ok) throw new Error(`Relatives fetch failed: ${String(res.status)}`);
    const data = (await res.json()) as { relatives: RelativeEntry[] };
    return data.relatives;
  }
}
