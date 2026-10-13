"""Exercise the standalone preview through file:// without a device or web server."""
import importlib.util
import os
from pathlib import Path
import pytest
from playwright.sync_api import expect, sync_playwright

ROOT = Path(__file__).resolve().parents[2]


def test_preview_matches_production_assets():
    spec = importlib.util.spec_from_file_location('build_preview', ROOT / 'tools/build_preview.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    assert (ROOT / 'UI/test.html').read_text(encoding='utf-8') == module.render()


@pytest.fixture(scope='module')
def preview_browser():
    with sync_playwright() as p:
        browser = p.chromium.launch(channel=os.environ.get('CWS_TEST_BROWSER', 'chrome'), headless=True)
        yield browser
        browser.close()


@pytest.fixture
def preview(preview_browser):
    page = preview_browser.new_page(viewport={'width': 1280, 'height': 900})
    errors, external = [], []
    page.on('pageerror', lambda error: errors.append(str(error)))
    page.on('request', lambda request: external.append(request.url) if request.url.startswith(('http:', 'https:')) else None)
    yield page
    page.close()
    assert not errors
    assert not external


def open_preview(page, scenario='configured', review=True):
    page.goto((ROOT / 'UI/test.html').as_uri() + '?scenario=' + scenario + ('&review=1' if review else ''))


def sign_in(page, password='Preview-admin!42'):
    page.locator('#login-password').fill(password)
    page.get_by_role('button', name='Sign in', exact=True).click()
    expect(page.locator('#admin-panel')).to_be_visible()


def test_preview_scan_save_and_diagnostics(preview):
    open_preview(preview)
    preview.get_by_role('button', name='Fill demo password').click()
    expect(preview.locator('#login-password')).to_have_value('Preview-admin!42')
    sign_in(preview)
    preview.get_by_role('button', name='Wi-Fi', exact=True).click()
    preview.get_by_role('button', name='Scan networks').click()
    expect(preview.locator('#message')).to_contain_text('Network scan complete')
    preview.locator('#networks').select_option('Field Hotspot')
    preview.locator('#wifi-form').get_by_role('button', name='Test & save changes').click()
    expect(preview.locator('#trial-status')).to_contain_text('wifi saved backend pending validation', timeout=12000)
    expect(preview.locator('#overview-values')).to_contain_text('Field Hotspot')
    preview.get_by_role('button', name='Diagnostics', exact=True).click()
    expect(preview.locator('#diagnostic-values')).to_contain_text('not_verified')


def test_preview_failed_trial_preserves_configuration(preview):
    open_preview(preview)
    sign_in(preview)
    preview.locator('#preview-outcome').select_option('failure')
    preview.get_by_role('button', name='Wi-Fi', exact=True).click()
    preview.locator('#ssid').fill('Rejected demo router')
    preview.locator('#wifi-form').get_by_role('button', name='Test & save changes').click()
    expect(preview.locator('#trial-status')).to_contain_text('failed rolled back', timeout=12000)
    preview.get_by_role('button', name='Wi-Fi', exact=True).click()
    preview.get_by_role('button', name='Cancel edits', exact=True).click()
    expect(preview.locator('#ssid')).to_have_value('Farm Router')


def test_preview_first_login_password_replacement(preview):
    open_preview(preview, 'first-time')
    sign_in(preview)
    expect(preview.get_by_role('button', name='Wi-Fi', exact=True)).to_be_disabled()
    preview.locator('#current').fill('Preview-admin!42')
    preview.locator('#next').fill('New-preview-admin!42')
    preview.locator('#confirm').fill('New-preview-admin!42')
    preview.get_by_role('button', name='Save password').click()
    expect(preview.locator('#login-panel')).to_be_visible()
    preview.locator('#login-password').fill('Preview-admin!42')
    preview.get_by_role('button', name='Sign in', exact=True).click()
    expect(preview.locator('#message')).to_contain_text('not accepted')
    preview.wait_for_timeout(2100)
    sign_in(preview, 'New-preview-admin!42')
    expect(preview.get_by_role('button', name='Wi-Fi', exact=True)).to_be_enabled()


def test_preview_expiration_and_reset(preview):
    open_preview(preview)
    sign_in(preview)
    preview.get_by_role('button', name='Expire setup window').click()
    expect(preview.locator('#expired-panel')).to_be_visible()
    preview.get_by_role('button', name='Reset preview').click()
    expect(preview.locator('#login-panel')).to_be_visible()
    sign_in(preview)


@pytest.mark.parametrize('width', [320, 768, 1280])
def test_preview_responsive_layout(preview, width):
    preview.set_viewport_size({'width': width, 'height': 900})
    open_preview(preview, review=False)
    sign_in(preview)
    for section in ['Overview', 'Wi-Fi', 'MQTT & TLS', 'Security', 'Diagnostics']:
        preview.get_by_role('button', name=section, exact=True).click()
        assert preview.evaluate('document.documentElement.scrollWidth <= window.innerWidth')
    preview.get_by_role('button', name='MQTT & TLS', exact=True).click()
    directory = ROOT / 'test-results/preview'
    directory.mkdir(parents=True, exist_ok=True)
    preview.screenshot(path=str(directory / f'preview-{width}.png'), full_page=True)


def test_client_view_has_only_admin_interface(preview):
    preview.goto((ROOT / 'UI/test.html').as_uri())
    expect(preview).to_have_title('CWS · Device setup')
    expect(preview.locator('#preview-tools')).to_be_hidden()
    assert 'simulated' not in preview.locator('body').inner_text().lower()
    sign_in(preview)
    for section in ['Overview', 'Wi-Fi', 'MQTT & TLS', 'Security', 'Diagnostics']:
        preview.get_by_role('button', name=section, exact=True).click()
        visible = preview.locator('body').inner_text().lower()
        assert not any(label in visible for label in ['simulated', 'preview', 'demo'])
