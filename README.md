# LOQ RGB Studio

A native Qt 6 desktop profile editor and four-zone keyboard lighting simulator for the Lenovo LOQ 15AHP9 (machine type 83DX, ITE8295 controller).

## Hardware safety

The app starts in **simulation mode**. Linux hardware mode is available for the ITE8295 at USB `048D:C993`, interface 0 only. It discovers a matching hidraw node at runtime, checks the USB identity and the interface-0 Feature descriptor, and does not rely on a fixed `/dev/hidrawN` name. It does not open interface 1 or detach a kernel driver.

Each hardware apply requires a confirmation dialog that displays the complete candidate packet. The protocol payload is based on the `research/LegionAura/` reference, not independently verified on this keyboard. The descriptor declares 32 Feature data bytes; the corresponding hidraw report buffer includes report ID `0xCC`, for 33 bytes total. The observed GET response was only 11 bytes, and does not validate SET payload semantics. The app reports successful transport completion, not verified lighting output. Do not confirm a hardware apply unless you accept this uncertainty. Fn + Space remains available as the manual lighting recovery control.

The preview intentionally represents four broad lighting zones. It does not claim per-key RGB support, and its key layout and zone boundaries are estimates rather than physically verified measurements. The user-facing effects are Static, Wave, and Breathing. Static RGB is user-reported working; Wave and Breathing remain reference-derived and physically unverified. Software animation previews are never streamed frame-by-frame to hardware.

## Features

- A focused Presets, Hardware, and Settings navigation; the preset library is the opening page.
- Eight built-in read-only presets: Rainbow Wave, Reverse Rainbow, Ocean Wave, Aurora Wave, Spectrum Wave, Colour Breathing, White Breathing, and Zone Breathing.
- Editable custom presets with create, edit, rename, duplicate, delete (confirmation required), import, and export. Custom presets persist in saved order and survive restarts. Built-ins can only be customized by creating a copy.
- A focused editor with exactly Static, Wave, and Breathing. Static supports one colour for all zones or independent zone colours; Wave supports multi-colour or four fixed section colours plus direction; Breathing supports single, multi-colour, or individual zone colours.
- Live, smooth software preview using the existing keyboard visualization and four-zone profile model. HEX colour inputs and brightness are available in the editor; speed and direction are shown only for relevant effects.
- Visible Cycle My Presets control and `Ctrl+Alt+N` application shortcut; cycling traverses custom presets only and wraps in saved order. Hardware Apply continues through the existing confirmation dialog.
- Explicit simulation/hardware mode selection; hardware apply runs off the UI thread and only persists the profile after a successful transfer.
- Versioned JSON schemas (`schemaVersion: 1`) and persisted last successfully applied profile.
- System tray, minimize-to-tray, tray restore/apply/quit actions, and optional XDG autostart.

The application does not intercept Fn + Space. The exposed Linux keyboard capability metadata lists `KEY_SPACE` but not `KEY_FN`; I did not read live key events, so whether firmware emits any alternate Fn+Space event remains unknown. The app does not grab keyboard input or request broad input permissions; use its visible cycle control or `Ctrl+Alt+N` instead. Fn + Space remains available for Lenovo's native lighting control and manual recovery.

Custom animation presets and the last applied profile are stored below Qt's `AppConfigLocation` (on Ubuntu, normally under `~/.config/`). Preset JSON stores the four-zone profile and its colour mode. Imported and exported files use JSON. An example profile object is:

```json
{
  "schemaVersion": 1,
  "name": "Midnight",
  "effect": "Wave",
  "brightness": 80,
  "speed": 50,
  "direction": "ltr",
  "zones": ["#18d9ff", "#a43cff", "#ff3d88", "#ffb02e"]
}
```

Autostart is opt-in from Settings. It creates or removes only this application's file in `~/.config/autostart`. Login restoration starts in simulation mode and does not send hardware commands.

## Build on Ubuntu

Install dependencies yourself after checking your Ubuntu release and package availability (the project does not install packages):

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-base-dev-tools
```

The Linux hardware backend uses the Linux hidraw API and requires no HIDAPI package. No udev rules are included or installed. If the current user lacks access to the matching hidraw node, configure device permissions separately; the app does not elevate privileges or change permissions. The package versions supplied by Ubuntu depend on the release; CMake requires Qt 6.4 or newer.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/loq-rgb-studio
```

### Read-only ITE8295 HID diagnostic

The standalone `ite8295-hid-read` utility opens only `/dev/hidraw0` in read-only
mode and issues exactly one `HIDIOCGFEATURE(33)` request for report ID `0xCC`.
It prints the returned bytes, checks the report ID, and warns if the response
is shorter than the descriptor-sized 33-byte buffer. A short response is not
interpreted as RGB state, and the utility does not retry. It does not send
Feature SET reports, change keyboard lighting, or access another device.

Build the utility:

```bash
cmake --build build --target ite8295-hid-read
```

Run it only when explicitly ready to make the read-only feature request:

```bash
./build/ite8295-hid-read
```

No automatic privilege escalation or permission changes are performed. If
`/dev/hidraw0` is not accessible to the current user, the utility reports the
open error and exits.

For a local system install, use `cmake --install build --prefix ~/.local`. The app launcher and SVG icon are installed under the selected prefix.

## Architecture

- `src/profile.*`: validated four-zone domain model, built-in presets, versioned JSON conversion.
- `src/animationengine.*`: time-stepped four-zone preview engine, isolated from UI and transport.
- `src/animationpresets.*`: built-in/custom presets, colour modes, serialization, mutation guards, and custom cycling order.
- `src/themestore.*`: custom-theme, custom animation-preset, and last-applied JSON persistence plus import/export.
- `src/hardware/keyboardbackend.*`: backend abstraction, simulation backend, mockable ITE8295 backend, and worker object.
- `src/hardware/ite8295protocol.*`: HID descriptor Feature-length parser and candidate packet builder (reference-derived and not device-verified).
- `src/hardware/linuxhidrawtransport.cpp`: runtime Linux hidraw discovery and Feature SET transport for verified USB identity/interface 0 only.
- `src/keyboardpreview.*`, `src/mainwindow.*`: custom painted keyboard widget and Qt Widgets application.
- `tests/tst_core.cpp`: profile/schema, preset, simulation, packet, descriptor, fake sysfs discovery, mock transport, and animation tests. Tests do not write to hardware.

## License and research

The application is authored for this project. `research/LegionAura/` is treated as reference material only and is not built, copied, or modified by the app. Its MIT license applies to that separate research source.
