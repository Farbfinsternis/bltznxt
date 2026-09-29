// Dienst "shell": Dateien im Standardprogramm des Systems öffnen.
//
// Nur für Medien (Bilder, Klänge, Modelle) — das, wofür die Original-IDE den
// `mediaview.exe` startet. Ausführbare Dateien und Skripte öffnet der Dienst
// nie, auch wenn der Renderer darum bittet: ein Klick in der Dateiliste darf
// kein Programm starten.

'use strict';

const { shell } = require('electron');
const path = require('path');

const MEDIA = new Set([
	// Bilder
	'.png', '.jpg', '.jpeg', '.bmp', '.tga', '.gif', '.pcx', '.iff', '.webp',
	// Klänge und Musik
	'.wav', '.mp3', '.ogg', '.mid', '.midi', '.mod', '.s3m', '.xm', '.it', '.rmi', '.sgt', '.flac',
	// Modelle
	'.x', '.3ds', '.md2', '.b3d', '.glb', '.gltf'
]);

/** @returns {Promise<{ opened: boolean, error?: string }>} */
async function openExternal(file) {
	if (typeof file !== 'string' || !MEDIA.has(path.extname(file).toLowerCase())) {
		return { opened: false, error: 'not a media file' };
	}
	const error = await shell.openPath(path.resolve(file));
	return error ? { opened: false, error } : { opened: true };
}

module.exports = { api: { openExternal }, MEDIA };
