// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. See LICENSING.md for details.
//
// Run the gallery built with Emscripten in a headless Chrome, print what the
// page prints, and exit with its exit code: the --check of the desktop gallery,
// in a browser, for the CI. WebGL 2 is drawn on the CPU (SwiftShader).
//
//   python3 -m http.server -d build/emscripten-release/bin 8080 &
//   node examples/Web/check.mjs "http://127.0.0.1:8080/Compages-examples.html?args=--check 05f_Galaxy" 300
//
// The browser is $CHROME, or google-chrome. Node 22 or later (WebSocket).

import { spawn } from 'node:child_process';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const [url, seconds = '300'] = process.argv.slice(2);
if (!url) {
  console.error('usage: node check.mjs URL [timeout in seconds]');
  process.exit(2);
}

const profile = mkdtempSync(join(tmpdir(), 'compages-check-'));
const port = 9300 + Math.floor(Math.random() * 600);
const chrome = spawn(process.env.CHROME || 'google-chrome', [
  '--headless=new', '--no-sandbox', `--user-data-dir=${profile}`,
  '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist',
  '--window-size=1280,720', `--remote-debugging-port=${port}`, 'about:blank',
], { stdio: 'ignore' });

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
const finish = (code) => {
  chrome.kill('SIGKILL');
  rmSync(profile, { recursive: true, force: true });
  process.exit(code);
};

let targets = null;
for (let i = 0; (i < 100) && !targets; ++i) {
  try {
    targets = await (await fetch(`http://127.0.0.1:${port}/json/list`)).json();
  } catch {
    await sleep(200);
  }
}
const page = targets?.find((target) => target.type === 'page');
if (!page) {
  console.error('the browser did not start');
  finish(2);
}

const socket = new WebSocket(page.webSocketDebuggerUrl);
await new Promise((resolve) => socket.addEventListener('open', resolve));
let next = 0;
const send = (method, params = {}) => socket.send(JSON.stringify({ id: ++next, method, params }));

let exit = null;
socket.addEventListener('message', (event) => {
  const message = JSON.parse(event.data);
  if (message.method === 'Runtime.consoleAPICalled') {
    const text = message.params.args.map((arg) => arg.value ?? arg.description ?? '').join(' ');
    console.log(text);
    const found = /^EXIT (-?\d+)/.exec(text);
    if (found) {
      exit = Number(found[1]);
    }
  } else if (message.method === 'Runtime.exceptionThrown') {
    const details = message.params.exceptionDetails;
    console.log('exception: ' + (details.exception?.description ?? details.text));
    exit = (exit === null) ? 1 : exit;
  }
});
send('Runtime.enable');
send('Page.navigate', { url });

const end = Date.now() + Number(seconds) * 1000;
while ((exit === null) && (Date.now() < end)) {
  await sleep(200);
}
if (exit === null) {
  console.error(`no exit after ${seconds} s`);
  finish(2);
}
finish(exit);
