# tank trouble clone

c+raylib. lots of tunables since linux users sure like those

## features

* better bullet balancing.
  * lifetime has a lower cap (less spam)
  * bullets can collide with each other for mutual destruction (more big brain plays)
  * each tank color can only have a set amount of bullets
  active on-screen at once (less spam)
* more tunables. see `src/config.h`
  * tune bullet speed, max bullet bounce, max bullet lifetime, bullet speed, etc.
  * tune tank speed, turning, scale
  * tune maze generation, size (no custom mazes yet)
  * for deeper tuning, the code is kept intentionally simple to
  allow patching. for example adding more than 4 players.

## build

use a c compiler and have the raylib game library on your PATH.

### linux

After installing raylib (libraylib-dev or raylib depending on distribution), run

```bash
make
./tanktrouble
```

If your compositor uses fractional scaling on Wayland,
[gamescope](https://wiki.archlinux.org/title/Gamescope)
can be used to deal with potential pixel math issues.

```bash
make
gamescope -w 1280 -h 720 -- ./tanktrouble
```

### windows

todo

### mac

todo
