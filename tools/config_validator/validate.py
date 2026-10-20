#!/usr/bin/env python3
import json
import sys
from pathlib import Path

REQUIRED = {
    'schema_version', 'profile', 'platform', 'runnable', 'transport',
    'signal_provider', 'adapters', 'enabled_services', 'service_dependencies',
    'api_versions', 'resource_limits', 'security', 'ota', 'logging'
}
REQUIRED_ADAPTERS = {'crypto', 'can', 'ota_slots'}
RUNNABLE_ADAPTERS = {
    'crypto': {'openssl-evp'},
    'can': {'socketcan'},
    'ota_slots': {'filesystem-ab-journal'}
}


def validate(path):
    data = json.loads(Path(path).read_text())
    errors = []
    missing = REQUIRED - set(data)
    errors += [f'missing {item}' for item in sorted(missing)]
    if data.get('schema_version') != '1.0':
        errors.append('unsupported schema_version')
    if data.get('transport') != 'in_process':
        errors.append('unsupported transport')

    adapters = data.get('adapters', {})
    errors += [f'missing adapter {item}' for item in sorted(REQUIRED_ADAPTERS - set(adapters))]
    if data.get('runnable'):
        for kind, allowed in RUNNABLE_ADAPTERS.items():
            if adapters.get(kind) not in allowed:
                errors.append(f'runnable profile has unsupported {kind} adapter')

    services = set(data.get('enabled_services', []))
    dependencies = data.get('service_dependencies', {})
    for service, needs in dependencies.items():
        if service not in services:
            errors.append(f'dependencies declared for disabled service {service}')
        for need in needs:
            if need not in services:
                errors.append(f'{service} requires missing {need}')

    visiting = set()
    visited = set()

    def walk(node):
        if node in visiting:
            errors.append(f'cyclic dependency at {node}')
            return
        if node in visited:
            return
        visiting.add(node)
        for child in dependencies.get(node, []):
            walk(child)
        visiting.remove(node)
        visited.add(node)

    for item in services:
        walk(item)
    return errors


if __name__ == '__main__':
    failed = False
    for argument in sys.argv[1:]:
        problems = validate(argument)
        if problems:
            failed = True
            print(argument + ': ' + '; '.join(problems))
        else:
            print(argument + ': valid')
    sys.exit(1 if failed else 0)
