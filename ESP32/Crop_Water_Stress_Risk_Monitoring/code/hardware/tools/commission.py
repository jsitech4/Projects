"""Create private commissioning records and perform authorized serial service."""
import argparse
import getpass
import hashlib
import json
from pathlib import Path
import secrets
import string
import time

ITERATIONS = 60000


def create_records(directory: Path, password: str):
    if not (12 <= len(password) <= 96 and password.isascii() and all(32 <= ord(c) <= 126 for c in password)
            and any(c.isupper() for c in password) and any(c.islower() for c in password)
            and any(c.isdigit() for c in password) and any(c in string.punctuation for c in password)):
        raise ValueError('Administrator password must match the firmware strength policy')
    directory.mkdir(parents=True, exist_ok=True)
    if any((directory / name).exists() for name in ('factory.json', 'owner.json', 'service.json')):
        raise FileExistsError('Commissioning records already exist; existing credentials were preserved')
    ap = secrets.token_urlsafe(18)
    token = secrets.token_hex(32)
    salt = secrets.token_bytes(16)
    verifier = hashlib.pbkdf2_hmac('sha256', password.encode(), salt, ITERATIONS)
    records = {
        'factory.json': {'ap_password': ap, 'admin_salt': salt.hex(), 'admin_verifier': verifier.hex(),
                         'service_verifier': hashlib.sha256(token.encode()).hexdigest()},
        'owner.json': {'setup_network_password': ap, 'initial_admin_password': password,
                       'username': 'admin', 'portal': 'http://192.168.4.1'},
        'service.json': {'service_token': token},
    }
    for name, record in records.items():
        with (directory / name).open('x', encoding='utf-8') as stream:
            json.dump(record, stream, indent=2)
            stream.write('\n')
    return directory / 'factory.json'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    create = sub.add_parser('create')
    create.add_argument('--directory', type=Path, required=True)
    service = sub.add_parser('service')
    service.add_argument('--port', required=True)
    service.add_argument('--record', type=Path, required=True)
    service.add_argument('--operation', choices=['recover_admin', 'initialize_storage'], required=True)
    args = parser.parse_args()
    if args.action == 'create':
        password = getpass.getpass('Initial administrator password: ')
        if password != getpass.getpass('Confirm password: '):
            raise ValueError('Passwords differ')
        path = create_records(args.directory.resolve(), password)
        print(f'Private records created in {path.parent}. Credential values are not printed.')
    else:
        import serial
        token = json.loads(args.record.read_text())['service_token']
        request = {'action': args.operation, 'service_token': token}
        if args.operation == 'recover_admin':
            password = getpass.getpass('Replacement administrator password: ')
            if password != getpass.getpass('Confirm password: '):
                raise ValueError('Passwords differ')
            request['new_password'] = password
        with serial.Serial(args.port, 115200, timeout=1) as port:
            print('Restart the monitor now. Waiting for its startup message.')
            until = time.monotonic() + 90
            while time.monotonic() < until:
                if b'CWS firmware ready' in port.readline():
                    time.sleep(3.2)
                    port.write(json.dumps(request).encode() + b'\n')
                    break
            else:
                raise TimeoutError('Device startup was not observed')
            until = time.monotonic() + 15
            while time.monotonic() < until:
                reply = port.readline()
                if b'service_ok' in reply:
                    print('Authorized service completed.')
                    return
                if b'service_rejected' in reply:
                    raise RuntimeError('Device rejected service request')
            raise TimeoutError('No service result received')


if __name__ == '__main__':
    main()
