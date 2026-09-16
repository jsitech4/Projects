#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <math.h>

#include "local_server.h"

#include "device_manager/device_manager.h"
#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"
#include "load_relay/load_relay.h"
#include "storage/storage.h"

namespace local_server
{
  static WebServer server(80);

  static bool running = false;

  static String ipAddress =
      "0.0.0.0";

  static const char *apSsid =
      "Hot Liquid Monitoring and Control System";

  static const char *apPassword =
      "12345678";

  // =========================================================
  // SETTINGS UI STATE
  // =========================================================

  static const char indexPage[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width,initial-scale=1.0">

<title>
Hot Liquid Monitoring and Control System
</title>

<style>

:root{
--bg:#eef3f8;
--card:#ffffff;
--text:#18212f;
--muted:#667085;
--accent:#2563eb;
--good:#16a34a;
--warn:#ca8a04;
--bad:#dc2626;
--orange:#ea580c;
--line:rgba(100,116,139,.22);
--shadow:0 14px 35px rgba(15,23,42,.12);
}

[data-theme=dark]{
--bg:#0b1220;
--card:#111c2f;
--text:#e5edf8;
--muted:#98a2b3;
--accent:#60a5fa;
--line:rgba(148,163,184,.18);
--shadow:0 14px 35px rgba(0,0,0,.35);
}

*{
box-sizing:border-box;
}

html{
scroll-behavior:smooth;
}

body{
margin:0;
min-height:100vh;
font-family:Arial,Helvetica,sans-serif;
background:
radial-gradient(
circle at top left,
rgba(37,99,235,.14),
transparent 32%
),
var(--bg);
color:var(--text);
display:flex;
justify-content:center;
}

.page{
width:100%;
max-width:1220px;
margin:0 auto;
}

.header{
padding:22px 18px;
display:flex;
justify-content:center;
align-items:center;
gap:14px;
max-width:1180px;
margin:auto;
flex-wrap:wrap;
text-align:center;
}

.title{
flex:1 1 560px;
display:flex;
flex-direction:column;
align-items:center;
}

.title h1{
margin:0;
font-size:25px;
letter-spacing:-.4px;
}

.title p{
margin:6px auto 0;
color:var(--muted);
line-height:1.4;
max-width:760px;
}

.top-actions{
display:flex;
justify-content:center;
align-items:center;
gap:10px;
flex:1 1 100%;
}

.pill{
border:0;
border-radius:999px;
padding:10px 14px;
background:var(--card);
color:var(--text);
box-shadow:var(--shadow);
font-weight:700;
}

.theme-btn{
width:44px;
height:44px;
border:0;
border-radius:50%;
background:var(--card);
color:var(--text);
box-shadow:var(--shadow);
cursor:pointer;
font-size:20px;
}

.container{
max-width:1180px;
margin:auto;
padding:0 16px 30px;
}

.grid{
display:grid;
grid-template-columns:
repeat(12,minmax(0,1fr));
gap:16px;
}

.card{
background:var(--card);
border:1px solid var(--line);
border-radius:22px;
padding:18px;
box-shadow:var(--shadow);
text-align:center;
display:flex;
flex-direction:column;
align-items:center;
}

.span3{grid-column:span 3}
.span4{grid-column:span 4}
.span5{grid-column:span 5}
.span6{grid-column:span 6}
.span7{grid-column:span 7}
.span8{grid-column:span 8}
.span12{grid-column:span 12}

.label{
font-size:13px;
color:var(--muted);
margin-bottom:8px;
font-weight:700;
text-transform:uppercase;
letter-spacing:.04em;
}

.value{
font-size:29px;
font-weight:800;
}

.unit{
font-size:14px;
color:var(--muted);
font-weight:500;
}

.small{
color:var(--muted);
font-size:13px;
line-height:1.5;
max-width:760px;
}

.status{
display:inline-block;
padding:9px 13px;
border-radius:999px;
font-weight:800;
font-size:13px;
background:#e5e7eb;
color:#111827;
}

.status.NORMAL{
background:#dcfce7;
color:#166534;
}

.status.LOW{
background:#dbeafe;
color:#1d4ed8;
}

.status.HIGH{
background:#fee2e2;
color:#991b1b;
}

.status.INVALID{
background:#e5e7eb;
color:#475569;
}

.bar{
width:100%;
max-width:520px;
height:12px;
background:rgba(100,116,139,.22);
border-radius:999px;
overflow:hidden;
margin-top:10px;
}

.fill{
height:100%;
width:0%;
background:var(--accent);
transition:width .35s ease;
border-radius:999px;
}

.fill.good{
background:var(--good);
}

.fill.warn{
background:var(--warn);
}

.fill.bad{
background:var(--bad);
}

.tank-wrap{
display:flex;
align-items:center;
justify-content:center;
gap:28px;
width:100%;
padding:8px 0 14px;
}

.tank{
position:relative;
width:170px;
height:250px;
border:4px solid var(--text);
border-radius:18px 18px 28px 28px;
background:rgba(148,163,184,.10);
overflow:hidden;
}

.tank:before{
content:'';
position:absolute;
left:24px;
right:24px;
top:12px;
height:7px;
border:2px solid var(--text);
border-radius:8px;
}

.liquid{
position:absolute;
left:0;
right:0;
bottom:0;
height:0%;
background:
linear-gradient(
180deg,
#38bdf8,
#0284c7
);
transition:height .5s ease;
}

.tank-label{
position:absolute;
inset:0;
display:flex;
align-items:center;
justify-content:center;
z-index:1;
font-size:27px;
font-weight:800;
color:#fff;
text-shadow:
0 2px 4px rgba(0,0,0,.5);
}

.valve{
display:flex;
flex-direction:column;
align-items:center;
gap:10px;
min-width:150px;
}

.valve-body{
width:74px;
height:74px;
border:5px solid var(--text);
transform:rotate(45deg);
background:var(--bad);
transition:background .25s ease;
}

.valve-body.open{
background:var(--good);
}

.valve-stem{
width:8px;
height:38px;
background:var(--text);
margin-top:-32px;
z-index:1;
}

.valve-title{
font-size:18px;
font-weight:800;
}

.valve-state{
font-size:13px;
color:var(--muted);
}

.actions{
display:flex;
justify-content:center;
gap:10px;
flex-wrap:wrap;
width:100%;
}

.btn{
border:0;
border-radius:14px;
padding:11px 14px;
font-weight:800;
background:var(--accent);
color:#fff;
cursor:pointer;
}

.btn.secondary{
background:#64748b;
}

.btn.danger{
background:var(--bad);
}

.btn.good{
background:var(--good);
}

input,
select{
padding:9px 10px;
border-radius:10px;
border:1px solid var(--line);
background:var(--card);
color:var(--text);
font-size:14px;
min-width:150px;
}

.metric-row{
width:100%;
display:flex;
justify-content:space-between;
align-items:center;
gap:12px;
border-top:1px solid var(--line);
padding:12px 0;
text-align:left;
}

.metric-row:first-of-type{
border-top:0;
}

.metric-row strong{
font-size:14px;
}

pre{
width:100%;
background:rgba(100,116,139,.12);
border:1px solid var(--line);
border-radius:16px;
padding:12px;
overflow:auto;
max-height:330px;
font-size:12px;
white-space:pre-wrap;
text-align:left;
}

canvas{
width:100%;
height:260px;
display:block;
}

.footer{
margin-top:16px;
text-align:center;
color:var(--muted);
font-size:12px;
}

@media(max-width:900px){

.span3,
.span4,
.span5,
.span6,
.span7,
.span8{
grid-column:span 12;
}

.value{
font-size:24px;
}

.metric-row{
flex-direction:column;
text-align:center;
}

input,
select{
width:100%;
}

}

</style>

</head>

<body>

<div class="page">

<div class="header">

<div class="title">

<h1>
Hot Liquid Monitoring and Control System
</h1>

<p>
Live tank level, PT100 temperature,
ultrasonic measurement and valve control.
</p>

</div>

<div class="top-actions">

<span class="pill"
      id="backendPill">
Backend: --
</span>

<button
class="theme-btn"
id="themeIcon"
onclick="toggleTheme()">
🌙
</button>

</div>

</div>

<div class="container">

<div class="grid">

<!-- TEMPERATURE -->

<div class="card span3">

<div class="label">
Temperature
</div>

<div class="value"
     id="temp">
-- <span class="unit">°C</span>
</div>

<span
class="status NORMAL"
id="tempStatus">
NORMAL
</span>

<p class="small"
   id="tempRange">
Thresholds: -- / -- °C
</p>

</div>

<!-- DISTANCE -->

<div class="card span3">

<div class="label">
Ultrasonic Distance
</div>

<div class="value"
     id="distance">
-- <span class="unit">cm</span>
</div>

<p class="small">
Distance from ultrasonic sensor
to liquid surface.
</p>

</div>

<!-- LEVEL -->

<div class="card span3">

<div class="label">
Tank Level
</div>

<div class="value"
     id="levelPercent">
-- <span class="unit">%</span>
</div>

<p class="small"
   id="calibrationText">
Calibration: -- cm full / -- cm low
</p>

</div>

<!-- CONDITION -->

<div class="card span3">

<div class="label">
Tank Condition
</div>

<span class="status NORMAL"
      id="levelStatus">
NORMAL
</span>

<p class="small"
   id="levelConditionText">
Waiting for measurement.
</p>

</div>

<!-- TANK -->

<div class="card span12">

<div class="label">
Tank And Valve
</div>

<div class="tank-wrap">

<div class="tank">

<div class="liquid"
     id="liquidFill">
</div>

<div class="tank-label"
     id="tankLabel">
--%
</div>

</div>

<div class="valve">

<div class="valve-body"
     id="valveBody">
</div>

<div class="valve-stem">
</div>

<div class="valve-title">
Outlet Valve
</div>

<div class="valve-state"
     id="valveState">
Relay OFF
</div>

</div>

</div>

<div class="bar">

<div class="fill good"
     id="tankFill">
</div>

</div>

<p class="small">
The valve symbol follows the physical relay state.
</p>

</div>

<!-- SENSOR STATUS -->

<div class="card span6">

<div class="label">
Sensor Status
</div>

<div class="value"
     id="sensorStatus">
WAITING
</div>

<p class="small"
   id="sensorStatusText">
Waiting for PT100 and ultrasonic measurements.
</p>

</div>

<!-- AUTOMATION STATE -->

<div class="card span6">

<div class="label">
Automation State
</div>

<div class="metric-row">

<strong>
Tank latch
</strong>

<span id="tankLatchState">
NOT TRIGGERED
</span>

</div>

<div class="metric-row">

<strong>
Temperature latch
</strong>

<span id="temperatureLatchState">
NOT TRIGGERED
</span>

</div>

</div>

<!-- VALVE CONTROL -->

<div class="card span12">

<div class="label">
Valve Control
</div>

<p class="small"
   id="relay">
Valve relay: --
</p>

<div class="actions">

<button
class="btn good"
onclick="cmd('/api/relay?state=on')">
Valve OPEN
</button>

<button
class="btn danger"
onclick="cmd('/api/relay?state=off')">
Valve CLOSE
</button>

</div>

<p class="small">
Manual commands directly control the relay.
Active automatic latch conditions can turn it ON again.
</p>

</div>

<!-- SETTINGS -->

<div class="card span12">

<div class="label">
System Settings
</div>

<div class="metric-row">

<strong>
Full tank distance
</strong>

<span>

<input
id="fullDistance"
type="number"
min="1"
max="499"
step="0.1">

cm

</span>

</div>

<div class="metric-row">

<strong>
Low / empty tank distance
</strong>

<span>

<input
id="lowDistance"
type="number"
min="1"
max="500"
step="0.1">

cm

</span>

</div>

<div class="metric-row">

<strong>
Full level trigger
</strong>

<span>

<input
id="fullThreshold"
type="number"
min="1"
max="100"
step="1">

%

</span>

</div>

<div class="metric-row">

<strong>
Low level trigger
</strong>

<span>

<input
id="lowThreshold"
type="number"
min="0"
max="99"
step="1">

%

</span>

</div>

<div class="metric-row">

<strong>
Tank relay latch
</strong>

<span>

<select id="tankLatchMode">

<option value="0">
Disabled
</option>

<option value="1">
Latch at full level
</option>

<option value="2">
Latch at low level
</option>

</select>

</span>

</div>

<div class="metric-row">

<strong>
Low temperature threshold
</strong>

<span>

<input
id="lowTemperature"
type="number"
min="-200"
max="849"
step="0.1">

°C

</span>

</div>

<div class="metric-row">

<strong>
High temperature threshold
</strong>

<span>

<input
id="highTemperature"
type="number"
min="-199"
max="850"
step="0.1">

°C

</span>

</div>

<div class="metric-row">

<strong>
Temperature relay latch
</strong>

<span>

<select id="temperatureLatchMode">

<option value="0">
Disabled
</option>

<option value="1">
Latch at LOW temperature
</option>

<option value="2">
Latch at HIGH temperature
</option>

</select>

</span>

</div>

<div class="actions">

<button
class="btn good"
onclick="saveSettings()">
Save settings
</button>

<span
class="small"
id="settingsMessage">
Saved settings are restored after reboot.
</span>

</div>

</div>

<!-- LOGS -->

<div class="card span12">

<div class="label">
Recent Liquid Data Log
</div>

<pre id="motorLog">
Loading...
</pre>

<div class="actions">

<button
class="btn secondary"
onclick="loadLogs()">
Refresh Logs
</button>

<button
class="btn secondary"
onclick="cmd('/api/log_now')">
Log Now
</button>

<button
class="btn"
onclick="location.href='/download/motor_log.csv'">
Download Liquid CSV
</button>

<button
class="btn"
onclick="location.href='/download/analysis_log.csv'">
Download Analysis CSV
</button>

<button
class="btn"
onclick="location.href='/download/event_log.csv'">
Download Events CSV
</button>

</div>

</div>

<div class="card span6">

<div class="label">
Recent Analysis Log
</div>

<pre id="analysisLog">
Loading...
</pre>

</div>

<div class="card span6">

<div class="label">
Recent Event Log
</div>

<pre id="eventLog">
Loading...
</pre>

</div>

<!-- GRAPH -->

<div class="card span12">

<div class="label">
Trend Graph
</div>

<canvas
id="chart"
width="1000"
height="260">
</canvas>

<p class="small">
Temperature and tank level history.
</p>

</div>

</div>

<div class="footer">

ESP32-S3 WROOM-1U
Hot Liquid Monitoring and Control System

</div>

</div>

</div>

<script>

let points = [];

let settingsDirty = false;

let settingsSaving = false;

// =========================================================
// THEME
// =========================================================

document.documentElement.setAttribute(
'data-theme',
localStorage.getItem('theme') || 'light'
);

updateThemeIcon();

function updateThemeIcon(){

const icon =
document.getElementById('themeIcon');

if(!icon){
return;
}

icon.textContent =
document.documentElement
.getAttribute('data-theme') === 'dark'
? '☀️'
: '🌙';

}

function toggleTheme(){

const d =
document.documentElement;

const theme =
d.getAttribute('data-theme') === 'dark'
? 'light'
: 'dark';

d.setAttribute(
'data-theme',
theme
);

localStorage.setItem(
'theme',
theme
);

updateThemeIcon();

drawChart();

}

// =========================================================
// FORMAT
// =========================================================

function fmt(value, decimals){

const v = Number(value);

if(!Number.isFinite(v)){
return '--';
}

return v.toFixed(decimals);

}

function clamp(value,min,max){

return Math.max(
min,
Math.min(max,value)
);

}

// =========================================================
// SETTINGS DIRTY
// =========================================================

function markSettingsDirty(){

settingsDirty = true;

const message =
document.getElementById(
'settingsMessage'
);

if(message){

message.textContent =
'Unsaved changes.';

}

}

// =========================================================
// LOAD SETTINGS INTO UI
// =========================================================

function updateSettingsControls(s){

if(settingsDirty){
return;
}

document.getElementById(
'fullDistance'
).value =
s.full_distance_cm;

document.getElementById(
'lowDistance'
).value =
s.low_distance_cm;

document.getElementById(
'fullThreshold'
).value =
s.full_level_percent;

document.getElementById(
'lowThreshold'
).value =
s.low_level_percent;

document.getElementById(
'tankLatchMode'
).value =
s.tank_latch_mode;

document.getElementById(
'lowTemperature'
).value =
s.low_temperature_c;

document.getElementById(
'highTemperature'
).value =
s.high_temperature_c;

document.getElementById(
'temperatureLatchMode'
).value =
s.temperature_latch_mode;

}

// =========================================================
// FILL
// =========================================================

function setFill(id,value){

const element =
document.getElementById(id);

if(!element){
return;
}

const v =
clamp(
Number(value) || 0,
0,
100
);

element.style.width =
v + '%';

if(v >= 75){

element.className =
'fill good';

}
else if(v >= 40){

element.className =
'fill warn';

}
else{

element.className =
'fill bad';

}

}

// =========================================================
// COMMAND
// =========================================================

async function cmd(url){

try{

const response =
await fetch(url);

if(!response.ok){
throw new Error(
'Command failed'
);
}

await loadStatus();

await loadLogs();

}
catch(error){

console.error(error);

}

}

// =========================================================
// STATUS
// =========================================================

async function loadStatus(){

try{

const response =
await fetch(
'/api/status',
{
cache:'no-store'
}
);

const s =
await response.json();

// ---------------------------------------------------------
// Temperature
// ---------------------------------------------------------

document.getElementById(
'temp'
).innerHTML =
s.temp_valid
?
fmt(
s.temperature_c,
1
) +
' <span class="unit">°C</span>'
:
'N/A';

const tempStatus =
document.getElementById(
'tempStatus'
);

tempStatus.textContent =
s.temperature_status;

tempStatus.className =
'status ' +
s.temperature_status;

document.getElementById(
'tempRange'
).textContent =
'Low: ' +
fmt(
s.low_temperature_c,
1
) +
' °C | High: ' +
fmt(
s.high_temperature_c,
1
) +
' °C';

// ---------------------------------------------------------
// Distance
// ---------------------------------------------------------

document.getElementById(
'distance'
).innerHTML =
s.level_valid
?
fmt(
s.distance_cm,
1
) +
' <span class="unit">cm</span>'
:
'N/A';

// ---------------------------------------------------------
// Level
// ---------------------------------------------------------

document.getElementById(
'levelPercent'
).innerHTML =
s.level_valid
?
fmt(
s.level_percent,
0
) +
' <span class="unit">%</span>'
:
'N/A';

const level =
s.level_valid
?
clamp(
Number(s.level_percent),
0,
100
)
:
0;

document.getElementById(
'liquidFill'
).style.height =
level + '%';

setFill(
'tankFill',
level
);

document.getElementById(
'tankLabel'
).textContent =
s.level_valid
?
fmt(
s.level_percent,
0
) + '%'
:
'N/A';

document.getElementById(
'calibrationText'
).textContent =
'Calibration: ' +
fmt(
s.full_distance_cm,
1
) +
' cm full / ' +
fmt(
s.low_distance_cm,
1
) +
' cm low';

// ---------------------------------------------------------
// Tank status
// ---------------------------------------------------------

const levelStatus =
document.getElementById(
'levelStatus'
);

let levelText =
'NORMAL';

if(!s.level_valid){

levelText =
'INVALID';

}
else if(
s.level_percent >=
s.full_level_percent
){

levelText =
'FULL';

}
else if(
s.level_percent <=
s.low_level_percent
){

levelText =
'LOW';

}

levelStatus.textContent =
levelText;

levelStatus.className =
'status ' +
(
levelText === 'FULL'
?
'HIGH'
:
levelText === 'LOW'
?
'LOW'
:
levelText === 'INVALID'
?
'INVALID'
:
'NORMAL'
);

document.getElementById(
'levelConditionText'
).textContent =
'Full trigger: ' +
fmt(
s.full_level_percent,
0
) +
'% | Low trigger: ' +
fmt(
s.low_level_percent,
0
) +
'%';

// ---------------------------------------------------------
// Backend
// ---------------------------------------------------------

document.getElementById(
'backend'
);

document.getElementById(
'backendPill'
).textContent =
'Backend: ' +
s.backend;

// ---------------------------------------------------------
// Sensors
// ---------------------------------------------------------

const sensorsReady =
s.temp_valid &&
s.level_valid;

document.getElementById(
'sensorStatus'
).textContent =
sensorsReady
?
'READY'
:
'WAITING';

document.getElementById(
'sensorStatusText'
).textContent =
sensorsReady
?
'PT100 and ultrasonic measurements are valid.'
:
'Waiting for valid sensor readings.';

// ---------------------------------------------------------
// Relay
// ---------------------------------------------------------

document.getElementById(
'relay'
).textContent =
'Valve relay: ' +
(s.relay_on ? 'ON' : 'OFF') +
' | Requested: ' +
(s.relay_requested ? 'ON' : 'OFF');

document.getElementById(
'valveState'
).textContent =
s.relay_on
?
'Relay ON / Valve OPEN'
:
'Relay OFF / Valve CLOSED';

document.getElementById(
'valveBody'
).className =
'valve-body' +
(s.relay_on ? ' open' : '');

// ---------------------------------------------------------
// Latch states
// ---------------------------------------------------------

document.getElementById(
'tankLatchState'
).textContent =
s.tank_latch_triggered
?
'TRIGGERED'
:
'NOT TRIGGERED';

document.getElementById(
'temperatureLatchState'
).textContent =
s.temperature_latch_triggered
?
'TRIGGERED'
:
'NOT TRIGGERED';

// ---------------------------------------------------------
// Settings
// ---------------------------------------------------------

updateSettingsControls(s);

// ---------------------------------------------------------
// Chart
// ---------------------------------------------------------

if(
s.temp_valid ||
s.level_valid
){

points.push({

temp:
s.temp_valid
?
Number(s.temperature_c)
:
null,

level:
s.level_valid
?
Number(s.level_percent)
:
null

});

if(points.length > 70){

points.shift();

}

drawChart();

}

}
catch(error){

console.error(
'Status error:',
error
);

}

}

// =========================================================
// SAVE SETTINGS
// =========================================================

async function saveSettings(){

if(settingsSaving){
return;
}

const fullDistance =
Number(
document.getElementById(
'fullDistance'
).value
);

const lowDistance =
Number(
document.getElementById(
'lowDistance'
).value
);

const fullThreshold =
Number(
document.getElementById(
'fullThreshold'
).value
);

const lowThreshold =
Number(
document.getElementById(
'lowThreshold'
).value
);

const tankLatch =
Number(
document.getElementById(
'tankLatchMode'
).value
);

const lowTemperature =
Number(
document.getElementById(
'lowTemperature'
).value
);

const highTemperature =
Number(
document.getElementById(
'highTemperature'
).value
);

const temperatureLatch =
Number(
document.getElementById(
'temperatureLatchMode'
).value
);

const message =
document.getElementById(
'settingsMessage'
);

// ---------------------------------------------------------
// Validation
// ---------------------------------------------------------

if(
!Number.isFinite(fullDistance) ||
!Number.isFinite(lowDistance) ||
fullDistance <= 0 ||
lowDistance <= fullDistance
){

message.textContent =
'Full distance must be smaller than low distance.';

return;

}

if(
!Number.isFinite(fullThreshold) ||
!Number.isFinite(lowThreshold) ||
fullThreshold <= lowThreshold ||
fullThreshold > 100 ||
lowThreshold < 0
){

message.textContent =
'Full level must be higher than low level.';

return;

}

if(
!Number.isFinite(lowTemperature) ||
!Number.isFinite(highTemperature) ||
lowTemperature >= highTemperature
){

message.textContent =
'Low temperature must be below high temperature.';

return;

}

settingsSaving = true;

message.textContent =
'Saving settings...';

// ---------------------------------------------------------
// Build request
// ---------------------------------------------------------

const query =
'/api/settings/save?' +

'full_distance=' +
encodeURIComponent(
fullDistance
) +

'&low_distance=' +
encodeURIComponent(
lowDistance
) +

'&full_level=' +
encodeURIComponent(
fullThreshold
) +

'&low_level=' +
encodeURIComponent(
lowThreshold
) +

'&tank_latch=' +
encodeURIComponent(
tankLatch
) +

'&low_temperature=' +
encodeURIComponent(
lowTemperature
) +

'&high_temperature=' +
encodeURIComponent(
highTemperature
) +

'&temperature_latch=' +
encodeURIComponent(
temperatureLatch
);

// ---------------------------------------------------------
// Send
// ---------------------------------------------------------

try{

const response =
await fetch(
query,
{
cache:'no-store'
}
);

const result =
await response.json();

if(result.ok){

settingsDirty = false;

message.textContent =
'Settings saved to internal flash and applied.';

await loadStatus();

}
else{

message.textContent =
result.message ||
'Settings were not saved.';

}

}
catch(error){

console.error(error);

message.textContent =
'Dashboard could not save settings.';

}

settingsSaving = false;

}

// =========================================================
// LOGS
// =========================================================

async function loadLogs(){

try{

const response =
await fetch(
'/api/logs',
{
cache:'no-store'
}
);

const s =
await response.json();

document.getElementById(
'motorLog'
).textContent =
s.motor_log ||
'No liquid log yet';

document.getElementById(
'analysisLog'
).textContent =
s.analysis_log ||
'No analysis log yet';

document.getElementById(
'eventLog'
).textContent =
s.event_log ||
'No event log yet';

}
catch(error){

console.error(error);

}

}

// =========================================================
// CHART
// =========================================================

function drawChart(){

const canvas =
document.getElementById(
'chart'
);

const ctx =
canvas.getContext('2d');

const w =
canvas.width;

const h =
canvas.height;

ctx.clearRect(
0,
0,
w,
h
);

ctx.strokeStyle =
getComputedStyle(
document.documentElement
)
.getPropertyValue(
'--line'
);

ctx.lineWidth = 1;

for(
let i = 0;
i < 5;
i++
){

const y =
30 + i * 45;

ctx.beginPath();

ctx.moveTo(
30,
y
);

ctx.lineTo(
w - 20,
y
);

ctx.stroke();

}

drawLine(
ctx,
points.map(
p => p.temp
),
0,
100,
'#ea580c'
);

drawLine(
ctx,
points.map(
p => p.level
),
0,
100,
'#16a34a'
);

}

function drawLine(
ctx,
arr,
min,
max,
color
){

if(arr.length < 2){
return;
}

const valid =
arr.map(
(v,i) => ({
v,
i
})
)
.filter(
x =>
Number.isFinite(x.v)
);

if(valid.length < 2){
return;
}

const w =
ctx.canvas.width;

const h =
ctx.canvas.height;

ctx.strokeStyle =
color;

ctx.lineWidth = 3;

ctx.beginPath();

valid.forEach(
(item,index) => {

const x =
30 +
(item.i /
Math.max(
1,
points.length - 1
)) *
(w - 55);

const y =
h -
25 -
(
(item.v - min) /
(max - min)
) *
(h - 55);

if(index === 0){

ctx.moveTo(
x,
y
);

}
else{

ctx.lineTo(
x,
y
);

}

}
);

ctx.stroke();

}

// =========================================================
// INPUT CHANGE EVENTS
// =========================================================

[
'fullDistance',
'lowDistance',
'fullThreshold',
'lowThreshold',
'tankLatchMode',
'lowTemperature',
'highTemperature',
'temperatureLatchMode'
]
.forEach(
id => {

const element =
document.getElementById(id);

if(element){

element.addEventListener(
'input',
markSettingsDirty
);

element.addEventListener(
'change',
markSettingsDirty
);

}

}
);

// =========================================================
// START
// =========================================================

loadStatus();

loadLogs();

setInterval(
loadStatus,
1000
);

setInterval(
loadLogs,
10000
);

</script>

</body>
</html>
)HTML";

