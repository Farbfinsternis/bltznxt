// Erweiterung "language-blitz": die Sprache Blitz3D für den Editor.
//
// Färbung und Vervollständigung. Die Liste der eingebauten Befehle kommt vom
// Compiler (Dienst "toolchain") und wird bei jedem Start und nach jedem
// Wechsel des Compilers neu geholt — nie hier einkopiert.
//
// Das ist heute die einzige Sprache; eine weitere (etwa für Blitz-Includes
// oder Shader) wäre eine weitere Erweiterung dieser Form.

import { install, setCommands } from './blitz3d.js';

export default {
	id: 'language-blitz',
	dependsOn: ['toolchain'],

	activate(ctx) {
		ctx.subscriptions.add(install());

		const refresh = async () => {
			try {
				const toolchain = ctx.services.get('toolchain');
				const count = setCommands(await toolchain.listCommands());
				console.info(`[language-blitz] ${count} eingebaute Befehle geladen.`);
			} catch (err) {
				console.warn('[language-blitz] Befehlsliste nicht verfügbar:', err.message);
			}
		};

		// Nicht abwarten: die Aktivierung soll nicht am Compiler hängen.
		refresh();
		ctx.subscriptions.add(ctx.events.on('toolchain:changed', refresh));
	}
};
