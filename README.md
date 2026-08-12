# pebble-fetchy (Fetch)

A Pebble watch app to quickly send preconfigured HTTP requests directly from the watch. The three physical buttons **UP**, **OK** and **DOWN** each trigger a request you set up in advance.

> Requires a paired Pebble app (Android/iOS), or the official [Repebble](https://developer.repebble.com) development stack for emulator and installation.

---

## Features

- **3 configurable requests** — one per button (UP / OK / DOWN).
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
│   ├── c/           # Pebble C watch app code (main.c)
│   └── pkjs/        # PebbleKit JavaScript (index.js) — HTTP requests, configuration
├── wscript          # Waf build script (SDK)
├── package.json     # Project metadata & Pebble configuration (SDK 3)
└── build/           # Build output (pebble-fetchy.pbw)
```

| File | Purpose |
|------|---------|
| [`src/c/main.c`](src/c/main.c) | Watch app: button handling, status display, persistent config storage |
| [`src/pkjs/index.js`](src/pkjs/index.js) | Phone/JS side: XHR requests, config webview, local storage & backup |
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

---

## Configuration

Requests are configured via the **configuration web page**, opened from within the Pebble app (in the app's settings). You can set a name, URL, method, headers and body for each of the three buttons.

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
- The configuration URL currently points to `https://paul-em.github.io/pebble-fetch/` (see [`src/pkjs/index.js`](src/pkjs/index.js:1)).
- Communicating between the watch and the phone requires a **paired phone** or a running emulator.