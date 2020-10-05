# callaudiod - Call audio routing daemon
`callaudiod` is a daemon for dealing with audio routing during phone calls.

It provides a D-Bus interface allowing other programs to:
  * switch audio profiles
  * output audio to the speaker or back to its original port
  * mute the microphone

## Dependencies

`callaudiod` requires the following development libraries:
- libasound2-dev
- libglib2.0-dev
- libpulse-dev

## Building

`callaudiod` uses meson as its build system. Building and installing
`callaudiod` is as simple as running the following commands:

```
$ meson ../callaudiod-build
$ ninja -C ../callaudiod-build
# ninja -C ../callaudiod-build install
```

## Running

`callaudiod` is usually run as a systemd user service, but can also be manually
started from the command-line:

```
$ callaudiod
```

## License

`callaudiod` is licensed under the GPLv3+.