  // =========================================================
  // JSON ESCAPE
  // =========================================================

  static String jsonEscape(
      const String &input)
  {
    String out;

    out.reserve(
        input.length() + 8);

    for (size_t i = 0;
         i < input.length();
         i++)
    {
      char c =
          input[i];

      if (c == '"')
      {
        out += "\\\"";
      }
      else if (c == '\\')
      {
        out += "\\\\";
      }
      else if (c == '\n')
      {
        out += "\\n";
      }
      else if (c == '\r')
      {
        continue;
      }
      else
      {
        out += c;
      }
    }

    return out;
  }

  // =========================================================
  // CORS
  // =========================================================

  static void sendCors()
  {
    server.sendHeader(
        "Access-Control-Allow-Origin",
        "*");

    server.sendHeader(
        "Cache-Control",
        "no-store");
  }

  // =========================================================
  // ROOT
  // =========================================================

  static void handleRoot()
  {
    sendCors();

    server.send_P(
        200,
        "text/html",
        indexPage);
  }

  // =========================================================
  // STATUS
  // =========================================================

  static void handleStatus()
  {
    sendCors();

    device_manager::Snapshot snap =
        device_manager::getSnapshot();

    String json;

    json.reserve(1800);

    json += "{";

    json += "\"uptime_ms\":";
    json += String(millis());
    json += ",";

    // -----------------------------------------------------
    // Temperature
    // -----------------------------------------------------

    json += "\"temperature_c\":";

    if (snap.tempValid)
    {
      json +=
          String(
              snap.temperatureC,
              2);
    }
    else
    {
      json += "null";
    }

    json += ",";

    json += "\"temp_valid\":";
    json +=
        snap.tempValid
            ? "true"
            : "false";

    json += ",";

    // -----------------------------------------------------
    // Temperature settings
    // -----------------------------------------------------

    json += "\"low_temperature_c\":";
    json +=
        String(
            device_manager::
                getLowTemperatureC(),
            2);

    json += ",";

    json += "\"high_temperature_c\":";
    json +=
        String(
            device_manager::
                getHighTemperatureC(),
            2);

    json += ",";

    json += "\"temperature_status\":\"";
    json +=
        device_manager::
            getTemperatureStatusText();
    json += "\",";

    // -----------------------------------------------------
    // Distance
    // -----------------------------------------------------

    json += "\"distance_cm\":";

    if (snap.levelValid)
    {
      json +=
          String(
              snap.distanceCm,
              2);
    }
    else
    {
      json += "null";
    }

    json += ",";

    json += "\"level_valid\":";
    json +=
        snap.levelValid
            ? "true"
            : "false";

    json += ",";

    // -----------------------------------------------------
    // Level
    // -----------------------------------------------------

    json += "\"level_percent\":";

    if (snap.levelValid)
    {
      json +=
          String(
              snap.levelPercent,
              1);
    }
    else
    {
      json += "null";
    }

    json += ",";

    json += "\"full_distance_cm\":";
    json +=
        String(
            device_manager::
                getFullDistanceCm(),
            2);

    json += ",";

    json += "\"low_distance_cm\":";
    json +=
        String(
            device_manager::
                getLowDistanceCm(),
            2);

    json += ",";

    json += "\"full_level_percent\":";
    json +=
        String(
            device_manager::
                getFullLevelPercent(),
            1);

    json += ",";

    json += "\"low_level_percent\":";
    json +=
        String(
            device_manager::
                getLowLevelPercent(),
            1);

    json += ",";

    // -----------------------------------------------------
    // Tank latch
    // -----------------------------------------------------

    json += "\"tank_latch_mode\":";
    json +=
        String(
            static_cast<uint8_t>(
                device_manager::
                    getRelayLatchMode()));

    json += ",";

    json += "\"tank_latch_triggered\":";
    json +=
        snap.tankLatchTriggered
            ? "true"
            : "false";

    json += ",";

    // -----------------------------------------------------
    // Temperature latch
    // -----------------------------------------------------

    json += "\"temperature_latch_mode\":";
    json +=
        String(
            static_cast<uint8_t>(
                device_manager::
                    getTemperatureLatchMode()));

    json += ",";

    json += "\"temperature_latch_triggered\":";
    json +=
        snap.temperatureLatchTriggered
            ? "true"
            : "false";

    json += ",";

    // -----------------------------------------------------
    // Relay
    // -----------------------------------------------------

    json += "\"relay_on\":";
    json +=
        load_relay::isOn()
            ? "true"
            : "false";

    json += ",";

    json += "\"relay_requested\":";
    json +=
        load_relay::getRequestedState()
            ? "true"
            : "false";

    json += ",";

    // -----------------------------------------------------
    // Storage
    // -----------------------------------------------------

    json += "\"sd_ready\":";
    json +=
        storage::isSdReady()
            ? "true"
            : "false";

    json += ",";

    json += "\"internal_ready\":";
    json +=
        storage::isInternalReady()
            ? "true"
            : "false";

    json += ",";

    json += "\"backend\":\"";
    json +=
        jsonEscape(
            storage::getBackendName());
    json += "\",";

    json += "\"liquid_log_size\":";
    json +=
        String(
            storage::getFileSize(
                storage::
                    getLiquidLogFileName()));

    json += ",";

    json += "\"analysis_log_size\":";
    json +=
        String(
            storage::getFileSize(
                storage::
                    getAnalysisLogFileName()));

    json += ",";

    json += "\"event_log_size\":";
    json +=
        String(
            storage::getFileSize(
                storage::
                    getEventLogFileName()));

    json += "}";

    server.send(
        200,
        "application/json",
        json);
  }

