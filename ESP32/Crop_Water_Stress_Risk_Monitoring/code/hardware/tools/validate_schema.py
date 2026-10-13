"""Validate a CWS protocol message against the checked-in version 1.0 contract."""
import argparse
import json
from pathlib import Path
from jsonschema import Draft202012Validator

ROOT = Path(__file__).resolve().parents[1]


def validate(message):
    kind = message.get('message_type')
    if kind is None:
        kind = 'command' if 'command' in message else 'sync_ack'
    if kind not in {'command', 'command_response', 'sync_ack', 'telemetry', 'status', 'event', 'configuration', 'capabilities'}:
        raise ValueError('Unknown message type')
    schema = json.loads((ROOT / 'schemas' / (kind + '.schema.json')).read_text())
    Draft202012Validator(schema).validate(message)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('message', type=Path)
    args = parser.parse_args()
    validate(json.loads(args.message.read_text()))
    print('Schema valid.')
