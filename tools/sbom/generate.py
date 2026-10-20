#!/usr/bin/env python3
import json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
try: revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
except Exception: revision='unknown'
components=[{'type':'library','name':'smartvehicle-core','version':'0.1.0','purl':'pkg:generic/smartvehicle@0.1.0'}]
out={'bomFormat':'CycloneDX','specVersion':'1.5','version':1,'metadata':{'component':components[0],'properties':[{'name':'git.commit','value':revision}]},'components':components}
Path(root/'build').mkdir(exist_ok=True);Path(root/'build/sbom.cdx.json').write_text(json.dumps(out,indent=2)+'\n');print('build/sbom.cdx.json')
