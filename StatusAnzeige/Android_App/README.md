# StatusAnzeige BT – Android App

Android-App zur Bluetooth-Steuerung der StatusAnzeige.

## Funktionen

- sucht nach einem BLE-Gerät mit Namen `StatusAnzeige`
- verbindet sich mit dem StatusAnzeige-BLE-Service
- Schnellstatus: FREI, BUSSY, MEETING, PAUSE, SENDE EMAIL, BITTE WARTEN
- liest die Notify-/Status-Characteristic
- kein API-Passcode nötig, da die App direkt BLE verwendet

## BLE-Protokoll

Service: `7b4e1000-8b6f-4d69-9d52-5f6a7a8c1000`  
WRITE: `7b4e1001-8b6f-4d69-9d52-5f6a7a8c1000`  
READ / NOTIFY: `7b4e1002-8b6f-4d69-9d52-5f6a7a8c1000`

Beispiel: `status:frei`

## Build

`gradle :app:assembleDebug`

APK:
`app/build/outputs/apk/debug/app-debug.apk`
