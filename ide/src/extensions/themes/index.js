// Erweiterung "themes": die mitgelieferten Farbschemata.
//
// Nur Daten — ein Beitrag zum Punkt "themes" (Form siehe core/themes.js). Wer
// ein eigenes Theme möchte, trägt einen weiteren Eintrag bei; ausgewählt wird
// über die Einstellung `workbench.theme`.

export default {
	id: 'themes',

	messages: {
		en: {
			'theme.blitzClassic': 'Blitz3D Classic',
			'theme.dark': 'Dark',
			'theme.light': 'Light'
		}
	},

	contributes: {
		themes: [
			{
				// Die Vorgabe der Original-IDE (blitzide/prefs.cpp, setDefault, PRO):
				// rgb_bkgrnd 225588, rgb_string 00ff66, rgb_ident ffffff,
				// rgb_keyword aaffff, rgb_comment ffee00, rgb_digit 33ffdd, rgb_default eeeeee
				id: 'blitz-classic',
				label: 'theme.blitzClassic',
				dark: true,
				editor: {
					background: '#225588',
					default: '#eeeeee',
					identifier: '#ffffff',
					keyword: '#aaffff',
					comment: '#ffee00',
					string: '#00ff66',
					number: '#33ffdd'
				}
			},
			{
				id: 'dark',
				label: 'theme.dark',
				dark: true,
				editor: {
					background: '#1e1e1e',
					default: '#d4d4d4',
					identifier: '#9cdcfe',
					keyword: '#569cd6',
					comment: '#6a9955',
					string: '#ce9178',
					number: '#b5cea8'
				}
			},
			{
				id: 'light',
				label: 'theme.light',
				dark: false,
				editor: {
					background: '#ffffff',
					default: '#000000',
					identifier: '#001080',
					keyword: '#0000ff',
					comment: '#008000',
					string: '#a31515',
					number: '#098658'
				},
				ui: {
					'--bg': '#ffffff',
					'--bg-alt': '#f3f3f3',
					'--bg-raised': '#ffffff',
					'--bg-hover': '#e8e8e8',
					'--bg-active': '#0060c0',
					'--border': '#d4d4d4',
					'--fg': '#333333',
					'--fg-dim': '#6e6e6e',
					'--fg-disabled': '#aaaaaa',
					'--accent': '#007acc',
					'--accent-fg': '#ffffff'
				}
			}
		]
	}
};
