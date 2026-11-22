# Intelligens Mini Szerverszoba Monitorozó és Automatizáló Rendszer

**Készítette:**
* Barna Szilveszter
* Bernáth Dávid
* Nagy Réka Zsuzsa

**Intézmény:** Nyíregyházi Egyetem (2026)

---

## A projekt rövid leírása
A projekt célja egy ESP32 mikrokontrollerre, lokális Mosquitto MQTT brokerre, Node-RED átjáróra és ThingsBoard CE platformra épülő szerverszoba-felügyeleti és automatizálási rendszer megvalósítása. A rendszer helyben (edge szinten) azonnal beavatkozik tűzriasztás vagy túlmelegedés esetén (szervómotoros ajtóvezérlés, RGB LED, buzzer), miközben valós időben továbbítja a telemetriai adatokat és riasztásokat a felügyeleti dashboardra.

## Repozitórium felépítése
* `dokumentacio/` – A projekt részletes, képekkel illusztrált műszaki dokumentációja (`.docx` és `.pdf`)
* `esp32/serverszoba.ino` – Az ESP32 mikrokontroller forráskódja
* `node-red/flow.json` – A Node-RED adatfeldolgozó folyamat exportja
* `rule-chain/serverszoba-automation.json` – A ThingsBoard riasztási és hiszterézis szabálylánc exportja

## Biztonsági megjegyzés
A nyilvános repozitóriumba feltöltött konfigurációs és forráskód fájlokból a Wi-Fi hitelesítő adatok, a helyi hálózati IP-címek és a ThingsBoard eszközazonosító tokenek eltávolításra kerültek.

## Működési bemutató (Videó)

https://github.com/user-attachments/assets/d7948b70-36f7-4b3f-be26-d41c15a1d06b


