
# Sign Detection Arbeitsprotokoll

**Yannick Schössow**, **Ibrahim Al Krad** und **Oliver Greiner-Peter**

In diesem Protokoll sind grob alle Aktivitäten unserer Aufgabengruppe *Verkehrsschilderkennung* niedergeschrieben. 

-------------------

## Woche 1
- Vorlesung bei Florian Pramme
- Einführung in das Projekt

## Woche 2 
- Vorlesung bei Florian Pramme

## Woche 3
- erste [Architekturplanung](https://gitlab.cup.ostfalia.de/ostfaliacup/af24/gruppe2/-/blob/master/docs/Architekturplanung.pdf?ref_type=heads) und Mockups-Designs
- Diskussionen über die Schnittstellen von den einzelnen Komponenten und des Deciders/Planers
- Generierung von Testdaten und Videomaterial zur Weiterverarbeitung

## Woche 4 bis 6
- erste Versuche, maschinelles Lernen mit TensorFlow umzusetzen
- Dafür haben wir zuerst ein simples Kerasmodell trainiert und anschließend versucht, es zu laufen zu bekommen.
- Um das Modell auszuführen, wollten wir TensorFlow Lite verwenden, was jedoch immer noch zu groß war.
- Weswegen wir es auch noch einmal mit TensorFlow Micro für Mikrocontroller probiert haben
- Leider haben wir hier einige schlechte Erfahrungen sammeln müssen und haben TensorFlow aufgegeben.

##  Woche 7 & 8
- Wir wandten uns an YOLOv3 von darknet 
- Hier gab es weniger Installationsschwierigkeiten, da Darknet auf dem Auto vorinstalliert ist.
- Jedoch hatten wir nun das Problem, dass ein neues Modell hermusste, da unser Kerasmodell nicht unterstützt wird.
    - Also entweder neues Modell trainieren oder Keras konvertieren.
- Beides hat nicht wirklich funktioniert, weswegen wir uns an reines OpenCV gewendet haben.

## Woche 9 bis 11
- Zunächst haben wir eine HAAR Cascade Classifier XML-Datei für Erkennung von Stoppschildern generieren lassen.
- In Python haben wir das Modell sofort testen können und das Stoppschild wurde über die Webcam erkannt.
- Nun mussten wir nur noch die C++ Implementation von OpenCV nutzen und unseren Classifier für die Schilderkennung nutzen
- Es gab einige Anfangsschwierigkeiten mit der Konfiguration von OpenCV, da einige weitere Module notwendig waren.
- Wir konnten das Setup jedoch letztendlich umsetzen und können inzwischen Stoppschilder über die Kamera des Autos erkennen.
- Für die Distanzberechnung verwenden wir momentan eine sehr einfache Lösung, wir *schätzen* die Entfernung anhand der Größe der Box eines erkannten Schildes. 

## Woche 12 (aktueller Stand)
- In der C++ Implementation wurde die Möglichkeit hinzugefügt, mehr als nur ein Schild gleichzeitig zu erkennen
- Überarbeitung der Distanzerkennung
- Deployment auf Auto und Testing
