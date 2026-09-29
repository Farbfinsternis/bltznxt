// Erweiterung "workbench": das Gerüst der Menüleiste und was zum Fenster selbst gehört.
//
//   * legt die Menüs der Original-IDE an (File, Edit, Program, Help) — leer, die
//     Erweiterungen mit den passenden Befehlen füllen sie; ein Menü ohne
//     Einträge bleibt unsichtbar
//   * "Show Toolbars" (Umschalt+Esc): Symbol- und Statusleiste ein/aus, wie im Original
//   * "About"
//   * Sprache der Oberfläche als Einstellung
//   * meldet eine unlesbare Einstellungsdatei

import pkg from '../../../package.json';

export default {
	id: 'workbench',

	messages: {
		en: {
			'menu.file': 'File',
			'menu.edit': 'Edit',
			'menu.program': 'Program',
			'menu.help': 'Help',

			'workbench.toggleToolbars': 'Show Toolbars',
			'help.about': 'About BLTZNXT',
			'help.about.title': 'About BLTZNXT IDE',
			'help.about.text': 'BLTZNXT IDE {version}\nCompiler: {compiler}',
			'help.about.noCompiler': 'not found',

			'error.title': 'Error',
			'error.command': 'The command "{id}" failed:\n{message}',
			'tab.close': 'Close',

			'settings.corrupt.title': 'Settings file unreadable',
			'settings.corrupt.text':
				'The settings file could not be read ({error}).\nA copy was saved as {backup}; the IDE starts with default settings.',
			'button.ok': 'OK'
		}
	},

	contributes: (ctx) => ({
		settings: {
			'workbench.showToolbars': {
				type: 'boolean',
				default: true,
				description: 'Show the toolbar and the status bar.'
			},
			'workbench.language': {
				type: 'string',
				default: 'en',
				description: 'Language of the user interface.'
			}
		},

		commands: [
			{
				id: 'workbench.toggleToolbars',
				title: 'workbench.toggleToolbars',
				run: () => ctx.settings.set('workbench.showToolbars', !ctx.settings.get('workbench.showToolbars'))
			},
			{
				id: 'help.about',
				title: 'help.about',
				run: async () => {
					const { i18n } = ctx;
					const info = ctx.services.tryGet('toolchain')?.info;
					await ctx.services.get('dialogs').message({
						title: i18n.t('help.about.title'),
						text: i18n.t('help.about.text', {
							version: 'v' + pkg.version,
							compiler: info && info.available
								? `${info.version ? 'v' + info.version + ' — ' : ''}${info.path}`
								: i18n.t('help.about.noCompiler')
						}),
						buttons: [{ id: 'ok', label: i18n.t('button.ok'), default: true }]
					});
				}
			}
		],

		// Reihenfolge der Menüs wie in der Original-IDE
		menubar: [
			{ id: 'file', title: 'menu.file', order: 10 },
			{ id: 'edit', title: 'menu.edit', order: 20 },
			{ id: 'program', title: 'menu.program', order: 30 },
			{ id: 'help', title: 'menu.help', order: 90 }
		],

		menus: [
			{ menu: 'edit', command: 'workbench.toggleToolbars', group: '9_view', checkedWhen: 'workbench.toolbarsVisible' },
			{ menu: 'help', command: 'help.about', group: '9_about' }
		],

		keybindings: [{ command: 'workbench.toggleToolbars', key: 'Shift+Escape' }]
	}),

	activate(ctx) {
		const { settings, context, i18n } = ctx;

		const syncToolbars = () => context.set('workbench.toolbarsVisible', settings.get('workbench.showToolbars'));
		syncToolbars();
		ctx.subscriptions.add(
			settings.onDidChange((e) => {
				if (e.key === 'workbench.showToolbars') syncToolbars();
				if (e.key === 'workbench.language') i18n.setLanguage(e.value);
			})
		);
		i18n.setLanguage(settings.get('workbench.language'));

		ctx.subscriptions.add(
			ctx.events.on('settings:corrupt', async ({ error, backup }) => {
				const dialogs = await ctx.services.waitFor('dialogs');
				dialogs.message({
					title: i18n.t('settings.corrupt.title'),
					text: i18n.t('settings.corrupt.text', { error, backup }),
					buttons: [{ id: 'ok', label: i18n.t('button.ok'), default: true }]
				});
			})
		);
	}
};
