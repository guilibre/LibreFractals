import { Component, inject, signal, output, OnInit } from '@angular/core';
import { GalleryService, GalleryEntry } from './gallery.service';
import { GalleryEntryComponent } from './gallery-entry.component';

@Component({
  selector: 'app-gallery',
  imports: [GalleryEntryComponent],
  templateUrl: './gallery.component.html',
  styleUrl: './gallery.component.scss',
})
export class GalleryComponent implements OnInit {
  load = output<string>();

  private galleryService = inject(GalleryService);

  entries = signal<GalleryEntry[]>([]);
  loading = signal(false);
  hasMore = signal(false);
  error = signal('');

  private page = 0;
  private readonly limit = 20;

  ngOnInit(): void {
    void this.loadPage();
  }

  async loadMore() {
    this.page++;
    await this.loadPage();
  }

  async refresh() {
    this.page = 0;
    this.entries.set([]);
    await this.loadPage();
  }

  private async loadPage() {
    this.loading.set(true);
    this.error.set('');
    try {
      const result = await this.galleryService.getPage(this.page, this.limit);
      this.entries.update((prev) => [...prev, ...result.entries]);
      this.hasMore.set(this.entries().length < result.total);
    } catch (e) {
      this.error.set(String(e));
    } finally {
      this.loading.set(false);
    }
  }
}
