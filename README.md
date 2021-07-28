# Qalculate! Qt UI

<a href="https://raw.githubusercontent.com/Qalculate/qalculate.github.io/master/images/qalculate-qt.png"><img src="https://raw.githubusercontent.com/Qalculate/qalculate.github.io/master/images/qalculate-qt.png" width="552"></a>

Qalculate! is a multi-purpose cross-platform desktop calculator. It is simple to use but provides power and versatility normally reserved for complicated math packages, as well as useful tools for everyday needs (such as currency conversion and percent calculation). Features include a large library of customizable functions, unit calculations and conversion, symbolic calculations (including integrals and equations), arbitrary precision, uncertainty propagation, interval arithmetic, plotting, and a user-friendly interface (GTK, Qt, and CLI).

## Requirements
* Qt (>= 5.6)
* libqalculate (>= 3.20.0)

## Installation
Instructions and download links for installers, binaries packages, and the source code of released versions of Qalculate! are available at https://qalculate.github.io/downloads.html.

In a terminal window in the top source code directory run
* `lrelease qalculate-qt.pro` *(not required if using a release source tarball, only if using the git version)*
* `qmake`
* `make`
* `make install` *(as root, e.g. `sudo make install`)*

The resulting executable is named `qalculate-qt`.
