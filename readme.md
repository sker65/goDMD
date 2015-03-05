# goDMD

Firmware for den pinguino micro zum bau einer goDmd Uhr

# Setup der IDE

Als IDE kann Eclipse IDE genutzt werden. Siehe http://cogito-ergo-blog.de/blog/2015/02/25/eclipse-cdt-zur-pic-pic32-entwicklung-nutzen/

# Setup der Umgebung

Dieses Projekt nutzt einen chipKit core und einige Bibliotheken daraus. Um einen passende Core als libCore_api.a zum Linken zu haben, 
empfielt es sich dafür ebenfalls ein Eclipse CDT Projekt zu machen.
Auf dieses Projekt kann dann verwiesen werden.

## Libraries

Die Libraries die genutzt werden sind direkt aus dem Eclipse Workspace heraus verlinkt und 
zeigen auf die UECIDE Installation im home verzeichnis, wo der chipKit core Quellcode abgelegt ist.
Hier sind ggf. die Pfade in den Eclipse-Links anzupassen.

## IRremote Library
Die IR Remote Library wurde für den _Empfang_ an die PIC Umgebung angepasst und das Recorden
des IR Eingangs im Interrput angepasst.

Was nicht geht ist das Senden von IR Command. Die betreffen Stellen, die die PWM Signale 
erzeugen müssen noch von Arduino nach PIC portiert werden.

# SD Library
Diese Library ist eine projekt lokale Kopie der Lib aus dem ChipKIT core mit kleineren 
Anpassungen, damit sie aus dem pinguino micro ohne Probleme läuft.

Änderungen:
SD:
- seekCur eingefügt.
Sd2Card:
- setTranfersize to 8 bit
- setSpeed to 10000000UL (instead of 20000000UL) faster speed does not work