  // =========================================================
  // LOGS
  // =========================================================

  static void handleLogs()
  {
    sendCors();

    String json;

    json.reserve(14000);

    json += "{";

    json += "\"motor_log\":\"";

    json +=
        jsonEscape(
            storage::readTail(
                storage::
                    getLiquidLogFileName(),
                5000));

    json += "\",";

    json += "\"analysis_log\":\"";

    json +=
        jsonEscape(
            storage::readTail(
                storage::
                    getAnalysisLogFileName(),
                4000));

    json += "\",";

    json += "\"event_log\":\"";

    json +=
        jsonEscape(
            storage::readTail(
                storage::
                    getEventLogFileName(),
                3000));

    json += "\"";

    json += "}";

    server.send(
        200,
        "application/json",
        json);
  }

  // =========================================================
  // RELAY
  // =========================================================

  static void handleRelay()
  {
    sendCors();

    if (!server.hasArg("state"))
    {
      server.send(
          400,
          "application/json",
          "{\"ok\":false,\"message\":\"Missing state\"}");

      return;
    }

    String state =
        server.arg("state");

    state.toLowerCase();

    if (state == "on")
    {
      load_relay::turnOn();

      storage::logEvent(
          "RELAY",
          "Relay requested ON from dashboard.");
    }
    else if (state == "off")
    {
      load_relay::turnOff();

      storage::logEvent(
          "RELAY",
          "Relay requested OFF from dashboard.");
    }
    else
    {
      server.send(
          400,
          "application/json",
          "{\"ok\":false,\"message\":\"Invalid state\"}");

      return;
    }

    server.send(
        200,
        "application/json",
        "{\"ok\":true}");
  }

