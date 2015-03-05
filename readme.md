# goDMD

Firmware for den pinguino micro zum bau einer goDmd Uhr

## bugs
- uhrzeit stimmt nicht (geht nach)
-> check wann wird one sec addiert?
behoben

* Clock kann kein Datum anzeigen / keine Temperatur
- farbwechsel in main loop-
* uhrzeit / datum wechsel im main loop
* evtl muss der doppelpunkt "schmäler" (1 Byte) werden
* dto für den Punkt beim Datum
- kleiner Font für Uhr
-- geladen wird der Font bereits, aber noch nicht genutzt

- helligkeits regelung LED:
an ende des interrupts, müssen die LED aus bleiben für einen Anteil der
Zeit die eigentlich gesetzt ist und zwar jedes zweite mal.



## todos
- Temperatur anzeige
- IR Fernbedienung
- Ein Button Menü
- PIR Aktions.
- kleine Uhr
- Uhr im Hintergrund
- EEPROM mit SDCard simulieren
