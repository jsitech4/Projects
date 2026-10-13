"""Embed offline portal assets and optional private commissioning material."""
Import("env")
from pathlib import Path
import json
import os

root = Path(env.subst("$PROJECT_DIR"))
header = '#pragma once\n#include <Arduino.h>\nnamespace cws {\n'
for name, filename in [('PortalHtml', 'index.html'), ('PortalCss', 'styles.css'), ('PortalJs', 'app.js')]:
    content = (root / 'data' / 'portal' / filename).read_text(encoding='utf-8')
    header += f'inline const char {name}[] PROGMEM = R"CWSASSET({content})CWSASSET";\n'
(root / 'include' / 'GeneratedAssets.h').write_text(header + '}\n', encoding='utf-8')
private = os.environ.get('CWS_FACTORY_FILE')
values = json.loads(Path(private).read_text()) if private else {}
if private:
    required = {'ap_password': None, 'admin_salt': 32, 'admin_verifier': 64, 'service_verifier': 64}
    for key, length in required.items():
        if not isinstance(values.get(key), str) or (length and len(values[key]) != length):
            raise ValueError('Incomplete private commissioning material')
    if any(not 33 <= ord(c) <= 126 for c in values['ap_password']):
        raise ValueError('Setup credential must contain printable ASCII without whitespace')
header = '#pragma once\nnamespace cws {\n'
for key in ['ap_password', 'admin_salt', 'admin_verifier', 'service_verifier']:
    value = values.get(key, '')
    if any(c not in '0123456789abcdef' for c in value) and key != 'ap_password':
        raise ValueError('Invalid private verifier format')
    if key == 'ap_password' and value and not 12 <= len(value) <= 63:
        raise ValueError('Invalid setup password length')
    header += f'inline const char factory_{key}[] = {json.dumps(value)};\n'
(root / 'include' / 'FactorySecrets.h').write_text(header + '}\n', encoding='utf-8')
