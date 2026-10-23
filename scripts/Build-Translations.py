"""Validate the five UI catalogs and emit the embedded JSON resource."""
from pathlib import Path
import json,re
root=Path(__file__).resolve().parents[1]
catalog={}
for line in (root/'data/locale/translations.txt').read_text(encoding='utf-8').splitlines():
 if not line or line.startswith('#'):continue
 parts=line.split('¦')
 if len(parts)!=5:raise ValueError('Expected 5 columns: '+line)
 source,*translated=parts
 if source in catalog:raise ValueError('Duplicate source: '+source)
 catalog[source]=dict(zip(['es','en','it','fr','de'],[source,*translated]))
 for lang,value in catalog[source].items():
  if not value or sorted(re.findall(r'%\d+',value))!=sorted(re.findall(r'%\d+',source)):raise ValueError('Bad placeholders: '+source+' / '+lang)
spanish={'Base Title':'Título base','Category':'Categoría','Tags':'Etiquetas','Automatic Events':'Eventos automáticos','Title Preview':'Vista previa del título','Apply to Twitch':'Aplicar en Twitch'}
for source,value in spanish.items():catalog[source]['es']=value
(root/'data/locale/strings.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2),encoding='utf-8')
print(f'{len(catalog)} messages verified in es/en/it/fr/de')
