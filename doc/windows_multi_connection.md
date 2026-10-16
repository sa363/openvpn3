# Several simultaneous connections on Windows

This document describes how to build and run the OpenVPN 3 pieces that let
several VPN connections run at the same time on Windows: the multi-session
`ovpnagent` service, the `omicli`/`omicliagent` workers and the Qt tray GUI
in [gui/](../gui), and how to package them as an MSI.

## Architecture

```
ovpn3-gui.exe (user)
   │  OpenVPN management interface on 127.0.0.1, one port per connection
   ▼
omicliagent.exe ×N   one process per profile (openvpn/omi)
   │  HTTP over the named pipe \\.\pipe\ovpnagent
   ▼
ovpnagent.exe        Windows service running as LocalSystem (openvpn/ovpnagent/win)
                     one TunSession per client process
```

- **ovpnagent** sets up the tunnels for unprivileged clients. It keeps one
  session per client process, keyed by the PID of the named pipe client,
  so tunnels of different clients don't tear each other down. Across
  sessions it
  - excludes the VPN interfaces of all tunnels when choosing the gateway
    of a server bypass route, so it goes through the physical network;
  - lets the WFP filters of `block-outside-dns` / `block-local` of one
    tunnel permit the traffic of the others;
  - allows only one tunnel that redirects the default gateway;
  - shares the `block-ipv6` blackhole routes and keeps them until the last
    tunnel that needs them is gone;
  - reports routes that two tunnels have in common (`ROUTE CONFLICT`).
- **omicliagent** is the OpenVPN 3 client with the OpenVPN management
  interface (OMI), the protocol OpenVPN GUI speaks. It inlines files the
  profile refers to (`ca ca.crt`, `tls-auth ta.key`, ...) and falls back from
  ovpn-dco-win to TAP or Wintun for profiles that can't use DCO
  (`compress`, `comp-lzo`, `http-proxy`). `omicli` is the same client
  without the agent; it needs administrator rights.
- **ovpn3-gui** runs a worker per connection and talks to it over OMI. It
  does not link the core library and only needs Qt.

Each simultaneous connection needs a VPN adapter of its own. ovpn-dco-win
and TAP adapters must exist beforehand; the client uses the first free one.

## Prerequisites

