const { app, BrowserWindow, Menu, ipcMain } = require('electron');
const path = require('path');

const bridge = require('./electron/bridge');
const store = require('./electron/services/store');

function createWindow() {
	const win = new BrowserWindow({
		width: 1100,
		height: 800,
		backgroundColor: '#1e1e1e',
		webPreferences: {
			// Der Renderer bekommt kein Node. Alles, was das Betriebssystem
			// berührt, läuft im Main-Prozess und wird über electron/preload.js
			// und electron/bridge.js als schmale API freigegeben — siehe
			// src/platform/index.js.
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
	// Die Menüleiste zeichnet die IDE selbst (src/shell/menubar.js) aus den
	// Beiträgen der Erweiterungen; das Standardmenü von Electron stört nur.
	Menu.setApplicationMenu(null);

	store.setBaseDir(app.getPath('userData'));
	bridge.register(ipcMain);
	createWindow();
});

app.on('window-all-closed', () => {
	if (process.platform !== 'darwin') app.quit();
});
