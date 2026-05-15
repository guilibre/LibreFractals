import { Component, input } from '@angular/core';
import { SafeHtml } from '@angular/platform-browser';

@Component({
  selector: 'app-svg-viewer',
  templateUrl: './svg-viewer.html',
  styleUrl: './svg-viewer.scss',
})
export class SvgViewerComponent {
  readonly svg = input<SafeHtml | string>('');
}
