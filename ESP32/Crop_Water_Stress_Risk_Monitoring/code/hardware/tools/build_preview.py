"""Build a standalone browser preview from the production portal assets."""
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]


def render():
    portal = ROOT / 'data' / 'portal'
    preview = ROOT / 'UI'
    page = (portal / 'index.html').read_text(encoding='utf-8')
    css = (portal / 'styles.css').read_text(encoding='utf-8')
    controls = (preview / 'controls.html').read_text(encoding='utf-8')
    script = (preview / 'preview.js').read_text(encoding='utf-8')
    app = (portal / 'app.js').read_text(encoding='utf-8')
    defaults = json.loads((ROOT / 'examples' / 'configuration.json').read_text(encoding='utf-8'))
    page = page.replace('<link rel="stylesheet" href="/styles.css">', '<style>' + css + '</style>')
    page = page.replace('<script src="/app.js" defer></script>', '')
    page = page.replace('<body>', '<body>\n' + controls, 1)
    scripts = 'window.CWS_PREVIEW_DEFAULTS = ' + json.dumps(defaults) + ';\n' + script
    # Keep embedded JavaScript inside its own HTML script element.
    embed = lambda value: '<script>\n' + value.replace('</script', '<\\/script') + '\n</script>\n'
    page = page.replace('</body>', embed(scripts) + embed(app) + '</body>')
    return '<!-- Built by tools/build_preview.py from the production portal. -->\n' + page


if __name__ == '__main__':
    target = ROOT / 'UI' / 'test.html'
    target.parent.mkdir(exist_ok=True)
    target.write_text(render(), encoding='utf-8')
    print('Created UI/test.html from data/portal assets.')
