# OpenVPN3 GUI

See [doc/windows_multi_connection.md](../doc/windows_multi_connection.md) for the
complete build of the core, agent, workers, GUI and installer.

Tray application for Windows that runs several VPN connections at the same
time, in the spirit of OpenVPN GUI.

## Architecture

```
ovpn3-gui.exe (user)  ── management interface (127.0.0.1) ──►  omicliagent.exe, one per profile
                                                                      │ named pipe
                                                                      ▼
                                                    ovpnagent service (SYSTEM): one TunSession per client PID
```

- `ovpn3-gui.exe` lists the profiles, starts a worker per connection and talks
  to it over the OpenVPN management protocol (state, log, byte counts,
  username/password, static and dynamic challenge, web authentication).
- `omicliagent.exe` (`openvpn/omi`) is the OpenVPN3 client with a management
  interface. It sets up the tunnel through `ovpnagent`, so no administrator
  rights are needed.
- `ovpnagent.exe` (`openvpn/ovpnagent/win`) is the privileged service. It keeps
  one tunnel per client process, so connections don't tear each other down.

The GUI does not link the core library; it only needs Qt.

## Building

Requires Qt 6.5 or later (Widgets, Network) and the core build, which
provides the workers.

```bash
cmake -S gui -B build/gui -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build build/gui --config Release
```

The build copies `omicliagent.exe`, `omicli.exe` and their DLLs from
`OVPN3_WORKER_DIR` (default `build/msvc/amd64/openvpn/omi/Release`) and runs
`windeployqt`, so `build/gui/Release` is directly runnable.

## Installer

`installer/build.ps1` stages the GUI, the workers, `ovpnagent.exe` and the
MSVC runtime from the existing builds and packs them into an MSI with WiX 5
(`dotnet tool install --global wix --version 5.0.2`):

```bash
powershell -ExecutionPolicy Bypass -File gui/installer/build.ps1
```

The result is `build/installer/OpenVPN3-<version>-x64.msi`. It installs to
`Program Files\OpenVPN3`, registers and starts the `ovpnagent` service, adds a
Start menu shortcut and optionally starts the GUI at logon. The installer UI is
in Russian. A TAP, Wintun or ovpn-dco-win driver is not included and has to be
present already, e.g. from OpenVPN Community or OpenVPN Connect.

Uninstalling keeps the per-user data: profiles, settings and saved passwords.

## Translations

The interface follows the system language; Russian is included. After
changing user visible strings, refresh the `.ts` files and translate the new
entries, e.g. with Qt Linguist:

```bash
cmake --build build/gui --target update_translations
```

To add a language, append it to `OVPN3_GUI_LANGUAGES` in `CMakeLists.txt`.

## Running

1. Install and start the agent from an elevated prompt:
   `ovpnagent.exe install`, or `ovpnagent.exe run` in the foreground for debugging.
2. Put profiles (`*.ovpn`, `*.conf`, subfolders allowed) into
   `%USERPROFILE%\OpenVPN3\config`, or use *Import...*.
3. Start `ovpn3-gui.exe`. Starting it again brings up the connections window;
   `--connect <profile>` and `--import <file>` are forwarded to the running instance.

Without the agent, set the worker to `omicli.exe` in *Settings* and run the GUI
as administrator.

Saved passwords are encrypted with Windows DPAPI for the current user and
kept in `HKCU\Software\OpenVPN3\ovpn3-gui`. A saved password is used to sign
in without a prompt and is removed when the server rejects it.

Worker logs are written to `%LOCALAPPDATA%\OpenVPN3\ovpn3-gui\logs`.

## Limitations

- Only one connection that uses `redirect-gateway` is allowed at a time. The
  check looks at the profile files only, not at options pushed by the server.
- `block-outside-dns` of one connection also blocks DNS of the others.
- TAP and Wintun need one adapter per simultaneous connection; ovpn-dco-win
  creates adapters on demand.
