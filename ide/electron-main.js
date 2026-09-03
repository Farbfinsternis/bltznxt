const { app, BrowserWindow } = require('electron');
const path = require('path');

function createWindow() {
const win = new BrowserWindow({
	width: 1000,
	height: 800,
	webPreferences: {
	nodeIntegration: true,
	contextIsolation: false, // Für den Anfang einfacher
	webSecurity: false // Erlaubt das Laden lokaler Ressourcen im Dev-Modus
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

app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
if (process.platform !== 'darwin') app.quit();
});
