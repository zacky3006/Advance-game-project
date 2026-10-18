"""Solo death: fade in Mission Failed, then return to Menu."""
import unreal
from pathlib import Path
script=Path(unreal.Paths.project_dir())/'Scripts/ValidateMissionFailedPIE.py'
exec(compile(script.read_text(encoding='utf-8'),str(script),'exec'))
