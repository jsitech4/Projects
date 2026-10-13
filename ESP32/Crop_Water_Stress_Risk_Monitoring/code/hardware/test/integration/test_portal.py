"""Browser UI tests with an isolated protocol fixture, not a hardware simulator."""
import copy
import json
import os
from pathlib import Path
import pytest
from playwright.sync_api import sync_playwright, expect

ROOT=Path(__file__).resolve().parents[2]


@pytest.fixture(scope='module')
def browser():
    with sync_playwright() as p:
        browser=p.chromium.launch(channel=os.environ.get('CWS_TEST_BROWSER','msedge'),headless=True)
        yield browser
        browser.close()


@pytest.fixture
def ui(browser):
    context=browser.new_context(viewport={'width':1280,'height':900})
    page=context.new_page()
    config=json.loads((ROOT/'examples/configuration.json').read_text())
    status=json.loads((ROOT/'examples/telemetry.json').read_text())
    status['remaining_s']=299
    state={'authenticated':False,'must_change':False,'config':config,'status':status,'trials':[],'errors':[],'async_auth':False,'auth_polls':0}
    page.on('pageerror',lambda error:state['errors'].append(str(error)))
    def route(request):
        path=request.request.url.removeprefix('http://192.168.4.1')
        if not path.startswith('/api/'):
            file='index.html' if path=='/' else path[1:]
            if file not in ['index.html','styles.css','app.js']:
                request.fulfill(status=404,body='');return
            request.fulfill(body=(ROOT/'data/portal'/file).read_text(encoding='utf-8'),content_type={'index.html':'text/html','styles.css':'text/css','app.js':'text/javascript'}[file]);return
        action=path[5:];data=request.request.post_data_json if request.request.method=='POST' else None
        code=200
        result={}
        if action=='session':result={'remaining_s':state['status']['remaining_s'],'authenticated':state['authenticated'],'must_change_password':state['must_change'],'csrf':'fixture-csrf'}
        elif action=='login':
            if data['password']!='Fixture-password!42':code=401;result={'error':'authentication_failed'}
            elif state['async_auth']:code=202;result={'pending':True,'job_id':'fixture-job'}
            else:state['authenticated']=True;result={'must_change_password':state['must_change'],'csrf':'fixture-csrf'}
        elif action=='auth_result':
            assert data['job_id']=='fixture-job'
            state['auth_polls']+=1
            if state['auth_polls']<2:code=202;result={'pending':True}
            else:state['authenticated']=True;result={'must_change_password':state['must_change'],'csrf':'fixture-csrf'}
        elif action=='config':result=state['config']
        elif action=='status':result=state['status']
        elif action=='trial':
            assert request.request.headers.get('x-csrf-token')=='fixture-csrf'
            state['trials'].append(data);result={'state':'testing'};code=202
        elif action=='scan':result={'scanning':False,'networks':[{'ssid':'Test farm router','rssi_dbm':-45,'secured':True}]}
        elif action=='password':
            assert data['current']=='Fixture-password!42'
            assert data['next']==data['confirm']
            state['authenticated']=False;state['must_change']=False;result={'ok':True}
        elif action=='logout':state['authenticated']=False;result={'ok':True}
        elif action=='diagnostics':result={'wifi_association':'not_connected','ip_assigned':False,'dns_tcp_tls':'not_verified','mqtt_authentication':'not_verified','mqtt_subscription':'not_verified','telemetry_publication':'not_verified','backend_delivery':{'last_acknowledged_record':'','pending_records':0},'last_connection_error':0}
        elif action=='cancel':result={'ok':True}
        else:code=404;result={'error':'not_found'}
        request.fulfill(status=code,json=result)
    page.route('http://192.168.4.1/**',route)
    yield page,state
    assert not state['errors']
    context.close()


def login(page):
    page.goto('http://192.168.4.1/')
    page.get_by_label('Administrator password',exact=True).fill('Fixture-password!42')
    page.get_by_role('button',name='Sign in',exact=True).click()
    expect(page.locator('#admin-panel')).to_be_visible()


