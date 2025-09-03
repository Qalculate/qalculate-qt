# Qalculate! Qt

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Qt Version](https://img.shields.io/badge/Qt-%3E%3D%205.6-brightgreen.svg)](https://www.qt.io/)
[![libqalculate Version](https://img.shields.io/badge/libqalculate-%3E%3D%205.6.0-brightgreen.svg)](https://qalculate.github.io/)
[![Platform](https://img.shields.io/badge/platform-cross--platform-lightgrey.svg)](https://qalculate.github.io/downloads.html)

<p align="center">
  <a href="https://raw.githubusercontent.com/Qalculate/qalculate.github.io/master/images/qalculate-qt.png">
    <img src="https://raw.githubusercontent.com/Qalculate/qalculate.github.io/master/images/qalculate-qt.png" alt="Qalculate! Qt Screenshot" width="552">
  </a>
</p>

> **Qalculate!** is a powerful and versatile cross-platform desktop calculator. It combines the simplicity of a traditional calculator with the power of a complex math package, making it an essential tool for everyone from students to engineers.

## Key Features
- **Extensive Function Library:** A vast library of customizable functions for various fields.
- **Unit Calculations & Conversions:** Seamlessly handle units with smart conversion capabilities.
- **Symbolic Calculations:** Perform symbolic math, including integrals, equations, factorization, and simplification.
- **Arbitrary Precision:** No more worrying about rounding errors.
- **Uncertainty Propagation & Interval Arithmetic:** Advanced mathematical tools for precise calculations.
- **Plotting:** Visualize your data and functions with Gnuplot integration.
- **User-Friendly Interface:** A clean and intuitive interface available for GTK, Qt, and CLI.

## Getting Started

### Requirements
| Dependency | Version |
| :--- | :--- |
| Qt | >= 5.6 (Qt6 supported) |
| libqalculate | >= 5.6.0 |

### Installation
You can find installers and binary packages on the [official downloads page](https://qalculate.github.io/downloads.html).

To build from source, run the following commands in the root directory:
```bash
qmake
make
sudo make install
```
The executable will be named `qalculate-qt`.

<details>
<summary><b>Full Feature List (from libqalculate)</b></summary>

* **Calculation and parsing:**
   * Basic operations and operators: `+ - * / mod ^ E () && || ! < > >= <= != ~ & | << >> xor`
   * Fault-tolerant parsing of strings: `log 5 / 2 .5 (3) + (2( 3 +5` = `ln(5) / (2.5 * 3) + 2 * (3 + 5)`
   * Expressions may contain any combination of numbers, functions, units, variables, vectors and matrices, and dates
   * Supports complex and infinite numbers
   * Propagation of uncertainty
   * Interval arithmetic (for determination of the number of significant digits or direct calculation with intervals of numbers)
   * Supports all common number bases, as well as negative and non-integer radices, sexagesimal numbers, time format, and roman numerals
   * Ability to disable functions, variables, units or unknown variables for less confusion: e.g. when you do not want `(a+b)^2` to mean `(are+barn)^2` but `("a"+"b")^2`
   * Controllable implicit multiplication
   * Matrices and vectors, and related operations (determinants etc.)
   * Verbose error messages
   * Arbitrary precision
   * RPN mode
* **Result display:**
   * Supports all common number bases, as well as negative and non-integer radices, sexagesimal numbers, time format, and roman numerals
   * Many customization options: precision, max/min decimals, complex form, multiplication sign, etc.
   * Exact or approximate: `sqrt(32)` returns `4 * sqrt(2)` or `5.66`
   * Simple and mixed fractions: `4 / 6 * 2 = 1.333... = 4/3 = 1 + 1/3`
* **Symbolic calculation:**
   * E.g. `(x + y)^2 = x^2 + 2xy + y^2`; `4 "apples" + 3 "oranges"`
   * Factorization and simplification
   * Differentiation and integration
   * Can solve most equations and inequalities
   * Customizable assumptions give different results (e.g. `ln(2x) = ln(2) + ln(x)` if x is assumed positive)
* **Functions:**
   * Hundreds of flexible functions: trigonometry, exponents and logarithms, combinatorics, geometry, calculus, statistics, finance, time and date, etc.
   * Can easily be created, edited and saved to a standard XML file
* **Units:**
   * Supports all SI units and prefixes (including binary), as well as imperial and other unit systems
   * Automatic conversion: `ft + yd + m = 2.2192 m`
   * Explicit conversion: `5 m/s to mi/h = 11.18 miles/hour`
   * Smart conversion: automatically converts `5 kg*m/s^2` to `5 N`
   * Currency conversion with retrieval of daily exchange rates
   * Different name forms: abbreviation, singular, plural (m, meter, meters)
   * Can easily be created, edited and saved to a standard XML file
* **Variables and constants:**
   * Basic constants: pi, e, etc.
   * Lots of physical constants (with or without units) and properties of chemical element
   * CSV file import and export
   * Can easily be created, edited and saved to a standard XML file
   * Flexible - may contain simple numbers, units, or whole expressions
   * Data sets with objects and associated properties in database-like structure
* **Plotting:**
   * Uses Gnuplot
   * Can plot functions or data (matrices and vectors)
   * Ability to save plot to PNG image, postscript, etc.
   * Several customization options
* and more...

</details>

<details>
<summary><b>Usage Examples</b></summary>

_Note that semicolon can be replaced with comma in function arguments, if comma is not used as decimal or thousands separator._

### Basic functions and operators

`sqrt(4)` _= 4^(0.5) = 4^(1/2) = 2_

`sqrt(25; 16; 9; 4)` _= \[5  4  3  2\]_

`sqrt(32)` _= 4 × √(2) (in exact mode)_

`cbrt(−27)` _= root(-27; 3) = −3 (real root)_

`(−27)^(1/3)` _≈ 1.5 + 2.5980762i (principal root)_

`ln(25)` _= log(25; e) ≈ 3.2188758_

`log2(4)/log10(100)` _= log(4; 2)/log(100; 10) = 1_

`5!` _= 1 × 2 × 3 × 4 × 5 = 120_

`52 to factors` _= 2^2 × 13_

`sum(x; 1; 5)` _= 1 + 2 + 3 + 4 + 5 = 15_

`plot(x^2; −5; 5)` _(plots the function y=x^2 from -5 to 5)_

### Units

`5 dm3 to L` _= 5 dm^3 to L = 5 L_

`20 miles / 2h to km/h` _= 16.09344 km/h_

`1.74 to ft` _= 1.74 m to ft ≈ 5 ft + 8.5039370 in_

`50 Ω × 2 A` _= 100 V_

`500 € − 20% to $` _≈ $451.04_

### Physical constants

`k_e / G × a_0` _= (coulombs_constant / newtonian_constant) × bohr_radius ≈ 7.126e9 kg·H·m^−1_

`ℎ / (λ_C × c)` _= planck ∕ (compton_wavelength × speed_of_light) ≈ 9.1093837e-31 kg_

`(G × planet(earth; mass) × planet(mars; mass))/(54.6e6 km)^2` _≈ 8.58e16 N_

### Uncertainty and interval arithmetic

`sin(5±0.2)^2/2±0.3` _≈ 0.460±0.088 (0.46±0.12)_

`interval(−2; 5)^2` _≈ interval(−8.2500000; 12.750000) (interval(0; 25))_

### Algebra

`(5x^2 + 2)/(x − 3)` _= 5x + 15 + 47/(x − 3)_

`factorize x^4 − 7x^3 + 9x^2 + 27x − 54` _= (x + 2)(x − 3)^3_

`x+x^2+4 = 16` _= (x = 3 or x = −4)_

`solve(x = y+ln(y); y)` _= lambertw(e^x)_

### Calculus

`diff(6x^2)` _= 12x_

`integrate(6x^2; 1; 5)` _= 248_

`limit(ln(1 + 4x)/(3^x − 1); 0)` _= 4 / ln(3)_

### Matrices and vectors

`[1, 2, 3; 4, 5, 6]` _= \[1  2  3; 4  5  6\]_

`cross([1 2 3]; [4 5 6])` _= \[−3 6 −3\]_

`[1 2; 3 4]^-1` _= \[−2  1; 1.5  −0.5\]_

### Statistics

`mean(5; 6; 4; 2; 3; 7)` _= 4.5_

`stdev(5; 6; 4; 2; 3; 7)` _≈ 1.87_

### Time and date

`"2020-05-20" + 523d` _= "2021-10-25"_

`today − 5 days` _= "2020-07-05"_

`"2020-10-05" − today` _= 87 d_

### Number bases

`52 to bin` _= 0011 0100_

`52 to hex` _= 0x34_

`1978 to roman` _= MCMLXXVIII_

</details>

## Documentation
For more details about the syntax, and available functions, units, and variables, please consult the [**official manual**](https://qalculate.github.io/manual/).

## Contributing
Contributions are welcome! If you have a feature request, bug report, or pull request, please feel free to open an issue or submit a PR.

## License
This project is licensed under the **GNU General Public License v2.0**. See the [COPYING](COPYING) file for details.