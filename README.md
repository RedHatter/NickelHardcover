> [!NOTE]
> The canonical location of this repo is on [codeberg.org](https://codeberg.org/StrayRose/NickelHardcover). For the best support, please consider opening issues there.

# NickelHardcover

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://codeberg.org/StrayRose/NickelHardcover/src/branch/main/LICENSE)
[![Latest release](https://codeberg.org/StrayRose/NickelHardcover/badges/release.svg)](https://codeberg.org/StrayRose/NickelHardcover/releases/latest)
[![donate](https://img.shields.io/badge/donate-%E2%9D%A4-F16061)](https://ko-fi.com/strayrose)

[Hardcover.app](https://hardcover.app/) integration for Kobo eReaders.

Thanks to **pgaskin** for the [NickelHook](https://github.com/pgaskin/NickelHook) library and [NickelTC](https://github.com/pgaskin/NickelTC) toolchain, without which this project would not be possible.

**Warning: Firmware version 5.x is not supported yet.**

<img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/syncing.png" alt="Syncing with hardcover tooltip" width="32%"> <img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/review.png" alt="Review dialog" width="32%"> <img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/journal.png" alt="Reading journal dialog" width="32%">
<img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/menu.png?" alt="NickelHardcover menu" width="24%"> <img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/status.png" alt="Reading status menu" width="24%"> <img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/linking.png" alt="Manually linking dialog" width="24%"> <img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/settings.png" alt="Settings dialog" width="24%">

## Features

- Automatically update reading progress on Hardcover
- Automatically add Kobo annotations as Hardcover reading journal entries
- Review and rate books
- View reading journal and add new entries
- Update book status

## FAQ

- **Is my device supported?**  
  All Kobo devices except the Kobo Mini (2012) are supported. Please make sure your device is fully up to date.

- **Do I need to install NickelMenu as well?**  
  No. NickelHardcover does not make use of NickelMenu.

- **Why is it called NickelHardcover then?**  
  The Kobo UI is internally referred to as Nickel on the device. The name is simply referencing the fact that this is a mod to the native UI.

- **Can you add support for X feature?**  
  Maybe. [Open a new issue](https://codeberg.org/StrayRose/NickelHardcover/issues/new) and ask, be sure to provide your use case.

## Installation

1. Download the latest release from the [releases page](https://codeberg.org/StrayRose/NickelHardcover/releases)
2. Copy the _KoboRoot.tgz_ file to the `.kobo` directory on your Kobo
3. Eject/disconnect your Kobo, and it should automatically reboot

## Configuration

To function, NickelHardcover needs to be configured with an authorization token from your Hardcover.app account.

1. Visit [hardcover.app/account/api](https://hardcover.app/account/api) and sign in
2. Copy the contents of the text box starting with "Bearer”  
   ![Authorization token](https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/authorization_token.png)
3. Connect your Kobo and edit the file `.adds/NickelHardcover/config_example.ini`
4. On the second line, labeled "authorization”, remove the starting semicolon and paste the token (copied in step 2) after the equals sign
5. Modify any other settings to suit your preferences
6. Save the file as `config.ini`
7. Eject/disconnect your Kobo

Once the above steps are complete, you should have a file named `config.ini` with contents similar to the following.

```ini
; Your Hardcover.app authorization token copied from https://hardcover.app/account/api.
authorization = Bearer MrDIPsXoT02KrqumxgFsZ.dDxWNFxqotRGJcFvNm68mergYHi2uWSEobGa4S2P5PUMr6Gj8P4SV9hUq4dl2PDJJoggxFuSFf1MQEm8BM1a7mb6IcCokea4x1V9POQqD6bpBH0pJ5nHQVRpKLfqVOzKWpAcLS4OOpZZSlwPJIBisZMaVsJOYe6ujxgzIX5V1sDTWRtq1r4IapP0r9q7oVrq5rZjz9GrGT1rl87nnh91Plncr7u2zLMsfxK1PHGkArbPt1PZ7VL7jiTM2JsrhuV7kHawAxCZHfxONQw4wCVdRssDUhL4JDnljDk2qvMURPfkY4sctYyWturhAKjdwmuvQmfOqoXeT1sxi8qoUDsnQaJ9VexX199cYHcM2wxQn2LOYSa3OtKQ6SKCP4YR1WKKrGn4UBwebnkbZKVmDy8YFLu9h7WwfQgl8QvDT7d4LKXYCRC4ZhHoFlF27VQrLpqaglSQdDInkyUy0Z8MnDh7mxzpKy4O9WQftm2NkqxZEW4z2wmRTeVbdVyo2DdLT82K7

; Whether auto-sync is enabled or disabled by default for each book.
auto_sync_default = false

; When to sync Kobo highlights and notes (bookmarks) with the Hardcover.app journal.
; - always    Sync bookmarks with every sync
; - never     Never sync bookmarks
; - finished  Only sync bookmarks when reading progress reaches 100%
sync_bookmarks = always

; Run auto-sync at the specified hour even if the Kobo is asleep. Possible
; values are 1-24 where 1 is 1am, 22 is 10pm, etc.
; sync_daily = 2

; Whether to run auto-sync after closing a book or the Kobo is put to sleep.
; - always    Run auto-sync every time a book is closed
; - never     Never run auto-sync when a book is closed
; - 1-100     When a book is closed run auto-sync if the difference in read
;             percentage is greater than this number
sync_on_close = always

; Run auto-sync while a book is open when the difference between the last synced
; read percentage and the current read percentage is greater than this number
; (in percentage points). Set to 100 to disable.
threshold = 20
```

## Usage

While a book is open, you should find a new menu on the top right where all the functions can be accessed.

### Linking a Hardcover book

NickelHardcover attempts to determine which book to update on Hardcover using the ISBN of the open book. If that fails, or you would like more control, the "Manually link book” menu option allows you to manually select which book to update. The edition of the active read can be changed on the Hardcover book page under the description.

<img src="https://codeberg.org/StrayRose/NickelHardcover/raw/branch/main/screenshots/edition.png" alt="Changing edition" width="400">

### Syncing

There are a few ways to sync reading progress and highlights/annotations with Hardcover. You can trigger a manual sync at any time using the "Sync now” menu option. Additionally, auto-sync can be enabled for the open book using the "Enable auto-sync" menu option. When auto-sync runs can be configured in the NickelHardcover settings.

## Uninstall

To uninstall NickelHardcover, create a file called `nickelhardcover_uninstall` on the root of your device and manually restart your Kobo.
