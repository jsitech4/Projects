import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { execFileSync, spawn } from 'node:child_process';

const root = resolve(import.meta.dirname, '..');
const software = resolve(root, 'software');
const simulation = resolve(root, 'simulation');
const npm = process.platform === 'win32' ? 'npm.cmd' : 'npm';
const scenario = process.argv[2] ?? 'normal';
const allowed = new Set(['normal', 'dry-stress', 'sensor-fault', 'offline-recovery']);
if (!allowed.has(scenario)) throw new Error(`Unknown scenario "${scenario}". Use: ${[...allowed].join(', ')}.`);
if (!existsSync(resolve(software, '.env'))) throw new Error('Missing software/.env. Complete the local software setup once before running the simulator.');

function run(command, args, cwd = root) { execFileSync(command, args, { cwd, stdio: 'inherit', shell: process.platform === 'win32' }); }
function launch(args, cwd) { const child = spawn(npm, args, { cwd, detached: true, stdio: 'ignore', windowsHide: true, shell: process.platform === 'win32' }); child.unref(); return child.pid; }
async function ready(url) { try { return (await fetch(url)).ok; } catch { return false; } }
async function waitFor(url, label) { for (let i = 0; i < 30; i++) { if (await ready(url)) return; await new Promise((r) => setTimeout(r, 1000)); } throw new Error(`${label} did not start. Check its terminal logs or Docker Desktop.`); }

console.log('\nStarting Crop Water-Stress local simulation…\n');
run('docker', ['compose', '-f', 'software/docker-compose.yml', 'up', '-d', 'postgres', 'mosquitto']);
if (!existsSync(resolve(software, 'node_modules'))) run(npm, ['install'], software);
if (!existsSync(resolve(simulation, 'node_modules'))) run(npm, ['install'], simulation);
if (!existsSync(resolve(software, 'node_modules/.prisma/client'))) run(npm, ['run', 'db:generate'], software);
run(npm, ['run', 'db:migrate'], software);

if (!await ready('http://localhost:4000/health')) { console.log('Starting API…'); launch(['run', 'dev', '-w', '@cws/api'], software); }
await waitFor('http://localhost:4000/health', 'API');
if (!await ready('http://localhost:3000')) { console.log('Starting web dashboard…'); launch(['run', 'dev', '-w', '@cws/web'], software); }
await waitFor('http://localhost:3000', 'Web dashboard');

const runDir = resolve(simulation, '.run'); const pidFile = resolve(runDir, 'simulator.pid'); mkdirSync(runDir, { recursive: true });
let alreadyRunning = false;
if (existsSync(pidFile)) { try { process.kill(Number(readFileSync(pidFile, 'utf8')), 0); alreadyRunning = true; } catch { /* stale PID */ } }
if (!alreadyRunning) { const pid = launch(['run', scenario], simulation); writeFileSync(pidFile, String(pid)); console.log(`Started ${scenario} device scenario.`); } else console.log('A simulator process is already running; reusing it.');
console.log('\nReady. Sign in at http://localhost:3000/login, then open Devices.');
console.log(`Scenario: ${scenario}. To switch scenarios, stop the existing simulator first, then run: npm run simulation -- ${scenario}`);
