import './style.css';
import * as monaco from 'monaco-editor';
import './userWorker'; // Importiert unsere Worker-Konfiguration
import './blitz3d'; // Importiert unsere neue Sprachdefinition

const app = document.querySelector('#app');

// Editor erstellen
const editor = monaco.editor.create(app, {
	value: `; Beispiel für Blitz3D Code mit Keywords und Commands
Function Main()
	Graphics 800, 600 ; Das ist ein Command
	Print "Hallo Welt aus meiner IDE!"
End Function`,
	language: 'blitz3d', // Unsere neue Sprache verwenden
	theme: 'vs-dark',
	automaticLayout: true
});
