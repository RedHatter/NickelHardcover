# NickelHardcover
Sync reading progress from Kobo eReaders to [Hardcover.app](https://hardcover.app/).

## Installation

1. Download the latest release from the [releases page](https://github.com/RedHatter/NickelHardcover/releases)
2. Copy the the KoboRoot.tgz to the `.kobo` directory on your Kobo
3. Eject/disconnect your Kobo, and it should automatically reboot

## Configuration
In order to function NickelHardcover needs to be configured with an authorization token from your Hardcover.app account.

1. Visit [hardcover.app/account/api](https://hardcover.app/account/api) and signin
2. Copy the contents of the text box starting with "Bearer"
3. Connect your Kobo and edit the file `.adds/NickelHardcover/config.ini`
4. On the first line, labeled "authorization", remove the starting semicolon and paste the token copied in step 2 after the the equals sign
5. Save the file and eject/disconnect your Kobo

Once editted the configure file should look similar to the following.
```
authorization = Bearer MrDIPsXoT02KrqumxgFsZ.dDxWNFxqotRGJcFvNm68mergYHi2uWSEobGa4S2P5PUMr6Gj8P4SV9hUq4dl2PDJJoggxFuSFf1MQEm8BM1a7mb6IcCokea4x1V9POQqD6bpBH0pJ5nHQVRpKLfqVOzKWpAcLS4OOpZZSlwPJIBisZMaVsJOYe6ujxgzIX5V1sDTWRtq1r4IapP0r9q7oVrq5rZjz9GrGT1rl87nnh91Plncr7u2zLMsfxK1PHGkArbPt1PZ7VL7jiTM2JsrhuV7kHawAxCZHfxONQw4wCVdRssDUhL4JDnljDk2qvMURPfkY4sctYyWturhAKjdwmuvQmfOqoXeT1sxi8qoUDsnQaJ9VexX199cYHcM2wxQn2LOYSa3OtKQ6SKCP4YR1WKKrGn4UBwebnkbZKVmDy8YFLu9h7WwfQgl8QvDT7d4LKXYCRC4ZhHoFlF27VQrLpqaglSQdDInkyUy0Z8MnDh7mxzpKy4O9WQftm2NkqxZEW4z2wmRTeVbdVyo2DdLT82K7
sqlite_path = /mnt/onboard/.kobo/KoboReader.sqlite
frequency = 15
```

## Usage
You should find 3 new menu items in the overflow menu of open books.
* Sync now  
  Triggers an update to the reading progress on Hardcover.app
* Enable auto-sync  
  Enables automatically updating the reading progress every 15 minutes (configurable)
* Manually link book  
  NickelHardcover attempts to determine what book to update on Hardcover.app using the ISBN of the open book. If that fails this option allows you to manually select which book to update.
