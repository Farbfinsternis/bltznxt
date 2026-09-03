const { app, BrowserWindow } = require('electron');
const path = require('path');

const backend = require('./electron/backend');

function createWindow() {
	const win = new BrowserWindow({
		width: 1000,
		height: 800,
		webPreferences: {
			// Der Renderer bekommt kein Node. Alles, was das Betriebssystem
			// berührt, läuft im Main-Prozess und wird über electron/preload.js
			// als schmale API freigegeben — siehe src/platform/index.js.
			nodeIntegration: false,
			contextIsolation: true,
			preload: path.join(__dirname, 'electron', 'preload.js'),
			// Unverändert aus der ersten Fassung übernommen: im Build lädt die
			// App über file://, wo manche Browser Worker blockieren — Monaco
			// braucht sie. Sollte fallen, sobald der gepackte Build getestet
			// ist; mit contextIsolation ist der Renderer ohnehin abgeschottet.
			webSecurity: false
		}
	});

	// Prüfen, ob wir im Dev-Modus sind (Vite Server läuft)
	const isDev = process.env.NODE_ENV === 'development';

	if (isDev) {
		// Vite Standard-Port ist 5173
		win.loadURL('http://localhost:5173');
		win.webContents.openDevTools(); // DevTools automatisch öffnen
	} else {
		// Im Build-Modus die Datei laden
		win.loadFile(path.join(__dirname, 'dist/index.html'));
	}
}

app.whenReady().then(() => {
	backend.register();
	createWindow();
});

app.on('window-all-closed', () => {
	if (process.platform !== 'darwin') app.quit();
});
