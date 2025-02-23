# WLib Blob Management Library

## Einf�hrung


``` cpp
std::byte buffer[7];                                                   +-----+-----+-----+-----+-----+-----+-----+-----+
wlib::blob::MemoryBlob blob(buffer);                       -->     idx:|  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |
                                                                       +-----+-----+-----+-----+-----+-----+-----+-----+
                                                                  data:| XXX | XXX | XXX | XXX | XXX | XXX | XXX |     |
                                                                       +-----+-----+-----+-----+-----+-----+-----+-----+
                                                                          ^                                         ^   
                                                                          begin                                     end
                                                                          ^
                                                                          position

                                                                      +-----+-----+-----+-----+-----+-----+-----+-----+
blob.insert_back(uint16_t(0xAABB), std::endian::big);      -->    idx:|  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |
                                                                      +-----+-----+-----+-----+-----+-----+-----+-----+
                                                                 data:| AAh | BBh | XXX | XXX | XXX | XXX | XXX |     |
                                                                      +-----+-----+-----+-----+-----+-----+-----+-----+
                                                                         ^                                         ^   
                                                                         begin                                     end
                                                                                     ^
                                                                                     position

                                                                     +-----+-----+-----+-----+-----+-----+-----+-----+
blob.insert_front(uint16_t(0xCCDD), std::endian::little);  -->   idx:|  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |
                                                                     +-----+-----+-----+-----+-----+-----+-----+-----+
                                                                data:| DDh | CCh | AAh | BBh | XXX | XXX | XXX |     |
                                                                     +-----+-----+-----+-----+-----+-----+-----+-----+
                                                                        ^                                         ^   
                                                                        begin                                     end
                                                                                                ^
                                                                                                position

                                                                     +-----+-----+-----+-----+-----+-----+-----+-----+
blob.insert(2, uint16_t(0xEEFF), std::endian::big);        -->   idx:|  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |
                                                                     +-----+-----+-----+-----+-----+-----+-----+-----+
                                                                data:| DDh | CCh | EEh | FFh | AAh | BBh | XXX |     |
                                                                     +-----+-----+-----+-----+-----+-----+-----+-----+
                                                                        ^                                         ^   
                                                                        begin                                     end
                                                                                                            ^
                                                                                                            position
```

## �berblick

Die `wlib::blob::MemoryBlob`-Klasse ist Teil der WLib-Bibliothek und dient zur Verwaltung von Speicherblobs. Sie erm�glicht es, Daten in einem Speicherbereich sicher zu manipulieren, indem Funktionen zum Lesen, Schreiben, Einf�gen, L�schen und weitere n�tzliche Operationen bereitgestellt werden.

## Hauptmerkmale

- **Flexible Speicherverwaltung**: Unterst�tzt Operationen wie das �berschreiben, Einf�gen und Entfernen von Daten in einem kontinuierlichen Speicherbereich.
- **Typensicherheit**: Arbeitet mit `std::byte` und unterst�tzt arithmetische Typen durch das `ArithmeticOrByte`-Konzept.
- **Endian-Unterst�tzung**: Erm�glicht Operationen mit Ber�cksichtigung der Byte-Reihenfolge (Endianess).

## Anwendungsf�lle

Die `MemoryBlob`-Klasse kann in folgenden Szenarien n�tzlich sein:
- Manipulation von Rohdaten in Anwendungen wie Netzwerkprotokollen oder Dateisystemen.
- Buffer-Management in eingebetteten Systemen oder bei der Hardware-Interaktion.
- Generische Datenkonvertierungen und -bearbeitungen.

## Schnellstart

### Einbindung der Klasse

Stellen Sie sicher, dass Sie die Bibliothek in Ihr Projekt inkludieren:

```cpp
#include "wlib_blob.hpp"
```

### Objektinitialisierung

Erstellen Sie ein Objekt mit einem vorgegebenen Speicherbereich:

```cpp
std::byte buffer[1024];
wlib::blob::MemoryBlob blobObj(buffer);
```

### Daten schreiben

Schreiben Sie Daten in den Blob:

```cpp
std::byte data[] = {0x01, 0x02, 0x03};
blobObj.try_overwrite(0, data, sizeof(data));
```

### Daten lesen

Lesen Sie Daten aus dem Blob:

```cpp
std::byte output[3];
blobObj.try_read(0, output, sizeof(output));
```