  // =========================================================
  // LOG NOW
  // =========================================================

  static void handleLogNow()
  {
    sendCors();

    storage::logNow();

    server.send(
        200,
        "application/json",
        "{\"ok\":true}");
  }

  // =========================================================
  // SAVE SETTINGS
  // =========================================================

  static void handleSaveSettings()
  {
    sendCors();

    const char *requiredArgs[] =
        {
            "full_distance",
            "low_distance",
            "full_level",
            "low_level",
            "tank_latch",
            "low_temperature",
            "high_temperature",
            "temperature_latch"};

    for (size_t i = 0;
         i < 8;
         i++)
    {
      if (!server.hasArg(
              requiredArgs[i]))
      {
        server.send(
            400,
            "application/json",
            "{\"ok\":false,\"message\":\"Missing setting parameter\"}");

        return;
      }
    }

    // -----------------------------------------------------
    // Parse
    // -----------------------------------------------------

    float fullDistance =
        server.arg(
                  "full_distance")
            .toFloat();

    float lowDistance =
        server.arg(
                  "low_distance")
            .toFloat();

    float fullLevel =
        server.arg(
                  "full_level")
            .toFloat();

    float lowLevel =
        server.arg(
                  "low_level")
            .toFloat();

    int tankLatch =
        server.arg(
                  "tank_latch")
            .toInt();

    float lowTemperature =
        server.arg(
                  "low_temperature")
            .toFloat();

    float highTemperature =
        server.arg(
                  "high_temperature")
            .toFloat();

    int temperatureLatch =
        server.arg(
                  "temperature_latch")
            .toInt();

    // -----------------------------------------------------
    // Validate
    // -----------------------------------------------------

    if (!isfinite(fullDistance) ||
        !isfinite(lowDistance) ||
        !isfinite(fullLevel) ||
        !isfinite(lowLevel) ||
        !isfinite(lowTemperature) ||
        !isfinite(highTemperature))
    {
      server.send(
          400,
          "application/json",
          "{\"ok\":false,\"message\":\"Invalid numeric value\"}");

      return;
    }

    if (fullDistance <= 0.0f ||
        lowDistance <= fullDistance ||
        lowDistance > 500.0f)
    {
      server.send(
          400,
          "application/json",
          "{\"ok\":false,\"message\":\"Full distance must be smaller than low distance\"}");

      return;
    }

    if (fullLevel <= lowLevel ||
        fullLevel > 100.0f ||
        lowLevel < 0.0f)
    {
      server.send(
          400,
          "application/json",
          "{\"ok\":false,\"message\":\"Invalid level thresholds\"}");

      return;
    }

    if (tankLatch < 0 ||
        tankLatch > 2)
    {
      server.send(
          400,
          "application/json",
          "{\"ok\":false,\"message\":\"Invalid tank latch mode\"}");

      return;
    }

    if (lowTemperature >= highTemperature)
    {
      server.send(
          400,
          "application/json",
          "{\"ok\":false,\"message\":\"Low temperature must be below high temperature\"}");

      return;
    }

    if (lowTemperature < -200.0f ||
        highTemperature > 850.0f)
    {
      server.send(
          400,
          "application/json",
          "{\"ok\":false,\"message\":\"Temperature outside supported range\"}");

      return;
    }

    if (temperatureLatch < 0 ||
        temperatureLatch > 2)
    {
      server.send(
          400,
          "application/json",
          "{\"ok\":false,\"message\":\"Invalid temperature latch mode\"}");

      return;
    }

    // -----------------------------------------------------
    // Persist FIRST
    //
    // Runtime settings are only changed after the flash
    // write succeeds.
    // -----------------------------------------------------

    bool saved =
        storage::saveTankSettings(
            fullDistance,
            lowDistance,
            fullLevel,
            lowLevel,
            static_cast<uint8_t>(
                tankLatch),
            lowTemperature,
            highTemperature,
            static_cast<uint8_t>(
                temperatureLatch));

    if (!saved)
    {
      server.send(
          500,
          "application/json",
          "{\"ok\":false,\"message\":\"Could not save settings to internal flash\"}");

      return;
    }

    // -----------------------------------------------------
    // Apply runtime settings
    // -----------------------------------------------------

    bool applied =
        device_manager::applySettings(
            fullDistance,
            lowDistance,
            fullLevel,
            lowLevel,
            static_cast<
                device_manager::
                    RelayLatchMode>(tankLatch),
            lowTemperature,
            highTemperature,
            static_cast<
                device_manager::
                    TemperatureLatchMode>(temperatureLatch));

    if (!applied)
    {
      server.send(
          500,
          "application/json",
          "{\"ok\":false,\"message\":\"Settings were saved but could not be applied\"}");

      return;
    }

    storage::logEvent(
        "SETTINGS",
        "Dashboard settings saved and applied.");

    server.send(
        200,
        "application/json",
        "{\"ok\":true,\"message\":\"Settings saved and applied\"}");
  }

