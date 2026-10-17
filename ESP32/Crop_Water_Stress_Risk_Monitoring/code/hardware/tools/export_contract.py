"""Regenerate versioned protocol schemas and non-secret integration fixtures."""
import copy
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
META = {'$schema':'https://json-schema.org/draft/2020-12/schema'}


def obj(properties, required=None, extra=False):
    return {'type':'object','properties':properties,'required':list(properties) if required is None else required,'additionalProperties':extra}


def integer(low=0, high=4294967295):
    return {'type':'integer','minimum':low,'maximum':high}


def number(low, high, nullable=False):
    return {'type':['number','null'] if nullable else 'number','minimum':low,'maximum':high}


def string(maximum=128):
    return {'type':'string','maxLength':maximum}


BOOL = {'type':'boolean'}
IDENTIFIER = {'type':'string','pattern':'^[A-Za-z0-9_-]+$','minLength':1,'maxLength':79}


def write(name, value, directory='schemas'):
    target = ROOT / directory / name
    target.parent.mkdir(exist_ok=True)
    target.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def main():
    source = (ROOT / 'lib/System/Commands.cpp').read_text()
    table = re.search(r'CommandNames\[\]\s*=\s*\{(.*?)\};', source, re.S).group(1)
    names = re.findall(r'"([a-z]+\.[a-z_]+)"', table)
    core = (ROOT / 'lib/Core/Core.cpp').read_text()
    defaults = json.loads(re.search(r'R"\((\{"revision".*?)\)"', core, re.S).group(1))
    config_properties = {}
    for key, value in defaults.items():
        if isinstance(value, dict):
            config_properties[key] = obj({k: BOOL if isinstance(v,bool) else number(-1e8,1e8) if isinstance(v,(int,float)) else number(0,4095,True) if v is None else string(6000 if k=='ca' else 128) for k,v in value.items()}, required=[])
        else:
            config_properties[key] = integer() if isinstance(value,int) else string(48)
    for key in ['sample_s','upload_s','health_s']:
        config_properties[key] = integer(5 if key=='sample_s' else 10,86400)
    config_properties['mqtt']['properties'].update({'port':integer(1,65535),'timeout_s':integer(5,30),'prefix':{'const':'cws/v1/devices'}})
    config_properties['wifi']['properties'].update({'ssid':string(32),'password':string(63)})
    config_properties['storage']['properties']={'retain':integer(8,256),'drop_oldest':BOOL,'durable_ack':BOOL}
    config_properties['environment']['properties']={'enabled':BOOL,'temperature_offset':number(-10,10),'humidity_offset':number(-20,20)}
    for kind in ['battery','solar']:
        config_properties[kind]['properties']={'top_ohm':number(0,1e8),'bottom_ohm':number(0,1e8),'gain':number(.1,10),'offset':number(-10,10),'min_mv':number(150,2400),'max_mv':number(200,2450),'low_v':number(0,1000),'critical_v':number(0,1000)}
    config_properties['lcd']['properties'].update({'address':integer(32,63),'timeout_s':integer(0,3600),'page':integer(0,2)})
    patch_properties = {k:v for k,v in config_properties.items() if k not in ['revision','calibration_revision']}
    remote_patch_properties = {k:v for k,v in patch_properties.items()
                               if k not in ['wifi','mqtt','environment','soil','battery','solar']}
    write('configuration_patch.schema.json', META | obj(remote_patch_properties, required=[]))

    envelope = {'schema_version':{'const':'1.0'},'message_type':string(32),'message_id':IDENTIFIER,'device_id':IDENTIFIER,'boot_id':{'type':'string','pattern':'^[a-f0-9]{16}$'},'sequence':integer(1),'time':obj({'uptime_ms':integer(0,9007199254740991),'synchronized':BOOL,'unix_s':{'type':['integer','null'],'minimum':1700000000}})}
    def message(kind, fields, required=None):
        props=copy.deepcopy(envelope);props['message_type']={'const':kind};props.update(fields)
        return META | obj(props,list(envelope)+(list(fields) if required is None else required),extra=False)

    fault = obj({'code':string(64),'severity':{'enum':['warning','critical']},'since_uptime_ms':integer(),'occurrences_since_boot':integer()})
    health = obj({'uptime_ms':integer(0,9007199254740991),'reset_reason':integer(),'boot_count':integer(),'free_heap':integer(),'minimum_free_heap':integer(),'flash_bytes':integer(),'firmware_bytes':integer(),'operating_state':string(32),'active_faults':{'type':'array','items':fault}})
    connectivity = obj({'wifi_connected':BOOL,'ssid':string(32),'ip':string(15),'ip_assigned':BOOL,'rssi_dbm':{'type':'integer','minimum':-127,'maximum':0},'mqtt_connected':BOOL,'mqtt_subscribed':BOOL,'tls_verified':BOOL,'last_connection_error':integer(),'reconnection_count':integer(),'last_publish_acknowledged_by_broker':BOOL,'dropped_inbound_messages':integer()})
    storage = obj({'mounted':BOOL,'records':integer(0,256),'pending_records':integer(0,256),'capacity_records':integer(0,256),'capacity_bytes':integer(),'used_bytes':integer(),'dropped_records_since_boot':integer(),'storage_faults_since_boot':integer(),'retry_count_since_boot':integer(),'last_acknowledged_record':string(79),'oldest_pending_record':{'type':['string','null'],'maxLength':79}})
    indicators = obj({'led_on':BOOL,'led_pattern':string(32),'buzzer_active':BOOL,'buzzer_muted':BOOL})
    peripherals = obj({'present':BOOL,'encoder_enabled':BOOL,'backlight':BOOL,'page':integer(0,2)})
    status_fields = {'firmware_version':string(32),'build_version':string(32),'name':string(48),'site_id':string(48),'plot_id':string(48),'hardware_model':{'const':'ESP32-WROOM-32U'},'configuration_revision':integer(),'calibration_revision':integer(),'connectivity':connectivity,'storage':storage,'health':health,'indicators':indicators,'local_peripherals':peripherals,'setup_remaining_s':integer(0,300),'trial_state':string(64),'trial_error':string(96),'mqtt_host':string(128)}
    environment = obj({'enabled':BOOL,'valid':BOOL,'fault_code':string(64),'failure_count':integer(),'acquired_uptime_ms':integer(),'age_ms':{'type':['integer','null'],'minimum':0},'raw_temperature_c':number(-40,80,True),'raw_humidity_pct':number(0,100,True),'temperature_c':number(-50,90,True),'humidity_pct':number(0,100,True),'last_valid_temperature_c':number(-40,80,True),'last_valid_humidity_pct':number(0,100,True)})
    analog = {'raw_adc':integer(0,4095),'adc_mv':integer(0,5000),'adc_factory_calibrated':BOOL,'saturated':BOOL,'stuck_suspected':BOOL,'acquired_uptime_ms':integer(),'calibration_revision':integer(),'calibration_valid':BOOL,'valid':BOOL,'fault_code':string(64)}
    soil = obj(analog | {'enabled':BOOL,'filtered_adc':number(0,4095,True),'relative_moisture_pct':number(0,100,True),'vwc_pct':{'type':'null'}})
    power = analog | {'voltage_v':number(0,1000,True),'low_threshold_v':number(0,1000),'critical_threshold_v':number(0,1000),'low':BOOL,'critical':BOOL}
    measurements = obj({'environment':environment,'soil':soil,'battery':obj(power | {'percentage':{'type':'null'}}),'solar':obj(power)})
    write('status.schema.json',message('status',status_fields))
    write('telemetry.schema.json',message('telemetry',status_fields | {'measurements':measurements}))
    write('event.schema.json',message('event',{'code':string(64)}))
    write('configuration.schema.json',message('configuration',{'configuration':{'type':'object','not':{'anyOf':[{'required':['admin_password']},{'properties':{'wifi':{'required':['password']}},'required':['wifi']},{'properties':{'mqtt':{'required':['password']}},'required':['mqtt']}]}}}))
    write('capabilities.schema.json',message('capabilities',{'capabilities':obj({'commands':{'type':'array','items':{'enum':names},'uniqueItems':True},'command_authorization':string(),'mutations_require_expected_revision':{'const':True},'max_command_lifetime_s':{'const':300},'headless':{'const':True},'lcd_present':BOOL,'encoder_enabled':BOOL,'tls_required':{'const':True},'mqtt_qos':{'const':1},'durable_ack_enabled':BOOL,'ota':{'const':False},'vwc':{'const':False},'battery_percentage':{'const':False},'mtls':{'const':False},'max_message_bytes':{'const':8192},'max_history_records_per_response':{'const':1}})}))
    write('command_response.schema.json',message('command_response',{'request_id':string(64),'status':{'enum':['received','accepted','applied','rejected','failed','timed_out']},'error':string(128),'configuration_revision':integer(),'result':{'type':'object'},'duplicate':BOOL},['request_id','status','error','configuration_revision','result']))
    write('sync_ack.schema.json',META | obj({'schema_version':{'const':'1.0'},'device_id':IDENTIFIER,'message_id':IDENTIFIER,'durably_stored':{'const':True}}))
    command_schema = META | obj({'schema_version':{'const':'1.0'},'request_id':IDENTIFIER | {'maxLength':64},'device_id':IDENTIFIER,'command':{'enum':names},'args':{'type':'object'},'expected_revision':integer(),'expires_at':integer(1700000000,9007199254740991)},['schema_version','request_id','device_id','command','args','expires_at'])
    def args_for(command):
        group, action = command.split('.')
        if action in ['reboot','factory_reset','restore_defaults','forget_network','purge_acknowledged']:
            return obj({'confirm_device_id':IDENTIFIER})
        if command in ['config.patch','config.validate']:
            return obj({'patch':obj(remote_patch_properties,required=[])})
        if action in ['configure','test_connection']:
            return obj({'settings':config_properties[group]})
        if action == 'set_calibration' or action == 'validate_calibration':
            keys=['dry','wet'] if group=='soil' else ['top_ohm','bottom_ohm','gain','offset','min_mv','max_mv']
            return obj({k:config_properties[group]['properties'][k] for k in keys}, required=keys if group=='soil' else [])
        if action == 'set_offsets':return obj({'temperature_offset':number(-10,10),'humidity_offset':number(-20,20)},required=[])
        if action == 'capture_reference':return obj({'reference_v':number(.001,1000)})
        if action == 'set_thresholds':return obj({'low_v':number(0,1000),'critical_v':number(0,1000)},required=[])
        if action in ['set_enabled','set_backlight','set_page','set_timeout']:
            key={'set_enabled':'enabled','set_backlight':'backlight','set_page':'page','set_timeout':'timeout_s'}[action]
            return obj({key:config_properties[group]['properties'][key]})
        if action == 'show_notice':return obj({'text':string(80),'duration_ms':integer(1000,60000)})
        if action == 'test':return obj({'duration_ms':integer(20,3000)})
        if action == 'mute':return obj({'duration_s':integer(1,86400)})
        if action in ['set_preferences','set_alert_preferences']:return config_properties[group]
        if action in ['get_history','export_records']:return obj({'after':integer(),'limit':{'const':1}},required=['after'])
        if action == 'set_intervals':return obj({k:config_properties[k] for k in ['sample_s','upload_s','health_s']},required=[])
        if action == 'set_retention':return config_properties['storage']
        return obj({})
    conditions=[]
    manifest=[]
    for name in names:
        read = '.get_' in name or name in ['config.get','config.export','config.validate','soil.validate_calibration','storage.export_records','diagnostics.self_test']
        then={'properties':{'args':args_for(name)}}
        if not read:then['required']=['expected_revision']
        conditions.append({'if':{'properties':{'command':{'const':name}}},'then':then})
        manifest.append({'command':name,'mutating':not read,'args':args_for(name)})
    command_schema['allOf']=conditions
    write('command.schema.json',command_schema)
    write('commands.json',manifest,'examples')
    lines=['# Remote command reference','',
           'All requests use schema `1.0`, an exact device ID, a unique 1–64 character alphanumeric/underscore/hyphen request ID, an argument object and a Unix expiry no more than 300 seconds ahead. The broker authenticates and authorizes the sender; retained commands are rejected.','',
           'Commands marked **Yes** require `expected_revision`. Destructive commands also require `confirm_device_id` equal to the target. Unknown arguments are rejected. Optional arguments are marked `?`; `{}` means no arguments. The checked-in command schema contains the types and numerical bounds.','',
           'Read operations return snapshots or validation results. Configuration and preference setters return the committed revision. Calibration is restricted to the local developer portal. Acquisition and connection trials return `accepted` before a final response. Bounded LED/buzzer tests report that the operation started; they do not establish electrical functionality.','',
           'Mutation receipts are written before execution, with final status/error persisted afterward. Repeating an ID returns the stored status without executing again. A `received` receipt after reset has `execution_indeterminate_after_restart`; query the actual state before using a new ID. Accepted operations can be interrupted by reset. Read-only requests can be safely re-executed. There are 32 active mutation receipts, each retained through expiry, and a one-second mutation rate limit. Full replay windows reject new mutations.','']
    last_group=None
    for row in manifest:
        group=row['command'].split('.')[0]
        if group!=last_group:
            lines += [f'## {group.capitalize()}','', '| Command | Revision required | Arguments |','|---|---|---|']
            last_group=group
        props=row['args'].get('properties',{})
        required=row['args'].get('required',[])
        arguments=', '.join('`'+key+('' if key in required else '?')+'`' for key in props) or '`{}`'
        lines.append(f"| `{row['command']}` | {'Yes' if row['mutating'] else 'No'} | {arguments} |")
        if row==manifest[-1] or manifest[manifest.index(row)+1]['command'].split('.')[0]!=group:lines.append('')
    lines += ['## Execution details','',
              '- `read_now` acquires the complete sensor set and completes only after a record-storage attempt. Get the resulting data with `telemetry.get_last` or the corresponding sensor status command.',
              '- Sensor status commands expose observed readings and quality only. Calibration is local-developer-portal-only; remote configuration exports do not contain calibration settings. See [Calibration](CALIBRATION.md).',
              '- `wifi.scan` returns the previous scan results and requests a new asynchronous scan. A subsequent call retrieves the completed results.',
              '- `wifi.configure` / `mqtt.configure` save only after their selected trial passes. `*.test_connection` restores previous settings after the trial.',
              '- `device.sync_time` requests SNTP and reports the current synchronization state; it does not claim a completed time adjustment.',
              '- `lcd.set_enabled` configures the optional profile even when absent. Other LCD mutations require detected hardware. Encoder enable is available through `config.patch`.',
              '- LED/buzzer tests require 20–3,000 ms. Buzzer tests reject disabled or muted output. Notices require 1,000–60,000 ms and at most 80 characters.',
              '- `storage.export_records` / `telemetry.get_history` use exclusive `after` and `limit: 1`; `more` and `next_cursor` describe pagination.',
              '- Factory reset preserves records, the administrator verifier and calibration. Restore-defaults also preserves Wi-Fi/MQTT and calibration. See [Recovery](CONFIGURATION_RECOVERY.md).',
              '- Firmware check/start-update commands are unsupported and not advertised; updates use physical serial upload. No command executes arbitrary code, file paths or GPIO access.','',
              '## Response and error contract','',
              'Every response carries the originating request ID, `status`, `error`, `result`, resulting configuration revision and normal message envelope. Status values are `received`, `accepted`, `applied`, `rejected`, `failed` and reserved `timed_out`; connection timeouts currently produce `failed` with `connection_trial_failed`. Responses lost during a connection change can be recovered by repeating the same unexpired ID after reconnect.','',
              'Envelope errors: `invalid_json`, `wrong_version_or_device`, `invalid_envelope`, `clock_unsynchronized`, `expired_or_future`. Authorization/dispatch errors: `unsupported_command`, `revision_conflict`, `confirmation_required`, `unknown_argument`, `command_rate_limited`, `replay_window_full`, `replay_storage_unavailable`, `replay_journal_write_failed`. Operational errors include `configuration_trial_busy`, `connection_trial_failed`, `trial_cancelled`, `configuration_commit_failed`, `unsupported_hardware`, `acquisition_busy`, `sensor_minimum_interval`, `fresh_valid_adc_required`, `record_not_stored` and `response_too_large`. Validation errors identify the field group; successful responses carry an empty error string.','']
    (ROOT/'docs/COMMAND_REFERENCE.md').write_text('\n'.join(lines),encoding='utf-8')
    safe=copy.deepcopy(defaults)
    for key in ['wifi','mqtt']:
        safe[key].pop('password');safe[key]['password_set']=False
    safe['mqtt'].pop('ca');safe['mqtt']['ca_configured']=False
    write('configuration.json',safe,'examples')
    base={'schema_version':'1.0','message_type':'event','message_id':'CWS-TEST-0123456789abcdef-1','device_id':'CWS-TEST','boot_id':'0123456789abcdef','sequence':1,'time':{'uptime_ms':2500,'synchronized':False,'unix_s':None}}
    write('event.json',base | {'code':'boot'},'examples')
    write('command.json',{'schema_version':'1.0','request_id':'integration-1','device_id':'CWS-TEST','command':'soil.get_status','args':{},'expires_at':1800000120},'examples')
    write('command_response.json',base | {'message_type':'command_response','request_id':'integration-1','status':'applied','error':'','configuration_revision':3,'result':{'revision':3,'calibration_revision':1}},'examples')
    write('sync_ack.json',{'schema_version':'1.0','device_id':'CWS-TEST','message_id':base['message_id'],'durably_stored':True},'examples')
    caps={'commands':names,'command_authorization':'TLS broker identity and per-device ACL','mutations_require_expected_revision':True,'max_command_lifetime_s':300,'headless':True,'lcd_present':False,'encoder_enabled':False,'tls_required':True,'mqtt_qos':1,'durable_ack_enabled':True,'ota':False,'vwc':False,'battery_percentage':False,'mtls':False,'max_message_bytes':8192,'max_history_records_per_response':1}
    write('capabilities.json',base | {'message_type':'capabilities','capabilities':caps},'examples')
    def skeleton(schema):
        if 'const' in schema:return schema['const']
        if schema.get('type')=='object':return {k:skeleton(v) for k,v in schema['properties'].items()}
        if schema.get('type')=='array':return []
        if schema.get('type')=='boolean':return False
        if schema.get('type')=='null' or isinstance(schema.get('type'),list) and 'null' in schema['type']:return None
        if schema.get('type') in ['number','integer']:return schema.get('minimum',0)
        return ''
    sample=skeleton(message('telemetry',status_fields | {'measurements':measurements}));sample.update(base);sample['message_type']='telemetry';sample['firmware_version']='1.0.0';sample['build_version']='2026-10-13';sample['health']['operating_state']='offline_buffering';sample['name']='Protocol fixture';sample['trial_state']='idle';sample['indicators']['led_pattern']='startup'
    write('telemetry.json',sample,'examples')
    print(f'Wrote protocol schemas and {len(names)} command contracts.')


if __name__=='__main__':main()
