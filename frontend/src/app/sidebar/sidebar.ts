import { Component, input, output, signal } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { EditorComponent } from '../editor/editor';
import song from '../examples/song.frac?raw';
import stochastic_tree from '../examples/stochastic_tree.frac?raw';
import koch_curve from '../examples/koch_curve.frac?raw';
import koch_snowflake from '../examples/koch_snowflake.frac?raw';
import sierpinski from '../examples/sierpinski.frac?raw';
import dragon from '../examples/dragon.frac?raw';
import plant from '../examples/plant.frac?raw';

const EXAMPLES: { label: string; source: string }[] = [
  { label: 'Song', source: song },
  { label: 'Stochastic tree', source: stochastic_tree },
  { label: 'Koch curve', source: koch_curve },
  { label: 'Koch snowflake', source: koch_snowflake },
  { label: 'Sierpiński', source: sierpinski },
  { label: 'Dragon', source: dragon },
  { label: 'Plant', source: plant },
];

@Component({
  selector: 'app-sidebar',
  imports: [EditorComponent, FormsModule],
  templateUrl: './sidebar.html',
  styleUrl: './sidebar.scss',
})
export class SidebarComponent {
  readonly source = input.required<string>();
  readonly loading = input(false);
  readonly audioLoading = input(false);
  readonly galleryAdding = input(false);
  readonly error = input('');

  readonly sourceChange = output<string>();
  readonly render = output();
  readonly listen = output();
  readonly addToGallery = output<string>();
  readonly toggleGallery = output();

  readonly examples = EXAMPLES;

  showGalleryInput = signal(false);
  galleryName = signal('');

  loadExample(source: string) {
    this.sourceChange.emit(source);
  }

  submitGallery() {
    this.addToGallery.emit(this.galleryName());
    this.galleryName.set('');
    this.showGalleryInput.set(false);
  }
}
