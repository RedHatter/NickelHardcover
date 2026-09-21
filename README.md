> [!NOTE]
> The canonical location of this repo is on [codeberg.org](https://codeberg.org/StrayRose/NickelHardcover). For the best support, please consider opening issues there.

# NickelHardcover

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/LICENSE)
[![Latest release](https://codeberg.org/StrayRose/NickelHardcover/badges/release.svg)](https://codeberg.org/StrayRose/NickelHardcover/releases/latest)
[![donate](https://img.shields.io/badge/donate-%E2%9D%A4-F16061)](https://ko-fi.com/strayrose)

[Hardcover.app](https://hardcover.app/) integration for Kobo eReaders.

This project wouldn't be possible without **pgaskin**'s [NickelHook](https://github.com/pgaskin/NickelHook) library and [NickelTC](https://github.com/pgaskin/NickelTC) toolchain.

**Warning: Firmware version 5.x is not supported yet.**

<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/syncing.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/syncing.png" alt="Syncing with hardcover" width="24.5%"></a>
<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/review.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/review.png" alt="Reviewing book" width="24.5%"></a>
<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/journal.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/journal.png" alt="Reading journal" width="24.5%"></a>
<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/new-journal.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/new-journal.png" alt="New reading journal note" width="24.5%"></a>
<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/menu.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/menu.png" alt="NickelHardcover menu" width="19.6%"></a>
<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/status.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/status.png" alt="Reading status menu" width="19.6%"></a>
<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/linking.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/linking.png" alt="Manually linking book" width="19.6%"></a>
<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/linking-edition.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/linking-edition.png" alt="Manually linking edition" width="19.6%"></a>
<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/settings.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/settings.png" alt="Settings" width="19.6%"></a>

## Features

- Automatically update reading progress on Hardcover.app
- Automatically sync Kobo annotations (highlights and notes) to your Hardcover.app journal
- Review and rate books
- View your Hardcover.app journal and add new entries
- Update book status

## FAQ

- **Is my device supported?**  
  All Kobo devices except the Kobo Mini (2012) are supported, as long as your firmware is up to date.

- **Do I need to install NickelMenu as well?**  
  No. NickelHardcover doesn't rely on NickelMenu.

- **Why is it called NickelHardcover then?**  
  Kobo internally calls its UI Nickel. The name simply references the fact that this is a mod to the native UI.

- **Can you add support for X feature?**  
  Maybe. [Open a new issue](https://codeberg.org/StrayRose/NickelHardcover/issues/new) and ask. Be sure to provide your use case.

## Installing or updating

1. Download the [KoboRoot.tgz](https://codeberg.org/StrayRose/NickelHardcover/releases/download/latest/KoboRoot.tgz) file from the [latest release](https://codeberg.org/StrayRose/NickelHardcover/releases/latest)
2. Copy the _KoboRoot.tgz_ file to the `.kobo` directory on your Kobo
3. Eject/disconnect your Kobo, and it should automatically reboot

No need to uninstall before updating. Your configuration is retained.

## Usage

While a book is open, you'll find a new menu in the top-right corner that gives access to all of NickelHardcover's features.

### Signing in

When you first open the menu, you'll see a "Sign in to Hardcover.app" menu item.

<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/signin.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/signin.png" alt="Sign in to Hardcover.app menu item" width="24.5%"></a>

Tapping it brings up a dialog with a QR code.

<a href="https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/screenshots/scan-qrcode.png" target="_blank"><img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/scan-qrcode.png" alt="Sign in dialog" width="24.5%"></a>

Scan the QR code, or visit the link and enter the code, then tap continue. That's it, you're ready to use NickelHardcover!

### Linking a Hardcover book

NickelHardcover attempts to determine which book to update on Hardcover.app using the ISBN of the open book. If that fails, or you want more control, use the "Manually link book" menu option to select the book or edition yourself.

### Syncing

There are a few ways to sync reading progress and annotations with Hardcover.app:
- Manually — trigger a sync at any time with the "Sync now" menu option
- Automatically — enable auto-sync for the open book with the "Enable auto-sync" menu option

You can configure when auto-sync runs (on a schedule, when closing a book, or by read percentage) in the NickelHardcover settings.

## Uninstall

To uninstall NickelHardcover, delete the `.adds/NickelHardcover` directory and manually restart your Kobo.