def test_login_error(ui):
    page,state=ui;page.goto('http://192.168.4.1/')
    page.locator('#login-password').fill('incorrect')
    page.get_by_role('button',name='Sign in',exact=True).click()
    expect(page.locator('#message')).to_contain_text('not accepted')
    assert not state['authenticated']


def test_asynchronous_authentication(ui):
    page,state=ui;state['async_auth']=True;login(page)
    assert state['authenticated'] and state['auth_polls']==2


def test_required_factory_replacement(ui):
    page,state=ui;state['must_change']=True;login(page)
    expect(page.locator('#security')).to_be_visible()
    expect(page.get_by_role('button',name='Wi-Fi',exact=True)).to_be_disabled()
    page.locator('#current').fill('Fixture-password!42')
    page.locator('#next').fill('Replacement-password!42')
    page.locator('#confirm').fill('Replacement-password!42')
    page.get_by_role('button',name='Save password').click()
    expect(page.locator('#login-panel')).to_be_visible()
    assert not state['authenticated']


@pytest.mark.parametrize('width',[320,768,1280])
def test_responsive_views(ui,width):
    page,state=ui;page.set_viewport_size({'width':width,'height':900});login(page)
    for name in ['Overview','Wi-Fi','MQTT & TLS','Security','Diagnostics']:
        page.get_by_role('button',name=name,exact=True).click()
        assert page.evaluate('document.documentElement.scrollWidth <= window.innerWidth')
    page.get_by_role('button',name='Wi-Fi',exact=True).click()
    directory=ROOT/'test-results/portal';directory.mkdir(parents=True,exist_ok=True)
    page.screenshot(path=str(directory/f'wifi-{width}.png'),full_page=True)
    page.get_by_role('button',name='MQTT & TLS',exact=True).click()
    page.screenshot(path=str(directory/f'mqtt-{width}.png'),full_page=True)


def test_wifi_trial_and_password_retention(ui):
    page,state=ui;login(page);page.get_by_role('button',name='Wi-Fi',exact=True).click()
    page.locator('#ssid').fill('Test farm router')
    page.locator('#wifi-form').get_by_role('button',name='Test & save changes').click()
    expect(page.locator('#message')).to_contain_text('saved only after')
    trial=state['trials'][-1]
    assert trial['save'] is True and trial['kind']=='wifi'
    assert trial['settings']['password']=='' and trial['clear_password'] is False
    assert trial['expected_revision']==0


def test_static_address_fields(ui):
    page,state=ui;login(page);page.get_by_role('button',name='Wi-Fi',exact=True).click()
    page.locator('#dhcp').uncheck()
    expect(page.locator('#static-fields')).to_be_visible()
    page.locator('#ssid').fill('Test farm router')
    for key,value in {'ip':'192.168.8.20','gateway':'192.168.8.1','subnet':'255.255.255.0','dns':'1.1.1.1'}.items():page.locator('#'+key).fill(value)
    page.get_by_role('button',name='Test connection',exact=True).click()
    expect(page.locator('#message')).to_contain_text('Previous settings will be restored')
    assert state['trials'][-1]['settings']['dhcp'] is False


def test_scan_populates_real_response(ui):
    page,state=ui;login(page);page.get_by_role('button',name='Wi-Fi',exact=True).click()
    page.get_by_role('button',name='Scan networks').click()
    expect(page.locator('#message')).to_contain_text('Network scan complete')
    page.locator('#networks').select_option('Test farm router')
    expect(page.locator('#ssid')).to_have_value('Test farm router')


def test_mqtt_empty_ca_retains_existing(ui):
    page,state=ui;login(page);page.get_by_role('button',name='MQTT & TLS',exact=True).click()
    page.locator('#host').fill('mqtt.example.org')
    page.get_by_role('button',name='Test server connection').click()
    expect(page.locator('#message')).to_contain_text('Previous settings will be restored')
    assert 'ca' not in state['trials'][-1]['settings']


def test_countdown_closes_ui(ui):
    page,state=ui;state['status']['remaining_s']=1;page.goto('http://192.168.4.1/')
    expect(page.locator('#expired-panel')).to_be_visible(timeout=5000)
