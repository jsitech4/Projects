"""TLS MQTT subscriber, command publisher, response inspector and durable receiver."""
import argparse
import json
import os
from pathlib import Path
import ssl
import threading
import time
import uuid
import paho.mqtt.client as mqtt
from validate_schema import validate


def durable_write(directory: Path, message):
    validate(message)
    identity = message['message_id']
    if not identity or any(not (c.isascii() and (c.isalnum() or c in '_-')) for c in identity):
        raise ValueError('Invalid record identity')
    directory.mkdir(parents=True, exist_ok=True)
    target = directory / (identity + '.json')
    data = json.dumps(message, sort_keys=True, separators=(',', ':')).encode()
    if target.exists():
        if target.read_bytes() != data:
            raise ValueError('Conflicting payload for a previously ingested identity')
        return target
    temporary = target.with_suffix('.tmp')
    with temporary.open('wb') as stream:
        stream.write(data)
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, target)
    if os.name != 'nt':
        descriptor = os.open(directory, os.O_RDONLY)
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)
    return target


def client_from_environment(on_message):
    host = os.environ['CWS_MQTT_HOST']
    ca = os.environ['CWS_MQTT_CA']
    username = os.environ['CWS_MQTT_USERNAME']
    password = os.environ['CWS_MQTT_PASSWORD']
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id='cws-tool-' + uuid.uuid4().hex[:16])
    client.username_pw_set(username, password)
    context = ssl.create_default_context(cafile=ca)
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    client.tls_set_context(context)
    client.on_message = on_message
    client.connect(host, int(os.environ.get('CWS_MQTT_PORT', '8883')), 30)
    return client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['subscribe', 'responses', 'command', 'ack', 'receive', 'integration'])
    parser.add_argument('--device', required=True)
    parser.add_argument('--command', default='device.get_status')
    parser.add_argument('--args', default='{}')
    parser.add_argument('--revision', type=int)
    parser.add_argument('--message-id')
    parser.add_argument('--directory', type=Path, default=Path('test-results/backend'))
    parser.add_argument('--timeout', type=int, default=30)
    args = parser.parse_args()
    if any(c not in 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-' for c in args.device):
        parser.error('Invalid device identifier')
    base = f'cws/v1/devices/{args.device}'
    request_id = uuid.uuid4().hex
    connected, completed = threading.Event(), threading.Event()
    failures = []

    def on_message(client, userdata, msg):
        try:
            if len(msg.payload) > 8192:
                raise ValueError('Message exceeds device contract limit')
            message = json.loads(msg.payload)
            validate(message)
            if message['device_id'] != args.device:
                raise ValueError('Wrong device identity')
            if args.mode == 'receive' and message['message_type'] in {'telemetry', 'event'}:
                durable_write(args.directory, message)
                acknowledgement = {'schema_version':'1.0', 'device_id':args.device,
                                   'message_id':message['message_id'], 'durably_stored':True}
                client.publish(base + '/sync_ack', json.dumps(acknowledgement), qos=1, retain=False)
                print('Stored and acknowledged', message['message_id'])
            else:
                print(json.dumps(message, indent=2))
            if message.get('request_id') == request_id and message.get('status') in {'applied','failed','rejected','timed_out'}:
                if message['status'] != 'applied':
                    failures.append(message.get('error', 'command_failed'))
                completed.set()
        except Exception as error:
            failures.append(type(error).__name__)
            print('Message rejected:', type(error).__name__)

    client = client_from_environment(on_message)

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            failures.append('broker_authentication_failed')
            completed.set()
            return
        suffixes = ['telemetry','events'] if args.mode == 'receive' else ['command_responses'] if args.mode in {'command','responses','integration'} else ['telemetry','status','events','capabilities','configuration','command_responses']
        client.subscribe([(base + '/' + suffix,1) for suffix in suffixes])

    def on_subscribe(client, userdata, mid, reason_codes, properties):
        if any(code.is_failure for code in reason_codes):
            failures.append('subscription_rejected')
        connected.set()

    client.on_connect = on_connect
    client.on_subscribe = on_subscribe
    client.loop_start()
    try:
        if not connected.wait(args.timeout) or failures:
            raise RuntimeError('Broker connection or subscription failed')
        if args.mode in {'command','integration'}:
            message = {'schema_version':'1.0','request_id':request_id,'device_id':args.device,
                       'command':args.command,'args':json.loads(args.args), 'expires_at':int(time.time())+120}
            if args.revision is not None:
                message['expected_revision'] = args.revision
            validate(message)
            client.publish(base + '/commands', json.dumps(message), qos=1, retain=False).wait_for_publish(timeout=args.timeout)
            if not completed.wait(args.timeout) or failures:
                raise RuntimeError('Device integration command did not complete successfully')
            print('Correlated, schema-valid device response received over TLS.')
        elif args.mode == 'ack':
            if not args.message_id:
                parser.error('--message-id is required for the isolated acknowledgement simulator')
            message = {'schema_version':'1.0','device_id':args.device,'message_id':args.message_id,'durably_stored':True}
            validate(message)
            client.publish(base + '/sync_ack', json.dumps(message), qos=1, retain=False).wait_for_publish(timeout=args.timeout)
        else:
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        client.disconnect()
        client.loop_stop()


if __name__ == '__main__':
    main()
