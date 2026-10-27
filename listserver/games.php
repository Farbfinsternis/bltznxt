<?php
// Listen-Server fuer die Spielsuche von BLTZNXT, fuer Webspace mit PHP 7.4+
// und SQLite (PDO). Gleiche Schnittstelle wie listserver.js; Beschreibung in
// README.md daneben und in NETWORK.md, Abschnitt Listen-Server.
//
// Hochladen, im Spiel   NetGameServer "https://example.org/games.php"   setzen,
// fertig. Die Datenbank games.sqlite legt das Skript selbst an, im selben
// Ordner (der Ordner muss fuer PHP beschreibbar sein; DB_FILE aendert den Ort).
//
//   GET  ?a=list&app=<id>&v=<version>   name<TAB>ip<TAB>port<TAB>players<TAB>token
//   POST a=host&app=&name=&port=&players=&v=&token=    ok <key>   | err <grund>
//   POST a=beat&key=&players=                          ok         | err gone
//   POST a=bye&key=                                    ok

const DB_FILE    = __DIR__ . '/games.sqlite';
const EXPIRE     = 90;     // Sekunden ohne Lebenszeichen, dann faellt ein Eintrag weg
const MAX_PER_IP = 8;      // Eintraege je Absender-IP
const MAX_TEXT   = 64;     // Laenge von Name und Kennung
const RATE       = 60;     // Anfragen je IP und Minute
const TRUST_PROXY = false; // true: Absender-IP aus X-Forwarded-For (nur hinter eigenem Proxy)

header('Content-Type: text/plain; charset=utf-8');
header('Cache-Control: no-store');

function clean($t) {
    $t = trim(preg_replace('/\s+/u', ' ', (string)$t));
    return mb_substr($t, 0, MAX_TEXT, 'UTF-8');
}

function number($t, $low, $high, $default) {
    if (!preg_match('/^\d+$/', (string)$t)) return $default;
    $v = (int)$t;
    return ($v >= $low && $v <= $high) ? $v : $default;
}

function param($k) {
    if (isset($_POST[$k])) return (string)$_POST[$k];
    if (isset($_GET[$k])) return (string)$_GET[$k];
    return '';
}

$ip = $_SERVER['REMOTE_ADDR'] ?? '';
if (TRUST_PROXY && !empty($_SERVER['HTTP_X_FORWARDED_FOR']))
    $ip = trim(explode(',', $_SERVER['HTTP_X_FORWARDED_FOR'])[0]);
if (strpos($ip, '::ffff:') === 0) $ip = substr($ip, 7);

try {
    $db = new PDO('sqlite:' . DB_FILE);
    $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    $db->exec('PRAGMA busy_timeout = 3000');
    $db->exec('CREATE TABLE IF NOT EXISTS games (
        gkey TEXT PRIMARY KEY, app TEXT, name TEXT, ip TEXT, port INTEGER,
        players INTEGER, v TEXT, token INTEGER, seen INTEGER)');
    $db->exec('CREATE TABLE IF NOT EXISTS hits (ip TEXT, t INTEGER)');
} catch (Exception $e) {
    echo "err database\n";
    exit;
}

$now = time();
$db->prepare('DELETE FROM games WHERE seen < ?')->execute([$now - EXPIRE]);
$db->prepare('DELETE FROM hits WHERE t < ?')->execute([$now - 60]);
$db->prepare('INSERT INTO hits (ip, t) VALUES (?, ?)')->execute([$ip, $now]);
$q = $db->prepare('SELECT COUNT(*) FROM hits WHERE ip = ?');
$q->execute([$ip]);
if ((int)$q->fetchColumn() > RATE) { echo "err busy\n"; exit; }

switch (param('a')) {
case 'list':
    $app = mb_strtolower(clean(param('app')), 'UTF-8');
    $v = param('v');
    $q = $db->prepare('SELECT name, ip, port, players, token FROM games WHERE app = ? AND (? = \'\' OR v = ?)');
    $q->execute([$app, $v, $v]);
    foreach ($q->fetchAll(PDO::FETCH_NUM) as $row) echo implode("\t", $row), "\n";
    break;

case 'host':
    $port = number(param('port'), 1, 65535, null);
    if ($port === null) { echo "err port\n"; break; }
    $q = $db->prepare('SELECT COUNT(*) FROM games WHERE ip = ?');
    $q->execute([$ip]);
    if ((int)$q->fetchColumn() >= MAX_PER_IP) { echo "err limit\n"; break; }
    $key = bin2hex(random_bytes(16));
    $db->prepare('INSERT INTO games (gkey, app, name, ip, port, players, v, token, seen) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)')
       ->execute([$key, mb_strtolower(clean(param('app')), 'UTF-8'), clean(param('name')), $ip, $port,
                  number(param('players'), 0, 65535, 0), clean(param('v')),
                  number(param('token'), 0, 4294967295, 0), $now]);
    echo "ok $key\n";
    break;

case 'beat':
    $players = number(param('players'), 0, 65535, null);
    $q = $db->prepare('UPDATE games SET seen = ?, players = COALESCE(?, players) WHERE gkey = ?');
    $q->execute([$now, $players, param('key')]);
    echo $q->rowCount() ? "ok\n" : "err gone\n";
    break;

case 'bye':
    $db->prepare('DELETE FROM games WHERE gkey = ?')->execute([param('key')]);
    echo "ok\n";
    break;

default:
    echo "err action\n";
}
