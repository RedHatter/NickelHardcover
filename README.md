# NickelHardcover
Hardcover.app integration for Kobo eReaders [Hardcover.app](https://hardcover.app/).

**Warning: Firmware version 5.x is not supported yet.**

<img src="https://github.com/RedHatter/NickelHardcover/blob/main/screenshots/syncing.png?raw=true" alt="Syncing with hardcover tooltip" width="33%"> <img src="https://github.com/RedHatter/NickelHardcover/blob/main/screenshots/review.png?raw=true" alt="Review dialog" width="33%"> <img src="https://github.com/RedHatter/NickelHardcover/blob/main/screenshots/journal.png?raw=true" alt="Reading journal dialog" width="33%">
<img src="https://github.com/RedHatter/NickelHardcover/blob/main/screenshots/menu.png?raw=true" alt="NickelHardcover menu" width="33%"> <img src="https://github.com/RedHatter/NickelHardcover/blob/main/screenshots/status.png?raw=true" alt="Reading status menu" width="33%"> <img src="https://github.com/RedHatter/NickelHardcover/blob/main/screenshots/linking.png?raw=true" alt="Manually linking dialog" width="33%">

## Features

* Automatically update reading progress on Hardcover
* Automatically add Kobo highlights and annotations as Hardcover reading journal entries
* Review and rate books
* View reading journal and add new entries
* Update book status

## Installation

1. Download the latest release from the [releases page](https://github.com/RedHatter/NickelHardcover/releases)
2. Copy the *KoboRoot.tgz* file to the `.kobo` directory on your Kobo
3. Eject/disconnect your Kobo, and it should automatically reboot

## Configuration

To function, NickelHardcover needs to be configured with an authorization token from your Hardcover.app account.

1. Visit [hardcover.app/account/api](https://hardcover.app/account/api) and sign in
2. Copy the contents of the text box starting with “Bearer”
![Authorization token](https://github.com/RedHatter/NickelHardcover/blob/main/screenshots/authorization_token.png?raw=true)
3. Connect your Kobo and edit the file `.adds/NickelHardcover/config_example.ini`
4. On the second line, labeled “authorization”, remove the starting semicolon and paste the token (copied in step 2) after the equals sign
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

; Whether to run auto-sync after closing a book or the Kobo is put to sleep.
; - always    Run auto-sync every time a book is closed
; - never     Never run auto-sync when a book is closed
; - a number  When a book is closed run auto-sync if the difference in read
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

NickelHardcover attempts to determine which book to update on Hardcover using the ISBN of the open book. If that fails, or you would like more control, the “Manually link book” menu option allows you to manually select which book to update. The edition of the active read can be changed on the Hardcover book page under the description.
![Changing edition](https://github.com/RedHatter/NickelHardcover/blob/main/screenshots/edition.png?raw=true)

### Syncing

There are a few ways to sync reading progress and highlights/annotations with Hardcover. You can trigger a manual sync at any time using the “Sync now” menu option. Additionally, if auto-sync is enabled for the open book NickelHardcover will, one, automatically sync whenever the book is closed, or the Kobo is put to sleep (based on the `sync_on_close` setting), and two, sync whenever the difference in reading percentage is more than the `threshold` setting.

## Uninstall

To uninstall NickelHardcover, simply create a file called `nickelhardcover_uninstall` on the root of your device and manually restart your Kobo.
