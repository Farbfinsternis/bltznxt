// Namen der Kanäle zwischen Main-Prozess und Renderer — an einer Stelle, damit
// Brücke, Vorlader und Dienste, die selbst Ereignisse senden, denselben nehmen.

'use strict';

module.exports = {
	INVOKE: 'bltznxt:invoke',
	EVENT: 'bltznxt:event'
};
