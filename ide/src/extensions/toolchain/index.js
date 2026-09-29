// Erweiterung "toolchain": der Compiler als Dienst.
//
// Dünne Schicht über dem Backend-Dienst "toolchain": sie reicht den Pfad aus
// den Einstellungen mit und hält den Zustand ("gefunden?") im Kontext, damit
// Menüs und Ansichten darauf reagieren können. Sie kennt den Compiler nur als
// Kommandozeile — was ein Aufruf bedeutet, steht in electron/services/toolchain.js.
//
// Später kann eine zweite Erweiterung einen anderen Dienst "toolchain" liefern
// (anderer Compiler, anderes Protokoll), solange sie dieselben Methoden bietet.
//
// Dienst "toolchain":
//   info                 letzter Stand von getInfo() (oder null)
//   refresh()            Compiler neu suchen; meldet 'toolchain:changed'
//   getInfo()            { available, path, source, version }
//   listCommands()       [{ name, signature, params }]
//   compile(file, opts)  siehe electron/services/toolchain.js
//
// Kontext: toolchain.available
// Ereignis: toolchain:changed

const NO_COMPILER = { path: null, source: 'none', available: false, version: null };

export default {
	id: 'toolchain',

	contributes: {
		settings: {
			'toolchain.compilerPath': {
				type: 'string',
				default: '',
				description: 'Path to blitzcc. Empty: search BLITZPATH, PATH and the development folder.'
			}
		},
		statusItems: [{ id: 'toolchain', align: 'right', priority: 10, tooltip: 'toolchain.tooltip' }]
	},

	messages: {
		en: {
			'toolchain.tooltip': 'Compiler',
			'toolchain.found': 'blitzcc {version}',
			'toolchain.foundNoVersion': 'blitzcc',
			'toolchain.missing': 'blitzcc not found'
		}
	},

	activate(ctx) {
		const { platform, settings, context } = ctx;
		let info = null;

		const options = () => ({ compilerPath: settings.get('toolchain.compilerPath') || undefined });

		async function getInfo() {
			if (!platform.hasBackend) return NO_COMPILER;
			return platform.invoke('toolchain', 'getInfo', options());
		}

		async function refresh() {
			try {
				info = await getInfo();
			} catch (err) {
				console.error('[toolchain] Compiler nicht erreichbar:', err);
				info = NO_COMPILER;
			}
			context.set('toolchain.available', info.available);
			const statusbar = ctx.services.tryGet('statusbar');
			if (statusbar) {
				statusbar.setText(
					'toolchain',
					info.available
						? info.version ? ctx.i18n.t('toolchain.found', { version: 'v' + info.version }) : ctx.i18n.t('toolchain.foundNoVersion')
						: ctx.i18n.t('toolchain.missing'),
					info.path || ''
				);
			}
			ctx.events.emit('toolchain:changed', info);
			return info;
		}

		ctx.services.provide('toolchain', {
			get info() { return info; },
			refresh,
			getInfo,
			async listCommands() {
				if (!platform.hasBackend) return [];
				return platform.invoke('toolchain', 'listCommands', options());
			},
			async compile(file, opts = {}) {
				return platform.invoke('toolchain', 'compile', file, { ...opts, ...options() });
			}
		});

		context.set('toolchain.available', false);

		// Ein anderer Compilerpfad heißt: neu suchen, Befehlsliste neu holen
		ctx.subscriptions.add(
			settings.onDidChange((e) => {
				if (e.key === 'toolchain.compilerPath') refresh();
			})
		);

		// Nicht abwarten — die Suche nach dem Compiler darf den Start nicht aufhalten.
		refresh();
	}
};
