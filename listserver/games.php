<?php
// Listen-Server fuer die Spielsuche von BLTZNXT, fuer Webspace mit PHP 7.4+
// und SQLite (PDO). Gleiche Schnittstelle wie listserver.js; Beschreibung in
// README.md daneben und in NETWORK.md, Abschnitt Listen-Server.
//
// Hochladen, im Spiel   NetGameServer "https://example.org/games.php"   setzen,
// fertig. Die Datenbank games.sqlite legt das Skript selbst an, im selben
// Ordner (der Ordner muss fuer PHP beschreibbar sein; DB_FILE aendert den Ort).
//
//   GET  ?a=list&app=<id>&v=<version>   name<TAB>ip<TAB>port<TAB>players<TAB>token<TAB>ip6
//   POST a=host&app=&name=&port=&players=&v=&token=&ip6=   ok <key>   | err <grund>
//   POST a=beat&key=&players=                              ok         | err gone
//   POST a=bye&key=                                        ok
//   POST a=knock&app=&token=&ip6=&port=                    ok         | err <grund>
//   POST a=knocks&key=                     ok, dann je Klopfer ip6<TAB>port | err gone
//
// Klopfen (NETWORK.md, IPv6 Punkt 7a): ein Client meldet "ich will zu Spiel
// (app, token), von Adresse ip6, Port port"; der Host fragt alle 3 s nach
// (a=knocks, zaehlt auch als Lebenszeichen) und schickt jedem Klopfer ein
// paar Pakete, das oeffnet die Firewall seines Routers fuer diese Adresse.
// Klopfer gelten 30 s, hoechstens 16 je Spiel; die Abfrage leert die Liste.
//
// ip ist die IPv4-Adresse des Hosts, ip6 seine IPv6-Adresse; leer, wo eine
// fehlt (NETWORK.md, IPv6 Punkt 4). Kommt die Anmeldung ueber IPv4, sieht der
// Server die IPv6-Adresse des Hosts nie: der Host nennt sie selbst (ip6=),
// angenommen wird nur eine globale (2000::/3). Kommt die Anmeldung ueber
// IPv6, gilt die Absenderadresse und ip6= nicht. Die Spalte ip haelt immer
// die Absenderadresse (fuer die Grenze je IP), ip6 nur die angegebene.

const DB_FILE    = __DIR__ . '/games.sqlite';
const EXPIRE     = 90;     // Sekunden ohne Lebenszeichen, dann faellt ein Eintrag weg
const MAX_PER_IP = 8;      // Eintraege je Absender-IP
const MAX_TEXT   = 64;     // Laenge von Name und Kennung
const RATE       = 60;     // Anfragen je IP und Minute
const TRUST_PROXY = false; // true: Absender-IP aus X-Forwarded-For (nur hinter eigenem Proxy)
const KNOCK_TTL  = 30;     // Sekunden, die ein Klopfer auf die Abfrage des Hosts wartet
const MAX_KNOCKS = 16;     // offene Klopfer je Spiel

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

function is_ip6($t) {
    return filter_var($t, FILTER_VALIDATE_IP, FILTER_FLAG_IPV6) !== false;
}

// ip6= des Hosts: eine globale IPv6-Adresse (2000::/3), sonst ''
function ip6_param($t) {
    $t = strtolower(trim((string)$t));
    if (!is_ip6($t)) return '';
    $b = inet_pton($t);
    return ($b !== false && (ord($b[0]) & 0xE0) === 0x20) ? inet_ntop($b) : '';
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
    $db->exec('CREATE TABLE IF NOT EXISTS knocks (gkey TEXT, ip6 TEXT, port INTEGER, t INTEGER)');
    // Spalte ip6 (seit Protokoll 3) auch in einer schon bestehenden Datenbank
    $cols = $db->query('PRAGMA table_info(games)')->fetchAll(PDO::FETCH_COLUMN, 1);
    if (!in_array('ip6', $cols, true)) $db->exec("ALTER TABLE games ADD COLUMN ip6 TEXT DEFAULT ''");
} catch (Exception $e) {
    echo "err database\n";
    exit;
}

