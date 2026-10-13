"""Host contract tests; these do not exercise physical ESP32 hardware."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import pytest
from jsonschema import Draft202012Validator, ValidationError

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from commission import create_records
from mqtt_tool import durable_write
from validate_schema import validate


@pytest.mark.parametrize('name',['telemetry','event','command','command_response','sync_ack','capabilities'])
def test_examples_match_contract(name):
    validate(json.loads((ROOT / 'examples' / (name+'.json')).read_text()))


@pytest.mark.parametrize('schema',list((ROOT / 'schemas').glob('*.json')))
def test_schemas_well_formed(schema):
    Draft202012Validator.check_schema(json.loads(schema.read_text()))


def test_mutation_requires_revision_and_bounds():
    message=json.loads((ROOT/'examples/command.json').read_text())
    message.pop('expected_revision')
    with pytest.raises(ValidationError):validate(message)
    message['expected_revision']=2
    message['args']['dry']=5000
    with pytest.raises(ValidationError):validate(message)


def test_arbitrary_gpio_rejected():
    message=json.loads((ROOT/'examples/command.json').read_text())
    message['command']='gpio.write'
    message['args']={'pin':16,'value':True}
    with pytest.raises(ValidationError):validate(message)


def test_ack_requires_permanent_ingestion():
    message=json.loads((ROOT/'examples/sync_ack.json').read_text())
    message['durably_stored']=False
    with pytest.raises(ValidationError):validate(message)


def test_nulls_remain_null():
    message=json.loads((ROOT/'examples/telemetry.json').read_text())
    assert message['measurements']['soil']['vwc_pct'] is None
    message['measurements']['soil']['vwc_pct']=45
    with pytest.raises(ValidationError):validate(message)


def test_backend_retry_deduplication(tmp_path):
    message=json.loads((ROOT/'examples/event.json').read_text())
    first=durable_write(tmp_path,message)
    first_stat=first.stat()
    assert durable_write(tmp_path,message)==first
    assert first.stat().st_mtime_ns==first_stat.st_mtime_ns
    assert len(list(tmp_path.glob('*.json')))==1
    assert not list(tmp_path.glob('*.tmp'))
    conflicting=copy.deepcopy(message)
    conflicting['code']='different_payload'
    with pytest.raises(ValueError):durable_write(tmp_path,conflicting)


def test_backend_interrupted_temp_recovers(tmp_path):
    message=json.loads((ROOT/'examples/event.json').read_text())
    (tmp_path/(message['message_id']+'.tmp')).write_text('{incomplete')
    final=durable_write(tmp_path,message)
    assert json.loads(final.read_text())==message
    assert not list(tmp_path.glob('*.tmp'))


def test_private_commissioning_has_unique_credentials(tmp_path):
    password='Fixture-only!Password42'
    first=create_records(tmp_path/'first',password)
    second=create_records(tmp_path/'second',password)
    f=json.loads(first.read_text());s=json.loads(second.read_text())
    assert f['ap_password']!=s['ap_password']
    assert f['admin_salt']!=s['admin_salt']
    assert f['admin_verifier']!=s['admin_verifier']
    assert password not in first.read_text()
    assert hashlib.pbkdf2_hmac('sha256',password.encode(),bytes.fromhex(f['admin_salt']),60000).hex()==f['admin_verifier']
    with pytest.raises(FileExistsError):create_records(tmp_path/'first',password)


def test_build_artifact_and_partition_limits():
    binary=ROOT/'.pio/build/esp32_wroom_32u/firmware.bin'
    if not binary.exists():pytest.skip('Firmware build has not completed')
    assert binary.stat().st_size<=0x200000
    partitions=[]
    for line in (ROOT/'partitions.csv').read_text().splitlines():
        if line and not line.startswith('#'):
            values=line.split(',');partitions.append((values[0],int(values[3],16),int(values[4],16)))
    for prior,current in zip(partitions,partitions[1:]):assert prior[1]+prior[2]<=current[1]
    assert max(offset+size for _,offset,size in partitions)==4*1024*1024
