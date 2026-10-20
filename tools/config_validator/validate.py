#!/usr/bin/env python3
import json,sys
from pathlib import Path
REQUIRED={'schema_version','profile','platform','runnable','transport','signal_provider','enabled_services','service_dependencies','api_versions','resource_limits','security','ota','logging'}
def validate(path):
 d=json.loads(Path(path).read_text()); errors=[]
 missing=REQUIRED-set(d); errors += [f'missing {x}' for x in sorted(missing)]
 if d.get('schema_version')!='1.0': errors.append('unsupported schema_version')
 if d.get('transport')!='in_process': errors.append('unsupported transport')
 services=set(d.get('enabled_services',[])); deps=d.get('service_dependencies',{})
 for service,needs in deps.items():
  if service not in services: errors.append(f'dependencies declared for disabled service {service}')
  for need in needs:
   if need not in services: errors.append(f'{service} requires missing {need}')
 visiting=set(); visited=set()
 def walk(node):
  if node in visiting: errors.append(f'cyclic dependency at {node}'); return
  if node in visited:return
  visiting.add(node)
  for child in deps.get(node,[]): walk(child)
  visiting.remove(node);visited.add(node)
 for item in services: walk(item)
 return errors
if __name__=='__main__':
 failed=False
 for arg in sys.argv[1:]:
  errors=validate(arg)
  if errors: failed=True; print(arg+': '+ '; '.join(errors))
  else: print(arg+': valid')
 sys.exit(1 if failed else 0)
