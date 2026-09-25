# QHana

A modern C++20 Hanafuda card game (Koi-Koi, Koi-Koi [BET], and Korean Go-Stop) featuring both a **Qt6 GUI** (`qhana`) and a **btop-inspired ncurses TUI** (`qhana-tui`), based on [SDLHana](http://sdlhana.nongnu.org/).

## Description

QHana implements the traditional 48-card Japanese flower card game (**Hanafuda**) with three distinct rule modes:
- **Koi-Koi**: Traditional Japanese rules (8 cards dealt, Five/Four/Three Lights, Boar-Deer-Butterfly, Flower/Moon Viewing Sake, Red/Blue Poetry Ribbons, Animals, Chaff, and Dealer privilege).
- **Koi-Koi [BET]**: Fast-paced 6-card betting mode with an interactive Big/Small Double-Up mini-game on winning hands.
- **Go-Stop**: Korean 2-player rules with 3-point minimum threshold, Godori (Five Birds), Grass Ribbons, Ppeok (Three-card stack bonuses), Ttadak / Sweep opponent chaff stealing, and Gwang-bak / Pi-bak / Mung-tta / Go multipliers.

## Project Structure

- `libqhana/`: Shared C++20 Hanafuda engine, card definitions, rule evaluator, Bot AI, and multi-language string table (English, French, Japanese).
- `app/`: Qt6 Widgets + QtCharts graphical application (`qhana`) with custom Hanafuda card table canvas, smooth animations, yaku breakdown, score history chart, and dialogs.
- `tui/`: Terminal User Interface (`qhana-tui`) built with wide-character `ncursesw`, featuring Unicode card art, Braille score history charts, and full keyboard/mouse navigation.

## Prerequisites

- **C++ Compiler** supporting C++20 (GCC 11+, Clang 13+, or MSVC)
- **CMake** (version 3.24 or higher)
- **Qt 6** development libraries (`qt6-base-dev`, `qt6-charts-dev`)
- **NCurses** development libraries (`libncurses-dev`)

On Debian/Ubuntu-based distributions:
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-charts-dev libncurses-dev
```

## Building and Running

### 1. Configure

```bash
cmake -S . -B build
```

### 2. Compile

```bash
cmake --build build --config Release
```

### 3. Run

Qt6 GUI application:
```bash
./build/bin/qhana
```

NCurses TUI application:
```bash
./build/bin/qhana-tui
```

## License

GPL v2 - see [LICENSE](LICENSE) for details.

## Authors & Credits

- **Original SDLHana Author**: Wei Mingzhi <whistler@openoffice.org>
- **Qt6 & TUI Port**: Mizux