$now = time();
$db->prepare('DELETE FROM games WHERE seen < ?')->execute([$now - EXPIRE]);
$db->prepare('DELETE FROM hits WHERE t < ?')->execute([$now - 60]);
$db->prepare('DELETE FROM knocks WHERE t < ?')->execute([$now - KNOCK_TTL]);
$db->prepare('INSERT INTO hits (ip, t) VALUES (?, ?)')->execute([$ip, $now]);
$q = $db->prepare('SELECT COUNT(*) FROM hits WHERE ip = ?');
$q->execute([$ip]);
if ((int)$q->fetchColumn() > RATE) { echo "err busy\n"; exit; }

switch (param('a')) {
case 'list':
    $app = mb_strtolower(clean(param('app')), 'UTF-8');
    $v = param('v');
    $q = $db->prepare('SELECT name, ip, port, players, token, ip6 FROM games WHERE app = ? AND (? = \'\' OR v = ?)');
    $q->execute([$app, $v, $v]);
    foreach ($q->fetchAll(PDO::FETCH_NUM) as $row) {
        if (is_ip6($row[1])) { $row[5] = $row[1]; $row[1] = ''; }   // Anmeldung ueber IPv6
        echo implode("\t", [$row[0], $row[1], $row[2], $row[3], $row[4], (string)$row[5]]), "\n";
    }
    break;

case 'host':
    $port = number(param('port'), 1, 65535, null);
    if ($port === null) { echo "err port\n"; break; }
    $q = $db->prepare('SELECT COUNT(*) FROM games WHERE ip = ?');
    $q->execute([$ip]);
    if ((int)$q->fetchColumn() >= MAX_PER_IP) { echo "err limit\n"; break; }
    $key = bin2hex(random_bytes(16));
    $db->prepare('INSERT INTO games (gkey, app, name, ip, port, players, v, token, seen, ip6) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)')
       ->execute([$key, mb_strtolower(clean(param('app')), 'UTF-8'), clean(param('name')), $ip, $port,
                  number(param('players'), 0, 65535, 0), clean(param('v')),
                  number(param('token'), 0, 4294967295, 0), $now,
                  is_ip6($ip) ? '' : ip6_param(param('ip6'))]);
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
    $db->prepare('DELETE FROM knocks WHERE gkey = ?')->execute([param('key')]);
    echo "ok\n";
    break;

case 'knock':
    $token = number(param('token'), 1, 4294967295, 0);
    $q = $db->prepare('SELECT gkey FROM games WHERE app = ? AND token = ? AND token > 0 LIMIT 1');
    $q->execute([mb_strtolower(clean(param('app')), 'UTF-8'), $token]);
    $gkey = $q->fetchColumn();
    if ($gkey === false) { echo "err gone\n"; break; }
    // ueber IPv6 gilt die Absenderadresse; sonst die angegebene, wenn sie global ist
    $ip6 = is_ip6($ip) ? $ip : ip6_param(param('ip6'));
    if ($ip6 === '') { echo "err ip6\n"; break; }
    $port = number(param('port'), 1, 65535, null);
    if ($port === null) { echo "err port\n"; break; }
    $q = $db->prepare('SELECT COUNT(*) FROM knocks WHERE gkey = ?');
    $q->execute([$gkey]);
    if ((int)$q->fetchColumn() >= MAX_KNOCKS) { echo "err limit\n"; break; }
    $db->prepare('INSERT INTO knocks (gkey, ip6, port, t) VALUES (?, ?, ?, ?)')->execute([$gkey, $ip6, $port, $now]);
    echo "ok\n";
    break;

case 'knocks':
    // zaehlt auch als Lebenszeichen; leert die Klopfer des Spiels
    $q = $db->prepare('UPDATE games SET seen = ? WHERE gkey = ?');
    $q->execute([$now, param('key')]);
    if (!$q->rowCount()) { echo "err gone\n"; break; }
    $q = $db->prepare('SELECT ip6, port FROM knocks WHERE gkey = ?');
    $q->execute([param('key')]);
    $rows = $q->fetchAll(PDO::FETCH_NUM);
    $db->prepare('DELETE FROM knocks WHERE gkey = ?')->execute([param('key')]);
    echo "ok\n";
    foreach ($rows as $row) echo $row[0], "\t", $row[1], "\n";
    break;

default:
    echo "err action\n";
}
