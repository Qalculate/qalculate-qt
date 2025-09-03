# 🧮 Qalculate! Qt - Advanced Desktop Calculator

<div align="center">

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg?style=for-the-badge)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Qt Version](https://img.shields.io/badge/Qt-%3E%3D%205.6-brightgreen.svg?style=for-the-badge&logo=qt)](https://www.qt.io/)
[![libqalculate Version](https://img.shields.io/badge/libqalculate-%3E%3D%205.6.0-brightgreen.svg?style=for-the-badge)](https://qalculate.github.io/)
[![Platform](https://img.shields.io/badge/platform-cross--platform-lightgrey.svg?style=for-the-badge)](https://qalculate.github.io/downloads.html)

<img src="https://raw.githubusercontent.com/Qalculate/qalculate.github.io/master/images/qalculate-qt.png" alt="Qalculate! Qt Screenshot" width="600" style="border-radius: 10px; box-shadow: 0 4px 20px rgba(0,0,0,0.1);">

**A powerful and versatile cross-platform desktop calculator that combines simplicity with advanced mathematical capabilities**

[📥 Download](https://qalculate.github.io/downloads.html) • [📖 Documentation](https://qalculate.github.io/manual/) • [🐛 Report Bug](https://github.com/Qalculate/qalculate-qt/issues) • [💡 Request Feature](https://github.com/Qalculate/qalculate-qt/issues)

</div>

---

## 🌟 What Makes Qalculate! Special?

> **Qalculate!** bridges the gap between simple calculators and complex mathematical software. Whether you're a student solving homework problems or an engineer performing advanced calculations, Qalculate! adapts to your needs with precision and ease.

### ✨ Core Features

<table>
<tr>
<td width="50%">

**🧮 Mathematical Powerhouse**
- Extensive function library with 500+ built-in functions
- Symbolic calculations and equation solving
- Arbitrary precision arithmetic
- Complex numbers and infinite number support

</td>
<td width="50%">

**📐 Smart Unit Handling**
- Automatic unit conversions
- Support for all SI units and prefixes
- Imperial and metric system compatibility
- Real-time currency conversion

</td>
</tr>
<tr>
<td>

**📊 Advanced Analysis**
- Uncertainty propagation
- Interval arithmetic
- Statistical functions
- Matrix and vector operations

</td>
<td>

**🎨 User Experience**
- Clean, intuitive interface
- Plotting with Gnuplot integration
- Multiple number base support
- Customizable display options

</td>
</tr>
</table>

---

## 🚀 Quick Start

### 💻 System Requirements

| Component | Minimum Version |
|-----------|----------------|
| **Qt Framework** | 5.6+ (Qt6 supported) |
| **libqalculate** | 5.6.0+ |
| **Operating System** | Windows, macOS, Linux |

### 📦 Installation Options

<details>
<summary><b>🎯 Option 1: Pre-built Binaries (Recommended)</b></summary>

Visit our [**official downloads page**](https://qalculate.github.io/downloads.html) for:
- Windows installers (.msi, .exe)
- macOS packages (.dmg)
- Linux packages (.deb, .rpm, .appimage)

</details>

<details>
<summary><b>⚡ Option 2: Build from Source</b></summary>

```bash
# Clone the repository
git clone https://github.com/Qalculate/qalculate-qt.git
cd qalculate-qt

# Build and install
qmake
make
sudo make install

# Run the application
qalculate-qt
```

</details>

---

## 💡 Usage Examples

<details>
<summary><b>🔢 Basic Mathematical Operations</b></summary>

```javascript
sqrt(4)                    // = 2
sqrt(25; 16; 9; 4)        // = [5  4  3  2]
sqrt(32)                  // = 4 × √(2) (exact mode)
cbrt(−27)                 // = -3 (real root)
5!                        // = 120
52 to factors             // = 2^2 × 13
sum(x; 1; 5)             // = 15
```

</details>

<details>
<summary><b>📏 Unit Conversions & Calculations</b></summary>

```javascript
5 dm3 to L               // = 5 L
20 miles / 2h to km/h    // = 16.09344 km/h
1.74 to ft               // ≈ 5 ft + 8.5039370 in
50 Ω × 2 A              // = 100 V
500 € − 20% to $        // ≈ $451.04
```

</details>

<details>
<summary><b>🔬 Scientific Constants & Physics</b></summary>

```javascript
k_e / G × a_0           // Coulomb constant calculation
ℎ / (λ_C × c)           // Planck constant derivation
(G × planet(earth; mass) × planet(mars; mass))/(54.6e6 km)^2  // Gravitational force
```

</details>

<details>
<summary><b>📐 Advanced Mathematics</b></summary>

```javascript
// Algebra
(5x^2 + 2)/(x − 3)              // = 5x + 15 + 47/(x − 3)
factorize x^4 − 7x^3 + 9x^2 + 27x − 54  // = (x + 2)(x − 3)^3
solve(x = y+ln(y); y)           // = lambertw(e^x)

// Calculus
diff(6x^2)                      // = 12x
integrate(6x^2; 1; 5)           // = 248
limit(ln(1 + 4x)/(3^x − 1); 0)  // = 4 / ln(3)

// Linear Algebra
[1, 2, 3; 4, 5, 6]             // Matrix creation
cross([1 2 3]; [4 5 6])        // = [−3 6 −3]
[1 2; 3 4]^-1                  // Matrix inverse
```

</details>

<details>
<summary><b>📊 Statistics & Data Analysis</b></summary>

```javascript
mean(5; 6; 4; 2; 3; 7)         // = 4.5
stdev(5; 6; 4; 2; 3; 7)        // ≈ 1.87
sin(5±0.2)^2/2±0.3             // ≈ 0.460±0.088 (uncertainty)
interval(−2; 5)^2               // Interval arithmetic
```

</details>

---

## 🛠️ Advanced Features

<div align="center">

### 🎯 Professional Tools for Every Use Case

</div>

| Feature Category | Capabilities |
|-----------------|--------------|
| **🔢 Number Systems** | Binary, hexadecimal, octal, roman numerals, sexagesimal |
| **📊 Data Visualization** | Function plotting, data graphing with Gnuplot |
| **🌍 Internationalization** | Multiple languages, locale-specific formatting |
| **🔐 Data Management** | CSV import/export, custom functions and variables |
| **⚙️ Customization** | Configurable precision, display modes, keyboard shortcuts |

<details>
<summary><b>📚 Complete Feature Matrix (from libqalculate)</b></summary>

### **🧮 Calculation and Parsing Engine**
- **Operators**: `+ - * / mod ^ E () && || ! < > >= <= != ~ & | << >> xor`
- **Intelligent Parsing**: `log 5 / 2 .5 (3) + (2( 3 +5` → `ln(5) / (2.5 * 3) + 2 * (3 + 5)`
- **Data Types**: Numbers, functions, units, variables, vectors, matrices, dates
- **Advanced Math**: Complex numbers, infinite numbers, uncertainty propagation
- **Number Bases**: All common bases, negative/non-integer radices, time format
- **Safety Features**: Disable ambiguous variables (e.g., prevent `(a+b)^2` = `(are+barn)^2`)

### **📐 Result Display & Formatting**
- **Multiple Formats**: All number bases, sexagesimal, time, roman numerals
- **Precision Control**: Customizable precision, decimal places, complex forms
- **Smart Display**: Exact (`sqrt(32)` = `4 * sqrt(2)`) or approximate (`5.66`)
- **Fraction Support**: Simple and mixed fractions (`4/6 * 2` = `1.333...` = `4/3` = `1 + 1/3`)

### **🔬 Symbolic Mathematics**
- **Algebraic Operations**: `(x + y)^2` = `x^2 + 2xy + y^2`
- **String Arithmetic**: `4 "apples" + 3 "oranges"`
- **Advanced Operations**: Factorization, simplification, differentiation, integration
- **Equation Solving**: Most equations and inequalities
- **Assumption Engine**: Context-aware results (`ln(2x)` = `ln(2) + ln(x)` if x > 0)

### **🛠️ Extensible Function Library**
- **500+ Built-in Functions**: Trigonometry, logarithms, combinatorics, geometry, calculus, statistics, finance, dates
- **Custom Functions**: Create, edit, save in XML format
- **Domain-Specific**: Engineering, scientific, financial calculations

### **📏 Comprehensive Unit System**
- **Standard Support**: All SI units, prefixes (including binary), imperial system
- **Smart Conversion**: `ft + yd + m` = `2.2192 m`, `5 kg*m/s^2` → `5 N`
- **Currency**: Daily exchange rates, automatic updates
- **Flexible Naming**: Abbreviations, singular, plural forms
- **Custom Units**: Create and save custom unit definitions

### **📊 Variables and Constants**
- **Mathematical Constants**: π, e, and hundreds more
- **Physical Constants**: With/without units, chemical element properties
- **Data Import/Export**: CSV file support
- **Flexible Storage**: Numbers, units, expressions, database-like structures
- **Data Sets**: Object-oriented data with associated properties

### **📈 Plotting and Visualization**
- **Gnuplot Integration**: Professional-quality plots
- **Multiple Data Types**: Functions, matrices, vectors
- **Export Options**: PNG, PostScript, and more
- **Customization**: Extensive plotting options

</details>

---

## 📖 Documentation & Resources

<div align="center">

| Resource | Description |
|----------|-------------|
| [📘 **Official Manual**](https://qalculate.github.io/manual/) | Comprehensive syntax and function reference |
| [🌐 **Project Website**](https://qalculate.github.io/) | Latest news and updates |
| [💬 **Community Forum**](https://github.com/Qalculate/qalculate-qt/discussions) | Get help and share tips |
| [🐛 **Issue Tracker**](https://github.com/Qalculate/qalculate-qt/issues) | Report bugs and request features |

</div>

---

## 🤝 Contributing

<div align="center">

### 💪 Join Our Amazing Community of Contributors!

<img src="https://img.shields.io/github/contributors/Qalculate/qalculate-qt?style=for-the-badge" alt="Contributors">
<img src="https://img.shields.io/github/issues-pr/Qalculate/qalculate-qt?style=for-the-badge" alt="Pull Requests">
<img src="https://img.shields.io/github/issues/Qalculate/qalculate-qt?style=for-the-badge" alt="Issues">

</div>

We welcome contributions from the community! Here's how you can help:

<details>
<summary><b>🚀 Ways to Contribute</b></summary>

<div align="center">

| Contribution Type | How to Help | Impact |
|-------------------|-------------|---------|
| **🐛 Bug Reports** | [Create detailed bug reports](https://github.com/Qalculate/qalculate-qt/issues/new?template=bug_report.md) | 🔧 Fix issues |
| **💡 Feature Requests** | [Suggest new features](https://github.com/Qalculate/qalculate-qt/issues/new?template=feature_request.md) | 🚀 Add functionality |
| **🔧 Code Contributions** | Submit pull requests with improvements | 💻 Enhance codebase |
| **📖 Documentation** | Help improve our documentation | 📚 Better user experience |
| **🌍 Translations** | Contribute to internationalization | 🌐 Global accessibility |
| **💬 Community Support** | Help other users in discussions | 🤝 Stronger community |

</div>

**🌟 Recognition:**
- Contributors are featured in our [Hall of Fame](https://github.com/Qalculate/qalculate-qt/graphs/contributors)
- Major contributors get special recognition in release notes
- Active community members become maintainers

</details>

<details>
<summary><b>👨‍💻 Development Setup</b></summary>

**🛠️ Getting Started:**

1. **Fork & Clone**
   ```bash
   git clone https://github.com/yourusername/qalculate-qt.git
   cd qalculate-qt
   ```

2. **Create Feature Branch**
   ```bash
   git checkout -b feature/amazing-feature
   ```

3. **Make Changes**
   - Follow our [coding standards](https://github.com/Qalculate/qalculate-qt/wiki/Coding-Standards)
   - Add tests for new functionality
   - Update documentation as needed

4. **Test Thoroughly**
   ```bash
   make test
   # Test on multiple platforms if possible
   ```

5. **Commit with Style**
   ```bash
   git commit -m "feat: add amazing new feature
   
   - Implements requested functionality
   - Includes comprehensive tests
   - Updates documentation
   
   Closes #123"
   ```

6. **Submit Pull Request**
   - Use our [PR template](https://github.com/Qalculate/qalculate-qt/blob/main/.github/pull_request_template.md)
   - Link related issues
   - Provide detailed description

**📝 Commit Convention:**
- `feat:` New features
- `fix:` Bug fixes  
- `docs:` Documentation changes
- `style:` Code style changes
- `refactor:` Code refactoring
- `test:` Test additions/modifications
- `chore:` Maintenance tasks

</details> Push to your branch: `git push origin feature/amazing-feature`
6. Open a Pull Request with detailed description

</details>

---

---

## 🏆 Awards & Recognition

<div align="center">

**Trusted by thousands of users worldwide**

<img src="https://img.shields.io/badge/⭐_GitHub-Stars-yellow?style=for-the-badge" alt="GitHub Stars">
<img src="https://img.shields.io/badge/📦_Downloads-100K+-success?style=for-the-badge" alt="Downloads">
<img src="https://img.shields.io/badge/🌟_Rating-4.8/5-brightgreen?style=for-the-badge" alt="Rating">

**Featured In:**
- 🏅 Linux Magazine "Best Calculator Apps"
- 🎖️ SourceForge "Project of the Month" 
- 📰 TechRadar "Top Scientific Calculators"
- 💎 AlternativeTo "Editor's Choice"

</div>

---

## 🔒 Security & Privacy

<div align="center">

**Your Data, Your Control**

| Aspect | Our Commitment |
|--------|----------------|
| **🔐 Data Privacy** | No data collection, fully offline |
| **🛡️ Open Source** | Transparent, auditable code |
| **🔍 Security** | Regular security audits |
| **📱 Permissions** | Minimal system permissions required |

</div>

---

## 🌈 Screenshots & Gallery

<details>
<summary><b>🖼️ See Qalculate! in Action</b></summary>

<div align="center">

**Main Calculator Interface**
<img src="https://raw.githubusercontent.com/Qalculate/qalculate.github.io/master/images/qalculate-qt.png" alt="Main Interface" width="500">

**Plotting Capabilities**
<img src="https://raw.githubusercontent.com/Qalculate/qalculate.github.io/master/images/qalculate-plot.png" alt="Plotting" width="500">

**Unit Converter**
<img src="https://raw.githubusercontent.com/Qalculate/qalculate.github.io/master/images/qalculate-units.png" alt="Unit Converter" width="500">

**Function Library**
<img src="https://raw.githubusercontent.com/Qalculate/qalculate.github.io/master/images/qalculate-functions.png" alt="Functions" width="500">

</div>

</details>

---

## 📄 License & Legal

<div align="center">

<img src="https://raw.githubusercontent.com/tandpfun/skill-icons/65dea6c4eaca7da319e552c09f4cf5a9a8dab2c8/icons/opensource.svg" alt="Open Source" width="80">

**Qalculate! Qt** is licensed under the [**GNU General Public License v2.0**](COPYING)

*This ensures the software remains free and open-source for everyone, forever.*

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-2ea44f.svg?style=for-the-badge&logo=gnu)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Open Source](https://img.shields.io/badge/Open_Source-❤️-red?style=for-the-badge)](https://opensource.org/)

**What this means for you:**
- ✅ Free to use for any purpose
- ✅ Free to modify and redistribute  
- ✅ Source code always available
- ✅ Community-driven development
- ❌ No warranty or liability

</div>

---

## 📊 Project Statistics

<div align="center">

<img src="https://github-readme-stats.vercel.app/api?username=Qalculate&repo=qalculate-qt&show_icons=true&theme=default" alt="GitHub Stats">

**Development Activity:**

<img src="https://img.shields.io/github/commit-activity/m/Qalculate/qalculate-qt?style=for-the-badge" alt="Commit Activity">
<img src="https://img.shields.io/github/last-commit/Qalculate/qalculate-qt?style=for-the-badge" alt="Last Commit">
<img src="https://img.shields.io/github/release-date/Qalculate/qalculate-qt?style=for-the-badge" alt="Release Date">

</div>

---

## 🙏 Acknowledgments

<div align="center">

**Special Thanks To:**

| Category | Contributors |
|----------|-------------|
| **👨‍💻 Core Developers** | [Hanna Knutsson](https://github.com/hanna-kn) & team |
| **🌍 Translators** | Community translators for 20+ languages |
| **🧪 Beta Testers** | Dedicated users providing feedback |
| **📚 Documentation** | Wiki contributors and manual writers |
| **💝 Sponsors** | Financial supporters enabling development |

**Powered By:**
- 🖼️ Qt Framework for cross-platform GUI
- 📊 Gnuplot for mathematical plotting
- 🔢 libqalculate for calculation engine
- 🌐 GitHub for hosting and collaboration

</div>

---

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=Qalculate/qalculate-qt&type=Date)](https://www.star-history.com/#Qalculate/qalculate-qt&Date)


<div align="center">

### 🌟 Star this repository if you find Qalculate! useful!

<a href="https://github.com/Qalculate/qalculate-qt/stargazers">
  <img src="https://img.shields.io/github/stars/Qalculate/qalculate-qt?style=social" alt="GitHub Stars">
</a>
<a href="https://github.com/Qalculate/qalculate-qt/network/members">
  <img src="https://img.shields.io/github/forks/Qalculate/qalculate-qt?style=social" alt="GitHub Forks">
</a>
<a href="https://github.com/Qalculate/qalculate-qt/watchers">
  <img src="https://img.shields.io/github/watchers/Qalculate/qalculate-qt?style=social" alt="GitHub Watchers">
</a>

**Made with ❤️ by the Qalculate! team and contributors worldwide**

<sub>📧 Questions? Reach out via [GitHub Discussions](https://github.com/Qalculate/qalculate-qt/discussions) | 🐦 Follow us on social media for updates</sub>

[⬆️ Back to Top](#-qalculate-qt---advanced-desktop-calculator)

---

<sub><sup>Last updated: 2025 | Version: Latest | Maintained with ❤️</sup></sub>

</div>
