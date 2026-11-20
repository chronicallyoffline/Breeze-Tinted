# Breeze Tinted

A fork of the Breeze window decoration (Plasma 6.8) that gives each window its own outline and title bar tint, taken from that window's colors.

Two plugins:

- **`decoration/`** is the `org.kde.breezetinted` KDecoration3 plugin plus its KCM. It works on its own.
- **`effect/`** is the optional `tintsampler` KWin effect. It samples each window's contents and passes the color to the decoration.

## How a window's color is chosen

The first source that yields a color wins:

1. **Per-app override.** In the decoration settings, go to *Window-Specific Overrides* and set *Tint color*. *Don't tint this window* draws the window like stock Breeze.
2. The automatic sources, in the order set on the *Tint* tab (by default: color scheme, icon, window contents). Each can be switched off.
   - **Application color scheme**: the window's highlight color (or failing that, its background), but only when it differs from the global scheme. This covers KDE apps that set their own scheme, and the KWin window rule *Titlebar color scheme*.
   - **Application icon**: the dominant saturated hue of the icon.
   - **Window contents**: the average color of the client area's edges (or its whole area), from the `tintsampler` effect. Samples that are close to gray are skipped.
3. **The global accent color.**

The resolved color is then shaped (saturation, a lightness clamp, an optional blend with the accent) and applied:

- **Outline**: blended from the stock Breeze outline towards the tint, with separate intensities for active and inactive windows. Thickness is adjustable.
- **Title bar**: blended from the stock title bar color towards the tint, again with separate active and inactive strengths. If the blend makes the title text unreadable, the text switches to black or white.

When a window's color changes, it cross-fades over the *Color transition* time.

## Build and install

```sh
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_EFFECT=ON
cmake --build build
sudo cmake --install build
```

Then go to *System Settings → Colors & Themes → Window Decorations*, pick **Breeze Tinted**, and click its edit button to configure it.

**Updating:** KWin keeps a plugin loaded once it has used it, so after installing a new build, log out and back in. Running `qdbus6 org.kde.KWin /KWin reconfigure` or reopening the settings is not enough. The settings page runs in its own process, so it shows new options right away, but they have no effect until KWin restarts.

`BUILD_EFFECT` is off by default. KWin's C++ effect API is not stable, so **the effect must be rebuilt after every Plasma update**. The decoration only depends on the stable KDecoration3 API, so it doesn't have this problem.

### The tintsampler effect

Enable it under *System Settings → Window Management → Desktop Effects → Tint Sampler*, or with:

```sh
kwriteconfig6 --file kwinrc --group Plugins --key tintsamplerEnabled true
qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect tintsampler
```

It has no settings page yet. Configure it in `kwinrc`, then reload the effect:

```ini
[Effect-tintsampler]
# minimum time between two samples of the same window, in ms (default 1000)
Interval=1000
# Edges (default): a thin strip along the client area's edges, usually toolbars and header bars
# Average: the whole client area
Mode=Edges
# comma separated, case-insensitive regexes matched against the window class
Blocklist=mpv,steam_app_.*
```

A window is only resampled after its contents change, and at most once per `Interval`. A sample renders the client area at 32 px and reads back about 4 KiB. When the effect is unloaded, every decoration goes back to its other sources.

## Testing without touching your session

The decoration and effect can be run in a headless KWin with its own config and D-Bus:

```sh
mkdir -p /tmp/tinted/plugins/org.kde.kdecoration3 /tmp/tinted/plugins/kwin/effects/plugins /tmp/tinted/config
ln -sf "$PWD/build/bin/org.kde.breezetinted.so" /tmp/tinted/plugins/org.kde.kdecoration3/
ln -sf "$PWD/build/bin/kwin/effects/plugins/tintsampler.so" /tmp/tinted/plugins/kwin/effects/plugins/
printf '[org.kde.kdecoration2]\nlibrary=org.kde.breezetinted\ntheme=Breeze Tinted\n\n[Plugins]\ntintsamplerEnabled=true\n' > /tmp/tinted/config/kwinrc
cp ~/.config/kdeglobals /tmp/tinted/config/

env -u WAYLAND_DISPLAY -u DISPLAY XDG_CONFIG_HOME=/tmp/tinted/config QT_PLUGIN_PATH=/tmp/tinted/plugins:/usr/lib/qt6/plugins \
  dbus-run-session -- kwin_wayland --virtual --width 1600 --height 1000 --socket wayland-tinted \
  --exit-with-session ./your-session-script.sh
```

Inside the session script, launch apps with `WAYLAND_DISPLAY=wayland-tinted`, and take a screenshot with `spectacle -b -n -f -o shot.png`. Replace `--virtual` with `--windowed` to watch it in a nested window instead.

## Credits

Based on [Breeze](https://invent.kde.org/plasma/breeze) 6.8.2 (`kdecoration/` and `libbreezecommon/`). The original copyright headers are kept. The decoration is GPL-2.0-only OR GPL-3.0-only, like Breeze; the effect is GPL-2.0-or-later, like KWin.
