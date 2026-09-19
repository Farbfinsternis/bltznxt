// Backend im Electron-Main-Prozess: alles, was das Betriebssystem berührt.
//
// Das ist die einzige Stelle der IDE, die blitzcc kennt — und sie kennt ihn
// ausschließlich als Prozess: Argumente rein, stdout/stderr und Exit-Code
// raus. Kein Include, kein Linken, kein Lesen von src/compiler/.
// Siehe README.md, Abschnitt "Repository Layout".

'use strict';

const { ipcMain, app } = require('electron');
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');

const { parseDiagnostics, isSourceDiagnostic, statusFromExitCode } = require('./diagnostics');

const EXE = process.platform === 'win32' ? 'blitzcc.exe' : 'blitzcc';

// ---------------------------------------------------------------------------
// Einstellungen (nur der Compiler-Pfad, mehr braucht es hier noch nicht)
// ---------------------------------------------------------------------------

function settingsFile() {
	return path.join(app.getPath('userData'), 'settings.json');
}

function readSettings() {
	try {
		return JSON.parse(fs.readFileSync(settingsFile(), 'utf8'));
	} catch {
		return {};
	}
}

function writeSettings(next) {
	const file = settingsFile();
	fs.mkdirSync(path.dirname(file), { recursive: true });
	fs.writeFileSync(file, JSON.stringify(next, null, 2), 'utf8');
}

// ---------------------------------------------------------------------------
// Compiler finden
// ---------------------------------------------------------------------------
//
// Reihenfolge: IDE-Einstellung -> BLITZPATH -> PATH -> Entwickler-Fallback.
//
// Der Fallback auf ../bin/ ist reine Bequemlichkeit für das Monorepo und darf
// nie Voraussetzung sein: die IDE muss gegen eine beliebig installierte
// BlitzNext-Version laufen und auch ganz ohne Compiler starten.

function isExecutable(p) {
	try {
		return fs.statSync(p).isFile();
	} catch {
		return false;
	}
}

function fromPathEnv() {
	const dirs = String(process.env.PATH || '').split(path.delimiter);
	for (const dir of dirs) {
		if (!dir) continue;
		const candidate = path.join(dir, EXE);
		if (isExecutable(candidate)) return candidate;
	}
	return null;
}

function devFallback() {
	// __dirname ist ide/electron/ — zwei Ebenen höher liegt im Monorepo das
	// bin/ des Compilers. Bewusst nicht app.getAppPath(): das zeigt je nach
	// Startart woanders hin. Im gepackten Build liegt __dirname in der asar,
	// der Kandidat existiert dann nicht und der Fallback greift korrekt nicht.
	const candidate = path.resolve(__dirname, '..', '..', 'bin', EXE);
	return isExecutable(candidate) ? candidate : null;
}

function resolveCompiler() {
	const configured = readSettings().compilerPath;
	if (configured) {
		return {
			path: configured,
			source: 'setting',
			available: isExecutable(configured)
		};
	}

	const bp = process.env.BLITZPATH;
	if (bp) {
		for (const candidate of [path.join(bp, 'bin', EXE), path.join(bp, EXE)]) {
			if (isExecutable(candidate)) {
				return { path: candidate, source: 'BLITZPATH', available: true };
			}
		}
	}

	const onPath = fromPathEnv();
	if (onPath) return { path: onPath, source: 'PATH', available: true };

	const dev = devFallback();
	if (dev) return { path: dev, source: 'dev-fallback', available: true };

	return { path: null, source: 'none', available: false };
}

// ---------------------------------------------------------------------------
// Prozessaufruf
// ---------------------------------------------------------------------------

/**
 * Startet blitzcc und sammelt die komplette Ausgabe ein.
 * Lehnt nie ab — ein fehlgeschlagener Start kommt als Ergebnis zurück,
 * damit der Renderer nur einen Fehlerpfad behandeln muss.
 */
function runCompiler(exe, args, cwd) {
	return new Promise((resolve) => {
		let child;
		try {
			child = spawn(exe, args, { cwd, windowsHide: true });
		} catch (err) {
			resolve({ exitCode: -1, stdout: '', stderr: String(err.message), spawnError: true });
			return;
		}

		let stdout = '';
		let stderr = '';
		child.stdout.on('data', (d) => { stdout += d.toString(); });
		child.stderr.on('data', (d) => { stderr += d.toString(); });

		child.on('error', (err) => {
			resolve({ exitCode: -1, stdout, stderr: stderr + String(err.message), spawnError: true });
		});
		child.on('close', (code) => {
			resolve({ exitCode: code === null ? -1 : code, stdout, stderr, spawnError: false });
		});
	});
}

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