| Tool | Used for | Notes |
|---|---|---|
| Visual Studio 2022 with "Desktop development with C++" | everything | MSVC v143, Windows SDK |
| CMake 3.21 or later | everything | comes with Visual Studio |
| [vcpkg](https://github.com/microsoft/vcpkg) | core dependencies | set `VCPKG_ROOT` to the checkout |
| Qt 6.5 or later, `msvc2022_64` | GUI | Widgets, Network, LinguistTools |
| .NET SDK 8 and WiX 5.0.2 | MSI | only for the installer |
| ovpn-dco-win and/or TAP-Windows6 driver | running | e.g. from OpenVPN Community or OpenVPN Connect |

Qt can be installed without the Qt online installer, for example with
[aqtinstall](https://github.com/miurahr/aqtinstall):

```bash
python -m pip install --user aqtinstall
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -O C:/Qt
```

The .NET SDK and WiX can be installed per user, without administrator rights:

```powershell
powershell -ExecutionPolicy Bypass -File dotnet-install.ps1 -Channel 8.0 -InstallDir $env:LOCALAPPDATA\Microsoft\dotnet
$env:DOTNET_ROOT = "$env:LOCALAPPDATA\Microsoft\dotnet"
& "$env:DOTNET_ROOT\dotnet.exe" tool install --global wix --version 5.0.2
```

`dotnet-install.ps1` is available from <https://dot.net/v1/dotnet-install.ps1>.

## Building the core, the agent and the workers

The `win-x64-release` preset uses the Visual Studio generator, the vcpkg
toolchain and enables ovpn-dco-win support (`CLI_OVPNDCOWIN=ON`):

```bash
set VCPKG_ROOT=C:\path\to\vcpkg
cmake --preset win-x64-release
cmake --build build/msvc/amd64 --config Release --target ovpnagent omicliagent omicli
```

The binaries end up in

- `build/msvc/amd64/openvpn/ovpnagent/win/Release/ovpnagent.exe`
- `build/msvc/amd64/openvpn/omi/Release/omicliagent.exe` and `omicli.exe`,
  next to the DLLs they need.

### Unit tests

```bash
cmake --build build/msvc/amd64 --config Release --target coreUnitTests
build\msvc\amd64\test\unittests\Release\coreUnitTests.exe --gtest_filter=BestGateway*
```

`IpHelperTest.*` adds real routes and needs administrator rights. The
`*Legacy*` SSL tests need the OpenSSL legacy provider module.

## Building the GUI

```bash
cmake -S gui -B build/gui -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build build/gui --config Release
```

The build

- copies `omicliagent.exe`, `omicli.exe` and their DLLs from
  `OVPN3_WORKER_DIR` (default `build/msvc/amd64/openvpn/omi/Release`),
- runs `windeployqt`, so `build/gui/Release` can be started directly,
- embeds the translations and Qt's own translations of the same languages.

### Translations

The interface follows the system language; Russian is included. After
changing user visible strings, refresh the `.ts` files and translate the new
entries, e.g. with Qt Linguist:

```bash
cmake --build build/gui --target update_translations
```

To add a language, append it to `OVPN3_GUI_LANGUAGES` in
[gui/CMakeLists.txt](../gui/CMakeLists.txt).

## Building the MSI

Build the core and the GUI first, then:

```bash
powershell -ExecutionPolicy Bypass -File gui/installer/build.ps1
```

The script stages the GUI with its Qt runtime, the workers, `ovpnagent.exe`
and the MSVC runtime (deployed next to the binaries), and builds
`build/installer/OpenVPN3-<version>-x64.msi`. Use `-Wix <path>` if `wix` is
not on `PATH`, and `-CoreBuild` / `-GuiBuild` for other build directories.

Check the package with

```bash
wix msi validate build/installer/OpenVPN3-<version>-x64.msi
```

The package

- installs per machine to `Program Files\OpenVPN3`,
- registers `ovpnagent` as an automatically started LocalSystem service
  that is restarted on failure,
- adds a Start menu shortcut and, as an optional feature, starts the GUI
  at logon,
- offers to start the GUI as the installing user at the end.

The installer UI is Russian (`-culture ru-RU` in `build.ps1`). The version
comes from `project(ovpn3-gui VERSION ...)` in `gui/CMakeLists.txt`; bump it
for every package, otherwise Windows Installer installs a second copy
instead of upgrading.

No driver is included: ovpn-dco-win or TAP-Windows6 must already be
installed. Uninstalling keeps the per-user data (profiles, settings, saved
passwords).

## Running without the installer

1. Start the agent from an elevated prompt, either as a service or in the
   foreground for debugging:

   ```bash
   ovpnagent.exe install
   sc start ovpnagent
   ```

   ```bash
   ovpnagent.exe run
   ```

   Don't do both: an agent refuses to start while another one serves
   `\\.\pipe\ovpnagent`.
2. Put profiles (`*.ovpn`, `*.conf`, subfolders allowed) into
   `%USERPROFILE%\OpenVPN3\config` or import them in the GUI.
3. Start `build\gui\Release\ovpn3-gui.exe`. Starting it again brings up the
   window of the running instance; `--connect <profile>` and
   `--import <file>` are forwarded to it.

Without the agent, select `omicli.exe` as the worker in the settings and run
the GUI as administrator.

## Logs and troubleshooting

| What | Where |
|---|---|
| Agent | `agent.log` next to `ovpnagent.exe` when running as a service, the console with `ovpnagent run` |
| Workers | `%LOCALAPPDATA%\OpenVPN3\ovpn3-gui\logs\<profile>.log`, the last 5 sessions as `<profile>.1.log` ... `<profile>.5.log` |
| GUI settings, saved usernames and DPAPI encrypted passwords | `HKCU\Software\OpenVPN3\ovpn3-gui` |

Common problems:

- **"ovpnagent service is not running"**: the GUI found no
  `\\.\pipe\ovpnagent`. Start the service, or use `omicli.exe`.
- **"ovpnagent: communication error" for every connection**: another agent
  created the pipe first and is gone; Windows still reports it as the pipe's
  server and clients fail to validate it. Restart the service. Current
  agents refuse to start next to each other.
- **"No free VPN adapter for this connection"** (`cannot acquire TAP
  handle`): every connection needs its own adapter. The worker log shows the
  driver that was used; add an adapter from an elevated prompt, e.g.
  `tapctl create --hwid ovpn-dco` from OpenVPN Community.
- **"the default gateway is already redirected"**: only one connection can
  use `redirect-gateway`, also when the server pushes it.
- **"Overlapping routes"**: two connections route the same network, traffic
  to it may go through either tunnel. This is a server side configuration.
- **Absolute paths in profiles**: inside quotes a backslash escapes the next
  character, as in OpenVPN 2.x. Use `C:\\path\\ca.crt` or `C:/path/ca.crt`.

## Limitations

- Only OpenVPN 2.x client profiles in TLS mode are supported. `secret`,
  `fragment`, `dev tap`, `socks-proxy` and `cryptoapicert` stop the
  connection; scripts and plugins (`up`, `down`, `plugin`, ...) are ignored.
- `block-ipv6` pushed by a server blocks global IPv6 on the whole machine,
  also for split tunnels.
- The conflict check of the GUI before connecting only looks at the
  profile files; the agent checks the routes that are actually used.
