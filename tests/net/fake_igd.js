#!/usr/bin/env node
// Nachgebauter Router (UPnP Internet Gateway Device) fuer tests/net/upnp*.
// Nur HTTP an 127.0.0.1, keine SSDP-Suche: das Programm bekommt die Adresse
// der Geraetebeschreibung ueber BLTZNXT_NET_UPNP.
//
//   node fake_igd.js --port 28791 [--ext 203.0.113.7] [--permanent] [--lifetime 30]
//
// --permanent lehnt Leihfristen ausser 0 mit Fehler 725 ab, wie manche Router.
// Auf --port + 1 gibt ein TCP-Port das Protokoll der Aufrufe aus (eine Zeile
// je Aufruf) und schliesst dann; so liest es das Testprogramm mit
// OpenTCPStream.

'use strict';
const http = require('http');
const net = require('net');

const opt = { port: 28791, ext: '203.0.113.7', permanent: false, lifetime: 0 };
for (let i = 2; i < process.argv.length; i++) {
  const a = process.argv[i];
  if (a === '--port') opt.port = Number(process.argv[++i]);
  else if (a === '--ext') opt.ext = process.argv[++i];
  else if (a === '--permanent') opt.permanent = true;
  else if (a === '--lifetime') opt.lifetime = Number(process.argv[++i]);
  else { console.error('unknown option ' + a); process.exit(2); }
}

const SVC = 'urn:schemas-upnp-org:service:WANIPConnection:1';
const log = [];

// Beschreibung wie bei echten Routern: Dienst tief verschachtelt, relative
// Steuer-URL, davor ein anderer Dienst ohne Weiterleitung.
const DESC = `<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
<device><deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>
<serviceList><service><serviceType>urn:schemas-upnp-org:service:Layer3Forwarding:1</serviceType>
<controlURL>/ctl/L3F</controlURL></service></serviceList>
<deviceList><device><deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>
<deviceList><device><deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType>
<serviceList><service><serviceType>${SVC}</serviceType><serviceId>urn:upnp-org:serviceId:WANIPConn1</serviceId>
<controlURL>/ctl/IPConn</controlURL></service></serviceList>
</device></deviceList></device></deviceList></device></root>`;

function arg(body, name) {
  const m = body.match(new RegExp('<' + name + '>([^<]*)</' + name + '>'));
  return m ? m[1] : '';
}

function soap(res, action, inner) {
  res.writeHead(200, { 'Content-Type': 'text/xml; charset="utf-8"' });
  res.end(`<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"><s:Body>` +
          `<u:${action}Response xmlns:u="${SVC}">${inner}</u:${action}Response></s:Body></s:Envelope>`);
}

function fault(res, code) {
  res.writeHead(500, { 'Content-Type': 'text/xml; charset="utf-8"' });
  res.end(`<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"><s:Body>` +
          `<s:Fault><faultcode>s:Client</faultcode><faultstring>UPnPError</faultstring><detail>` +
          `<UPnPError xmlns="urn:schemas-upnp-org:control-1-0"><errorCode>${code}</errorCode>` +
          `<errorDescription>error</errorDescription></UPnPError></detail></s:Fault></s:Body></s:Envelope>`);
}

http.createServer((req, res) => {
  let body = '';
  req.on('data', (d) => { body += d; });
  req.on('end', () => {
    if (req.method === 'GET' && req.url === '/desc.xml') {
      res.writeHead(200, { 'Content-Type': 'text/xml' });
      res.end(DESC);
      return;
    }
    if (req.method !== 'POST' || req.url !== '/ctl/IPConn') {
      res.writeHead(404);
      res.end();
      return;
    }
    const action = (req.headers.soapaction || '').replace(/"/g, '').split('#')[1] || '?';
    if (action === 'AddPortMapping') {
      const lease = arg(body, 'NewLeaseDuration');
      if (opt.permanent && lease !== '0') {
        log.push('add refused lease ' + lease);
        fault(res, 725);
        return;
      }
      log.push(`add ${arg(body, 'NewProtocol')} ${arg(body, 'NewExternalPort')} -> ` +
               `${arg(body, 'NewInternalClient')}:${arg(body, 'NewInternalPort')} lease ${lease} ` +
               `[${arg(body, 'NewPortMappingDescription')}]`);
      soap(res, action, '');
    } else if (action === 'DeletePortMapping') {
      log.push(`del ${arg(body, 'NewProtocol')} ${arg(body, 'NewExternalPort')}`);
      soap(res, action, '');
    } else if (action === 'GetExternalIPAddress') {
      log.push('ext');
      soap(res, action, `<NewExternalIPAddress>${opt.ext}</NewExternalIPAddress>`);
    } else {
      log.push('unknown ' + action);
      fault(res, 401);
    }
  });
}).listen(opt.port, '127.0.0.1');

net.createServer((s) => { s.end(log.join('\n') + '\n'); }).listen(opt.port + 1, '127.0.0.1');

if (opt.lifetime > 0) setTimeout(() => process.exit(0), opt.lifetime * 1000).unref();
