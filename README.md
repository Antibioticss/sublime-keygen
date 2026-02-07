# sublime-keygen

**Cross-platform KEYGEN** for Sublime products, including *Sublime Text* and *Sublime Merge*

Latest Stable and Dev builds for all platforms are supported

**Pre-built binaries** can be downloaded from [Actions](https://github.com/Antibioticss/sublime-keygen/actions) or [Releases](https://github.com/Antibioticss/sublime-keygen/releases)

## Build

`zig` is required to build this project, version `0.15.2` is recommended

```shell
zig build -Doptimize=ReleaseSmall
```

Then built binaries will be available in the `zig-out` folder

## Usage

**Patch the main executable** (`sublime_text` or `sublime_merge`) before using the generated license!

```plain
$ subkg -h
subkg - sublime products keygen v1.0
Usage: subkg <command> [options]
Commands:
  patch <path>
    (no option needed)
  keygen [options]
    -n <name>      name for the license
    -t <type>      can be: bundle (default), text, merge
    -s <seats>     license seats, pass 0 for unlimited seats (default)
    -i <id>        license id (number)
Made with love by Antibiotics <3
```

e.g. (for macOS users)

```shell
subkg patch '/Applications/Sublime Merge.app/Contents/MacOS/sublime_merge'

subkg keygen -n hello -t bundle -i 88888888 -s 1026

# only needed on macOS
codesign -f -s - '/Applications/Sublime Merge.app/Contents/MacOS/sublime_merge'
```

## How does it work

Sublime uses a 1024-bit RSA key to verify the signature of the license, and validates several bytes of the public key SHA-256 hash.

Command `patch` patches the public key and updates the hash bytes in the validation process.

Then command `keygen` generates a license signed with the private key corresponding to the patched public key.

For more details, check the source code `patch.c` and `keygen.c` in [src](https://github.com/Antibioticss/sublime-keygen/tree/main/src)

## Disclaimer

This repository is provided **for educational and research purposes only**.

The content of this project focuses on the study of **cryptographic standards (e.g. PKCS#1)**, software license formats, and cross-platform implementation techniques. It is intended to facilitate learning, academic discussion, and technical research in the fields of cryptography, reverse engineering, and software security.

Any references to license generation, key structures, or “keygen”-related implementations are **purely demonstrational** and **should not be used to bypass, violate, or circumvent software licensing mechanisms** or terms of service of any commercial software.

Sublime and all related trademarks, copyrights, and intellectual property rights are the property of their respective owners.
If you rely on Sublime Text or Sublime Merge in production or long-term use, please **purchase a legitimate license** to support the developers.

The author(s) of this repository assume **no responsibility or liability** for any misuse of the information provided herein.
By using or referencing this repository, you agree that you are solely responsible for ensuring compliance with applicable laws and software license agreements.
