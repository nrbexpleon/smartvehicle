#!/usr/bin/env python3
import json
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
try:
    revision = subprocess.check_output(
        ['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
except Exception:
    revision = 'unknown'
try:
    openssl_version = subprocess.check_output(
        ['openssl', 'version'], text=True).split()[1]
except Exception:
    openssl_version = 'unknown'

application = {
    'type': 'application',
    'name': 'smartvehicle',
    'version': '0.2.0',
    'purl': 'pkg:generic/smartvehicle@0.2.0'
}
components = [
    {
        'type': 'library',
        'name': 'OpenSSL',
        'version': openssl_version,
        'purl': f'pkg:generic/openssl@{openssl_version}',
        'licenses': [{'license': {'id': 'Apache-2.0'}}]
    }
]
out = {
    'bomFormat': 'CycloneDX',
    'specVersion': '1.5',
    'version': 1,
    'metadata': {
        'component': application,
        'properties': [{'name': 'git.commit', 'value': revision}]
    },
    'components': components
}
(root / 'build').mkdir(exist_ok=True)
(root / 'build/sbom.cdx.json').write_text(json.dumps(out, indent=2) + '\n')
print('build/sbom.cdx.json')
