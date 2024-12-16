# ✨starsaver

View stars, planets, and more, right in your terminal!

![](/assets/screenshot.png)

## Building

### Requirements

- Linux or macOS
- [Meson](https://github.com/mesonbuild/meson) 0.49 or newer
- [Ninja](https://github.com/ninja-build/ninja) 1.8.2 or newer
- A C compiler
- ncurses library

### Installation

Clone this repo in your local system and enter on of the project directories.

Initialize the builder:

```
meson setup build
```

To rebuild from then on from within the newly created `../build/` directory:

```
meson compile
```

To install to default location (this may require sudo privileges):

```
meson install
```

_Note, after rebuilding any changes, rerunning `meson install` is required_

### Known issues

- Ncurses detection is spotty on some systems, and you may need to install [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) in order for Meson to find it
- Many unicode characters will not render (at all) on macOS
- The Moon's phase is currently not correct
- Azimuthal grid drawing needs improvement
- Only a few constellation figures are currently available

## Usage

After installing *starsaver*, simply run `starsaver` to invoke it.

```
starsaver
```

Pressing ESC will exit.

### Basic Options

Say we wanted to view the sky at 5:00 AM on July 16, 1969—the morning of the
Apollo 11 launch at the Kennedy Space Center in Florida. We would run:

```
starsaver --latitude 28.573469 --longitude -80.651070 --datetime 1969-7-16T9:32:00
```

If we then wanted to display all stars with a magnitude brighter than or equal
to 5.0 and add color, we would add `--threshold 5.0 --color` as options.

If you simply want the current time, don't specify the `--datetime` option and
_starsaver_ will use the system time. For your current location, you will still
have to specify the `--lat` and `--long` options.

For more options and help run `starsaver -h` or `starsaver --help`

> ℹ️ Use a tool like https://www.latlong.net/ to get your latitude and longitude

> ℹ️ Star magnitudes decrease as apparent brightness increases

## Data Sources

- Stars: [Yale Bright Star Catalog](http://tdc-www.harvard.edu/catalogs/bsc5.html)
- Star names: [IAU Star Names](https://www.iau.org/public/themes/naming_stars/)
- Constellation figures: [Stellarium](https://stellarium.org/)
- Planet orbital elements: [NASA Jet Propulsion Laboratory](https://ssd.jpl.nasa.gov/planets/approx_pos.html)
- Planet magnitudes: [Computing Apparent Planetary Magnitudes for The Astronomical Almanac](https://arxiv.org/abs/1808.01973)
