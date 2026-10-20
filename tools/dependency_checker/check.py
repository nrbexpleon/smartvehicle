#!/usr/bin/env python3
import re,sys
from pathlib import Path
RULES={'examples':('adapters/','vendor/','linux/','sys/'),'include/smartvehicle/services':('smartvehicle/hal/','adapters/','vendor/'),'include/smartvehicle/middleware':('smartvehicle/hal/','adapters/','vendor/')}
def main(root):
 violations=[]
 for base,forbidden in RULES.items():
  for p in (Path(root)/base).rglob('*'):
   if p.suffix not in {'.cpp','.hpp','.h'}:continue
   text=p.read_text(errors='replace')
   for inc in re.findall(r'#include\s*[<"]([^>"]+)',text):
    if any(inc.startswith(x) for x in forbidden):violations.append(f'{p}: illegal include {inc}')
 if violations: print('\n'.join(violations));return 1
 print('architecture dependency rules: pass');return 0
if __name__=='__main__':sys.exit(main(sys.argv[1] if len(sys.argv)>1 else '.'))
