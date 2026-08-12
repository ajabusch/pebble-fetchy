# pebble-fetchy (Fetchy)

A Pebble watch app to quickly send preconfigured HTTP requests directly from the watch. The three physical buttons **UP**, **OK** and **DOWN** each trigger a request you set up in advance — either with a **short press** or a **long press**.

## Icon & Banner

| Asset | Source | Size | Used for |
|-------|--------|------|----------|
| [`resources/images/app_icon.png`](resources/images/app_icon.png) | `img/Icon-144.jpg` | 25×25 | Watch app menu icon (`IMAGE_MENU_ICON`) |
| [`img/Icon-80.jpg`](img/Icon-80.jpg) | – | 80×80 | App icon (smaller platform) |
| [`img/Icon-144.jpg`](img/Icon-144.jpg) | – | 144×144 | App icon (higher resolution) |
| [`img/banner.jpg`](img/banner.jpg) | – | 720×320 | Store / web banner |

The 25×25 menu icon is bundled into the `.pbw` via [`package.json`](package.json) (`resources.media`). The larger `img/` files are used when submitting the app to the store / publishing.

> Requires a paired Pebble app (Android/iOS), or the official [Repebble](https://developer.repebble.com) development stack for emulator and installation.

---

For now this looks best on a new pebble time 2, but it works on other pebbles too.
Maybe I fix the design for the other watches later on.

---

Props to Paul Em! And thanks to https://greenpt.com/ (EU/DSGVO/ECO/Water saving/privacy optimized) LLM-API as a little helper, I was able to create this App ... my C is a little rusty.

---

## Features

- **6 configurable requests** — two per button: a **short press** (UP / OK / DOWN) and a **long press** (UP-Hold / OK-Hold / DOWN-Hold).
- **HTTP methods**: `GET`, `POST`, `PUT`, `PATCH`, and more (with optional body for the latter).
- **Custom headers** per request.
- **On-watch status feedback**: success (`OK (200)`) or failure (`Failed (4xx)`, timeout `408`) with haptic feedback.
- **Persistent configuration backup** on the watch (`persist`) so the setup can be restored when the phone-side app is reopened.
- **Request names** displayed on the watch, surviving a reboot.

---

## Requirements

- [Repebble SDK](https://developer.repebble.com/sdk/) (tested with **SDK v4.17**, `pebble-tool` v5.x)
- ARM toolchain (bundled with the SDK, `arm-none-eabi-gcc`)
- `node`/`npm` (for the JavaScript part)

The required toolchain is usually shipped with the SDK; `arm-none-eabi-gcc` and `qemu-pebble` (emulator) live under the SDK `toolchain/` directory.

---

## Project structure

```
├── src/
│   ├── c/           # Pebble C watch application code (main.c)
│   └── pkjs/        # PebbleKit JavaScript (index.js) — HTTP requests, configuration
├── resources/
│   └── images/      # Bundled watch resources (app menu icon)
├── img/             # Store assets (icons + banner)
├── wscript          # Platform Action build script (SDK)
├── package.json     # Project metadata & Pebble configuration (SDK 3)
└── build/           # Build output (pebble-fetchy.pbw)
```

| File | Purpose |
|------|---------|
| [`src/c/main.c`](src/c/main.c) | Watch app: button handling, Press handling, status display, persistent config storage |
| [`src/pkjs/index.js`](src/pkjs/index.js) | Phone/JS side: XHR requests, config webview, local storage & backup |
| [`resources/images/app_icon.png`](resources/images/app_icon.png) | 25×25 watch app menu icon (`IMAGE_MENU_ICON`) |
| [`img/`](img/) | Store icons (`Icon-80.jpg`, `Icon-144.jpg`) and banner (`banner.jpg`) |
| [`wscript`](wscript) | Waf build rules |
| [`package.json`](package.json) | App metadata, UUID, targets, message keys |

---

## Quick start

```sh
# Build for all platforms
pebble build

# Install and launch in the emulator (Pebble Time 2 = "emery")
pebble install --emulator emery

# Install to a physical device (IP of the phone running the Pebble app)
pebble install --phone <IP>

# Follow live logs from watch/emulator
pebble logs --emulator emery
```

After a successful build the app is available at `build/pebble-fetchy.pbw`.

### Emulator - Buttons

- Left Arrow: Back button
- Up Arrow: Up button
- Down Arrow: Down button
- Right Arrow: Select button

---

## Configuration

Requests are configured via the **configuration web page**, opened from within the Pebble app (in the app's settings). You can set a name, URL, method, headers and body for each of the **six** button actions — one per short press (**Up**, **Select**, **Down**) and one per long press (**Up-Hold**, **Select-Hold**, **Down-Hold**). A long press (holding the button for about 0.6&nbsp;s) triggers the corresponding *Hold* request.

- The configuration is kept in the JS side's `localStorage` and additionally backed up to the watch's persistent storage.
- On startup the app checks whether local configuration exists; if not, it requests the backup from the watch.

---

## Target platforms

From [`package.json`](package.json:12):

| Platform | Device |
|----------|--------|
| `aplite`  | Pebble / Pebble Steel |
| `basalt`  | Pebble Time / Time Steel |
| `chalk`   | Pebble Time Round |
| `diorite` | Pebble 2 |
| `emery`   | **Pebble Time 2** |

The emulator command uses the same names (`--emulator emery`, `basalt`, …).

---

## License

MIT — see [`LICENSE`](LICENSE).

---

## Notes

- The project uses **SDK 3** (`package.json`) and is built with the **Repebble SDK 4.17**.
- The configuration URL currently points to `https://ajabusch.github.io/pebble-fetchy/` (see [`src/pkjs/index.js`](src/pkjs/index.js:1)); this is served from the `docs/` folder via GitHub Pages.
- Communicating between the watch and the phone requires a **paired phone** or a running emulator.