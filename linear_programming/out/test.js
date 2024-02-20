var _a, _b, _c;
var lpsolver = require("javascript-lp-solver/src/solver");
var obj = {};
obj.data = [{ "date": { "Uhrzeit": 0, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 11.19, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "19,64", "Verb_Std_max_year": 19.64, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 1, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 10.6, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "19,04", "Verb_Std_max_year": 19.04, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 2, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 10.3, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "17,22", "Verb_Std_max_year": 19.01, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 3, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 10.32, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "18,19", "Verb_Std_max_year": 19.3, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 4, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 10.32, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "20,83", "Verb_Std_max_year": 20.83, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 5, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 10.09, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "23,29", "Verb_Std_max_year": 23.75, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 6, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 10.11, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "31,64", "Verb_Std_max_year": 40.53, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 7, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 10, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "45,06", "Verb_Std_max_year": 45.71, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 8, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 9.73, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "1,238", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "48,03", "Verb_Std_max_year": 48.03, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 9, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 9.46, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "5,274", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "45,88", "Verb_Std_max_year": 45.88, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 10, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 8.79, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "7,907", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "42,74", "Verb_Std_max_year": 42.74, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 11, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 8.8, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "13,271", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "37,15", "Verb_Std_max_year": 37.15, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 12, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 8.46, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "18,477", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "36,87", "Verb_Std_max_year": 36.87, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 13, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 6.65, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "18,232", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "44,64", "Verb_Std_max_year": 44.64, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 14, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 5.67, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "24,218", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "40,13", "Verb_Std_max_year": 40.13, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 15, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 6.22, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "15,805", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "38,60", "Verb_Std_max_year": 38.6, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 16, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 8, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "8,79", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "33,06", "Verb_Std_max_year": 33.06, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 17, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 10.2, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "34,54", "Verb_Std_max_year": 34.54, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 18, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 11.55, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "34,64", "Verb_Std_max_year": 34.64, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 19, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 12.58, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "26,25", "Verb_Std_max_year": 26.25, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 20, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 13, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "21,50", "Verb_Std_max_year": 21.5, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 21, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 12.87, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "18,32", "Verb_Std_max_year": 20.68, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 22, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 12.79, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "18,50", "Verb_Std_max_year": 18.86, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }, { "date": { "Uhrzeit": 23, "Wochentag": "Dienstag", "Monat": "Januar" }, "Strompreis": { "Preis": 12.2, "Preis_min": 5.67, "Preis_max": 13 }, "Photovoltaik": { "Prognose": "0", "PV_max": 63.36 }, "Ladezustand": [], "Prognose": { "Beschreibung1": "Prognose_Verb_X ergibt sich aus historischen Daten(Mittelwert)der Firma Schulz Systemtechnik GmbH, Wallenhorst", "Beschreibung2": "Prognose_Verb_Std: in Abhängigkeit vom Monat und Uhrzeit Durschnittsverbrauch im Monat um eine bestimmte Uhrzeit", "Verb_Std": "19,71", "Verb_Std_max_year": 19.71, "Verb_Std_max_month": 48.03, "Beschreibung3": "Prognose_Verb_Wo:  in Abhängigkeit vom Wochentag(Mo-Fr)  Durschnittsverbrauch im Monat am Montag z.B", "Verb_Wo": "38,66", "Verb_Wo_max_year": 40.29, "Verb_Wo_max_week": 39.43, "Beschreibung4": "Prognose_Verb_Mo:  in Abhängigkeit vom Monat(Jan-Dez)    Durschnittsverbrauch im Monat allgemein", "Verb_Mo": 36.4, "Verb_Mo_max_MW": 36.4, "Verb_Mo_max_Gesamtverbrauch": 23537.05 } }];
var hours = 24;
var e_v = obj.data.E_V;
var e_pv = obj.data.E_PV;
var e_bn_value = obj.data.E_BN;
var e_l_max = obj.data.E_L_max;
var e_e_max = obj.data.E_E_max;
var q_normal = obj.data.Q_normal;
var q_nominal = obj.data.Q_normal;
var q_alt = obj.data.Q_Total;
var soc_start = obj.data.SoC_start;
var soc_final = obj.data.SoC_final;
var soc_min = obj.data.SoC_min;
var soc_max = obj.data.SoC_max;
var preis = obj.data.Strompreis;
var constraints = {};
var variables = {};
var soc = [];
//Variables// E_BE
for (var i = 1; i <= hours; i++) {
    variables["E_BE_".concat(i)] = (_a = {}, _a["E_B_".concat(i)] = -1, _a);
}
// E_BL
for (var i = 1; i <= hours; i++) {
    variables["E_BL_".concat(i)] = (_b = {}, _b["E_B_".concat(i)] = 1, _b);
}
//EB im E_Netz
for (var i = 1; i <= hours; i++) {
    variables["E_B_".concat(i)] = (_c = {}, _c["E_Netz_".concat(i)] = 1, _c);
}
// Energie aus der PV-Anlage in Stunde i
for (var i = 1; i <= hours; i++) {
    variables["E_PV_".concat(i)] = e_pv[i - 1];
}
// Verbrauch in Stunde i
for (var i = 1; i <= hours; i++) {
    variables["E_V_".concat(i)] = e_v[i - 1];
}
//Energie vom Netz in Stunde i
for (var i = 1; i <= hours; i++) {
    variables["E_Netz_".concat(i)] = { Kosten: preis[i - 1] };
}
//Constraints// Definition Ihres linearen Programmierungsmodells
//E_BE Begrenzung
for (var i = 1; i <= hours; i++) {
    constraints["E_BE_".concat(i)] = { max: e_e_max };
}
//E_BL Begrenzung
for (var i = 1; i <= hours; i++) {
    constraints["E_BL_".concat(i)] = { max: e_l_max };
}
//E_PV Begrenzung
for (var i = 1; i <= hours; i++) {
    constraints["E_PV_".concat(i)] = { min: 0 };
}
//E_V Begrenzung
for (var i = 1; i <= hours; i++) {
    constraints["E_V_".concat(i)] = { min: 0 };
}
// E_B Begrenzung
for (var i = 1; i <= hours - 1; i++) {
    constraints["E_B_".concat(i)] = { min: -e_e_max, max: e_l_max };
}
for (var i = 1; i <= hours; i++) {
    constraints["E_Netz_".concat(i, " + E_PV_").concat(i, " - E_B_").concat(i, " - E_V_").concat(i)] = { equal: 0, };
}
var problem = { optimize: "Kosten", opType: "min", constraints: constraints, variables: variables };
// Lösung des linearen Programmierungsproblems
var solution = lpsolver.Solve(problem);
var msg = {};
msg.solution = solution;
msg.constraints = problem.constraints;
msg.variables = problem.variables;
msg.soc = soc;
if (solution.feasible) {
    console.log("Optimale Lösung gefunden:");
    console.log("Gesamtkosten:", solution.result);
    // Zugriff auf die Werte der einzelnen Variablen    
    for (var i = 0; i < hours; i++) {
        var E_Netz_value = solution["E_Netz_".concat(i)];
        var E_BE_value = solution["E_BE_".concat(i)];
        var E_BL_value = solution["E_BL_".concat(i)];
        var Kosten = solution["Kosten".concat(i)];
        console.log("Stunde ".concat(i + 1, ":"));
        console.log("E_Netz:", E_Netz_value);
        console.log("E_BE:", E_BE_value);
        console.log("E_BL:", E_BL_value);
        console.log("Kosten:", Kosten);
        console.log("--------------");
    }
}
else {
    console.log("Das Modell hat keine zulässige Lösung gefunden.");
}
//# sourceMappingURL=test.js.map