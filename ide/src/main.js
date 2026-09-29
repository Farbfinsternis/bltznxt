import './shell/shell.css';
import platform from './platform/index.js';
import { createApp } from './app.js';
import { createShell } from './shell/shell.js';
import { builtinExtensions } from './extensions/index.js';

const app = createApp({ platform, extensions: builtinExtensions });
const shell = createShell({ root: document.querySelector('#app'), app });

// Zum Untersuchen in den DevTools und für die Oberflächentests (test/ui-smoke.js)
window.__ide = { app, shell };

app.start().then(() => shell.render());

window.addEventListener('beforeunload', () => {
	app.stop();
});
