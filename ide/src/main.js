import './shell/shell.css';
import platform from './platform/index.js';
import { createApp } from './app.js';
import { createShell } from './shell/shell.js';
import { builtinExtensions } from './extensions/index.js';

const app = createApp({ platform, extensions: builtinExtensions });
const shell = createShell({ root: document.querySelector('#app'), app });

// Zum Untersuchen in den DevTools und für die Oberflächentests (test/ui-smoke.js)
window.__ide = { app, shell };

app.start().then(() => {
	// Vorläufig, bis die Dateierweiterung (P1) Start und Wiederherstellen regelt:
	// ein namenloses Dokument, damit der Editor etwas zeigt.
	if (app.documents.list().length === 0) {
		app.documents.open({
			text: `; Beispiel für Blitz3D Code mit Keywords und Commands
Function Main()
	Graphics 800, 600 ; Das ist ein Command
	Print "Hallo Welt aus meiner IDE!"
End Function
`
		});
	}
	shell.render();
});

window.addEventListener('beforeunload', () => {
	app.stop();
});