async function getInfo() {
	const found = resolveCompiler();
	if (!found.available) {
		return { ...found, version: null };
	}

	// -v gibt "BlitzNext v0.5.5" aus.
	const res = await runCompiler(found.path, ['-v'], path.dirname(found.path));
	const m = /v?(\d+\.\d+\.\d+)/.exec(res.stdout);
	return { ...found, version: m ? m[1] : null };
}

// Die Kommandoliste wird bewusst zur Laufzeit beim Compiler erfragt und nicht
// in der IDE eingefroren — sonst veraltet die Autovervollständigung still,
// sobald der Compiler neue Befehle bekommt.
let commandCache = null; // { key, commands }

async function listCommands() {
	const found = resolveCompiler();
	if (!found.available) return [];

	let key = found.path;
	try {
		key += ':' + fs.statSync(found.path).mtimeMs;
	} catch {
		/* ohne mtime cachen wir nur über den Pfad */
	}
	if (commandCache && commandCache.key === key) return commandCache.commands;

	// +k liefert je Zeile "Name(param1, param2)"
	const res = await runCompiler(found.path, ['+k'], path.dirname(found.path));
	if (res.exitCode !== 0) return [];

	const commands = [];
	for (const line of res.stdout.split(/\r?\n/)) {
		const m = /^([A-Za-z_][A-Za-z0-9_]*)\((.*)\)\s*$/.exec(line.trim());
		if (!m) continue;
		const [, name, sig] = m;
		commands.push({
			name,
			signature: sig,
			params: sig ? sig.split(',').map((s) => s.trim()) : []
		});
	}

	commandCache = { key, commands };
	return commands;
}

/**
 * Kompiliert eine .bb-Datei.
 *
 * @param {string} file  absoluter Pfad zur Quelldatei
 * @param {object} opts  { debug, transpileOnly, outputName }
 */
async function compile(file, opts = {}) {
	const found = resolveCompiler();
	if (!found.available) {
		return {
			status: 'no-compiler',
			exitCode: -1,
			diagnostics: [],
			toolchain: [],
			unparsed: [],
			stdout: '',
			stderr: 'blitzcc nicht gefunden. Pfad in den Einstellungen setzen oder BLITZPATH belegen.',
			outputPath: null
		};
	}

	const args = ['-q'];
	if (opts.debug) args.push('-d');
	if (opts.transpileOnly) args.push('-c');
	if (opts.outputName) args.push('-o', opts.outputName);
	args.push(file);

	// Im Verzeichnis der Quelldatei starten: blitzcc legt die .exe daneben ab
	// und löst relative Pfade (#Include, Assets) von dort auf.
	const cwd = path.dirname(file);
	const res = await runCompiler(found.path, args, cwd);
	const parsed = parseDiagnostics(res.stderr);

	// Quelldiagnosen (.bb) dürfen als Editor-Marker gesetzt werden, Toolchain-
	// Diagnosen (.cpp aus g++) nicht — deren Zeilennummern gehören zur
	// generierten Datei. Beide getrennt zurückgeben, damit die Oberfläche sie
	// nicht verwechseln kann.
	const diagnostics = parsed.diagnostics.filter(isSourceDiagnostic);
	const toolchain = parsed.diagnostics.filter((d) => !isSourceDiagnostic(d));
	const unparsed = parsed.unparsed;

	let outputPath = null;
	if (res.exitCode === 0 && !opts.transpileOnly) {
		const base = opts.outputName || path.basename(file, path.extname(file));
		outputPath = path.join(cwd, base + (process.platform === 'win32' ? '.exe' : ''));
	}

	return {
		status: res.spawnError ? 'failed' : statusFromExitCode(res.exitCode),
		exitCode: res.exitCode,
		diagnostics,
		toolchain,
		unparsed,
		stdout: res.stdout,
		stderr: res.stderr,
		outputPath
	};
}

function setCompilerPath(p) {
	const next = readSettings();
	if (p) next.compilerPath = p; else delete next.compilerPath;
	writeSettings(next);
	commandCache = null; // anderer Compiler, andere Kommandoliste
	return resolveCompiler();
}

// ---------------------------------------------------------------------------
// IPC-Registrierung
// ---------------------------------------------------------------------------

function register() {
	ipcMain.handle('bltznxt:getInfo', () => getInfo());
	ipcMain.handle('bltznxt:listCommands', () => listCommands());
	ipcMain.handle('bltznxt:compile', (_e, file, opts) => compile(file, opts));
	ipcMain.handle('bltznxt:setCompilerPath', (_e, p) => setCompilerPath(p));
}

module.exports = {
	register,
	// exportiert für Tests und für den Fall, dass das Backend einmal ohne
	// Electron laufen soll (z.B. Headless-Prüfung der Compiler-Anbindung)
	getInfo,
	listCommands,
	compile,
	setCompilerPath,
	resolveCompiler
};
