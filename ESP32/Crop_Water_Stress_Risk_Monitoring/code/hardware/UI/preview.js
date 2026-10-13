/* Browser-only device fixture. This file is excluded from firmware portal assets. */
(() => {
  'use strict';
  const copy = value => JSON.parse(JSON.stringify(value));
  const demoPassword = 'Preview-admin!42';
  const scenario = new URL(location.href).searchParams.get('scenario') || 'configured';
  const state = {
    config: copy(window.CWS_PREVIEW_DEFAULTS), password: demoPassword,
    authenticated: false, mustChange: scenario === 'first-time',
    csrf: 'preview-csrf-only', expires: performance.now() + 300000,
    trial: null, trialState: 'idle', trialError: '', job: null,
    attempts: 0, retryAt: 0, scanUntil: 0,
  };
  delete window.CWS_PREVIEW_DEFAULTS;
  if (scenario !== 'first-time') {
    state.config.revision = 4;
    Object.assign(state.config.wifi, {ssid: 'Farm Router', password_set: true});
    Object.assign(state.config.mqtt, {host: 'mqtt.example.org', username: 'cws-device-001', ca_configured: true, password_set: true});
  }
  let wifi = scenario === 'configured', mqtt = wifi;
  const remaining = () => Math.max(0, Math.ceil((state.expires - performance.now()) / 1000));
  const reply = (body, status = 200) => new Response(JSON.stringify(body), {status, headers: {'Content-Type': 'application/json'}});
  const error = (message, status = 400) => reply({error: message}, status);
  const strong = password => typeof password === 'string' && /^[\x20-\x7e]{12,96}$/.test(password) && /[a-z]/.test(password) && /[A-Z]/.test(password) && /[0-9]/.test(password) && /[!-/:-@[-`{-~]/.test(password);
  const ipValue = value => {
    if (typeof value !== 'string' || !/^\d{1,3}(\.\d{1,3}){3}$/.test(value)) return null;
    const octets = value.split('.').map(Number);
    return octets.some(n => n > 255) ? null : octets.reduce((n, b) => (n * 256 + b) >>> 0, 0);
  };
  function validate(kind, settings) {
    if (kind === 'wifi') {
      if (!settings.ssid || new TextEncoder().encode(settings.ssid).length > 32) return 'wifi_invalid';
      if (settings.password && (settings.password.length < 8 || settings.password.length > 63)) return 'wifi_password_length';
      if (!settings.dhcp) {
        const values = ['ip', 'gateway', 'subnet', 'dns'].map(key => ipValue(settings[key]));
        if (values.some(v => v === null)) return 'static_ipv4_invalid';
        const [ip, gateway, mask, dns] = values, host = (~mask) >>> 0;
        if (!mask || host < 3 || (host & (host + 1)) || (ip & mask) !== (gateway & mask) ||
            !(ip & host) || (ip & host) === host || !(gateway & host) || (gateway & host) === host ||
            ip === gateway || !dns || (ip >>> 24) === 127 || (ip >>> 24) >= 224 ||
            (ip >>> 8) === 0xc0a804) return 'static_network_invalid';
      }
    } else {
      if (!settings.host || !/^[a-z0-9.-]+$/i.test(settings.host) || settings.host.length > 128) return 'mqtt_hostname_invalid';
      if (!Number.isInteger(settings.port) || settings.port < 1 || settings.port > 65535 || settings.timeout_s < 5 || settings.timeout_s > 30) return 'mqtt_invalid';
      if (settings.ca && (!settings.ca.includes('-----BEGIN CERTIFICATE-----') || !settings.ca.includes('-----END CERTIFICATE-----') || settings.ca.length > 6000)) return 'ca_certificate_invalid';
      if (!settings.ca && !state.config.mqtt.ca_configured) return 'broker_and_ca_required';
    }
    return '';
  }
  function settleTrial() {
    const t = state.trial;
    if (!t || performance.now() < t.until) return;
    if (t.success && t.save) {
      const next = copy(t.settings);
      if (next.password) next.password_set = true;
      delete next.password;
      if (t.clearPassword) next.password_set = false;
      if (next.ca) next.ca_configured = true;
      delete next.ca;
      Object.assign(state.config[t.kind], next);
      state.config.revision++;
      if (t.kind === 'wifi') { wifi = true; mqtt = false; }
      else mqtt = wifi;
    }
    state.trialState = !t.success ? 'failed_rolled_back' : !t.save ? 'test_passed_restored_previous' : t.kind === 'wifi' ? 'wifi_saved_backend_pending_validation' : 'saved';
    state.trialError = t.success ? '' : 'connection_trial_failed';
    state.trial = null;
  }
  function status() {
    return {
      device_id: 'CWS-7C9EBD42A610', firmware_version: '1.0.0', configuration_revision: state.config.revision,
      remaining_s: remaining(), mqtt_host: state.config.mqtt.host,
      trial_state: state.trialState, trial_error: state.trialError,
      connectivity: {ssid: state.config.wifi.ssid, wifi_connected: wifi, ip_assigned: wifi,
        ip: wifi ? (state.config.wifi.dhcp ? '192.168.1.84' : state.config.wifi.ip) : '0.0.0.0',
        mqtt_connected: mqtt, mqtt_subscribed: mqtt, tls_verified: mqtt, last_connection_error: state.trialError ? 4 : 0},
      storage: {pending_records: mqtt ? 0 : 12},
    };
  }
  window.fetch = async (url, options = {}) => {
    // Never forward requests: preview submissions remain in this page's memory.
    const action = String(url).replace(/^\/api\//, '');
    if (!String(url).startsWith('/api/')) return error('preview_network_access_disabled', 403);
    settleTrial();
    if (!remaining()) { state.authenticated = false; state.job = null; return error('setup_window_expired', 403); }
    const post = options.method === 'POST';
    let body = {};
    try { if (post) body = JSON.parse(options.body || '{}'); } catch { return error('invalid_json'); }
    if (action === 'session') return reply({remaining_s: remaining(), authenticated: state.authenticated, must_change_password: state.mustChange, csrf: state.csrf});
    if (action === 'login' || action === 'password') {
      if (action === 'password' && (!state.authenticated || new Headers(options.headers).get('X-CSRF-Token') !== state.csrf)) return error('authentication_required', 401);
      if (performance.now() < state.retryAt) return error('login_throttled', 429);
      if (state.job) return error('authentication_busy', 409);
      const password = action === 'login' ? body.password : body.current;
      if (password !== state.password) {
        state.attempts++;
        state.retryAt = performance.now() + Math.min(60000, 1000 * 2 ** state.attempts);
        return error('authentication_failed', 401);
      }
      if (action === 'password' && (!strong(body.next) || body.next !== body.confirm || body.next === body.current)) return error('password_strength_or_confirmation_invalid');
      state.job = {kind: action, next: body.next, until: performance.now() + 700};
      return reply({pending: true, job_id: 'preview-job'}, 202);
    }
    if (action === 'auth_result') {
      const job = state.job;
      if (!job || body.job_id !== 'preview-job') return error('authentication_job_not_found', 401);
      if (performance.now() < job.until) return reply({pending: true}, 202);
      state.job = null; state.attempts = 0;
      if (job.kind === 'password') {
        state.password = job.next; state.mustChange = false; state.authenticated = false;
        return reply({ok: true});
      }
      state.authenticated = true;
      return reply({must_change_password: state.mustChange, csrf: state.csrf});
    }
    if (!state.authenticated || (post && new Headers(options.headers).get('X-CSRF-Token') !== state.csrf)) return error('authentication_required', 401);
    if (action === 'logout') { state.authenticated = false; state.job = null; return reply({ok: true}); }
    if (state.mustChange) return error('password_replacement_required', 403);
    if (action === 'config') return reply(state.config);
    if (action === 'status') return reply(status());
    if (action === 'scan') {
      if (post) state.scanUntil = performance.now() + 900;
      return reply({scanning: performance.now() < state.scanUntil, networks: [
        {ssid: 'Farm Router', rssi_dbm: -46, secured: true},
        {ssid: 'Field Hotspot', rssi_dbm: -68, secured: true},
        {ssid: 'Guest Network', rssi_dbm: -77, secured: false},
      ]});
    }
    if (action === 'trial') {
      if (state.trial) return error('configuration_trial_busy', 409);
      if (body.expected_revision !== state.config.revision) return error('revision_conflict', 409);
      if (!['wifi', 'mqtt'].includes(body.kind) || !body.settings) return error('invalid_trial');
      const problem = validate(body.kind, body.settings);
      if (problem) return error(problem);
      state.trial = {kind: body.kind, settings: copy(body.settings), save: body.save, clearPassword: body.clear_password,
        success: document.getElementById('preview-outcome').value === 'success' && (body.kind === 'wifi' || wifi), until: performance.now() + 4000};
      state.trialState = 'testing_' + body.kind; state.trialError = '';
      return reply({state: state.trialState}, 202);
    }
    if (action === 'cancel') {
      if (!state.trial) return error('no_active_trial', 409);
      state.trial = null; state.trialState = 'cancelled'; state.trialError = '';
      return reply({ok: true});
    }
    if (action === 'diagnostics') return reply({
      ...status().connectivity, wifi_association: wifi ? 'passed' : 'not_connected',
      dns_tcp_tls: mqtt ? 'passed' : 'not_verified', mqtt_authentication: mqtt ? 'passed' : 'not_verified',
      mqtt_subscription: mqtt ? 'passed' : 'not_verified', telemetry_publication: mqtt ? 'broker_acknowledged' : 'not_verified',
      backend_delivery: {last_acknowledged_record: mqtt ? 'CWS-7C9EBD42A610-5d8af0912e7c634b-42' : '', pending_records: mqtt ? 0 : 12},
    });
    return error('not_found', 404);
  };
  document.getElementById('preview-tools').hidden = new URL(location.href).searchParams.get('review') !== '1';
  const selector = document.getElementById('preview-scenario');
  selector.value = ['configured', 'offline', 'first-time'].includes(scenario) ? scenario : 'configured';
  selector.addEventListener('change', () => {
    const next = new URL(location.href); next.searchParams.set('scenario', selector.value); location.href = next.href;
  });
  document.getElementById('preview-reset').addEventListener('click', () => location.reload());
  document.getElementById('preview-fill').addEventListener('click', () => {
    document.getElementById('login-password').value = demoPassword;
    document.getElementById('login-password').focus();
  });
  document.getElementById('preview-expire').addEventListener('click', () => {
    state.expires = performance.now(); state.authenticated = false; state.job = null;
    // Exercise the production countdown closure immediately for visual review.
    deadline = performance.now(); countdown();
  });
})();