  // =========================================================
  // CSV STREAM
  // =========================================================

  static void streamCsv(
      const char *path)
  {
    sendCors();

    File file =
        storage::openRead(path);

    if (!file)
    {
      server.send(
          404,
          "text/plain",
          "File not found");

      return;
    }

    server.streamFile(
        file,
        "text/csv");

    file.close();
  }

  static void handleLiquidDownload()
  {
    streamCsv(
        storage::
            getLiquidLogFileName());
  }

  static void handleAnalysisDownload()
  {
    streamCsv(
        storage::
            getAnalysisLogFileName());
  }

  static void handleEventDownload()
  {
    streamCsv(
        storage::
            getEventLogFileName());
  }

  // =========================================================
  // BEGIN
  // =========================================================

  void begin()
  {
    WiFi.mode(WIFI_AP);

    WiFi.softAP(
        apSsid,
        apPassword);

    ipAddress =
        WiFi.softAPIP()
            .toString();

    server.on(
        "/",
        HTTP_GET,
        handleRoot);

    server.on(
        "/api/status",
        HTTP_GET,
        handleStatus);

    server.on(
        "/api/logs",
        HTTP_GET,
        handleLogs);

    server.on(
        "/api/relay",
        HTTP_GET,
        handleRelay);

    server.on(
        "/api/log_now",
        HTTP_GET,
        handleLogNow);

    server.on(
        "/api/settings/save",
        HTTP_GET,
        handleSaveSettings);

    server.on(
        "/download/motor_log.csv",
        HTTP_GET,
        handleLiquidDownload);

    server.on(
        "/download/analysis_log.csv",
        HTTP_GET,
        handleAnalysisDownload);

    server.on(
        "/download/event_log.csv",
        HTTP_GET,
        handleEventDownload);

    server.begin();

    running = true;
  }

  // =========================================================
  // UPDATE
  // =========================================================

  void update()
  {
    if (running)
    {
      server.handleClient();
    }
  }

  // =========================================================
  // STATUS
  // =========================================================

  bool isRunning()
  {
    return running;
  }

  String getIp()
  {
    return ipAddress;
  }

  String getSsid()
  {
    return String(apSsid);
  }
}
