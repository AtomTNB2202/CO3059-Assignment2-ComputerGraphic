# Assignment 2 report

`report.tex` is the English Assignment 2 report, adapted from the supplied
Assignment 1 layout. `report.pdf` is compiled with XeLaTeX. Author:
Nguyễn Vinh Anh Quân — 2353007. Assignment 1 foundation:
Bùi Quốc Thái — 2353086.

Build from this directory with MiKTeX or TeX Live:

```powershell
xelatex -interaction=nonstopmode -halt-on-error report.tex
xelatex -interaction=nonstopmode -halt-on-error report.tex
```

Use XeLaTeX on Overleaf. Keep `figures/` beside the TEX source. The fonts have
TeX fallbacks. Figures contain actual application captures/exports; inherited
Assignment 1 screenshots are identified in their caption. `test-results.txt`
records the CTest run used when preparing this report.

The illustrated eight-sample dataset was generated at `out/yolo-viewer-check/`
using `assets/scenes/tabletop.json` and `assets/configs/batch_demo.json`.
It contains 40 visible annotations in 4 categories. The dataset itself is not
required to compile the report; all report images are included in `figures/`.

When moving/submitting, include TEX, PDF and figures. The existing draft
source-package script excludes `report/`; attach this directory separately.
