# SmokeHouse_1.0
 
Witam,

To jest moje pierwsze podejście do sterownika wedzarni elektrycznej.

Ideą było:
- sterowanie za pomocą telefonu, bez konieczności posiadania szpecącego panelu sterowania na wędzarni
- wyeliminowanie Blynka i innych podobnych aplikacji - sterowanie ma być bezpośrednie

Szczerze - jak się zabierałem za pisanie programu to moja wiedza byłą mniej więcej na poziomie wiedzu jak zrobić programik Blink
Teraz wiem już troszkę więcej i planuje rozwijać.

Opis programu:

Program jest stworzony w Platformio - jego plik ini już pobierze wymagane biblioteki. Nie jest raczej trudne przerobić go pod Arduino IDE.


SmokeHouse v1.0

Urządzenie po pierwszej instalacji można już aktualizować poprzez OTA. Nie ma już potrzeby wpinania do USB.
Na głównej stronie po adresie IP dodajemy /update i wtedy uruchamia się OTA i możemy podać pilk programu lub plik filesystem

Opis programu

- linia 37  - const int WiFiMode = 0;   // 0 - Hardcoded STA MODE, 1 - Hardcoded AP Mode, 2 - WifiManager
            0 - Hardcoded STA MODE - w tym trybie podajemy dane routera wifi w liniach 45 i 46 - i urzadzenie bedzie przypisane na stałe do tego routera
            1 - Hardcoded AP Mode - w tym trybie urządzenie będzie pracowało w trybie Access Point pod adresem IP 192.168.4.1 - to jest przydatne gdy nie mamy w zasięgu routera ani żadnej sieci - sterujemy urządzeniem bezpośrednio z telefonu
            2 - WifiManager - przy pierwszym uruchomieniu uruchamia się WIFI manager - podajemy dane naszej sieci i jak pomyślnie sie zaloguje to kontynuuje już pracę z danym routerem aż do momentu jak nie będzie się mógł połączyć to przy uruchomieniu znów się uruchomi manager.
            
- linia 123 - String TempMaxMain = "95"; maksymalna temperatura jaką się da ustawić
- linia 125 - int TempOverheat = 115; temperatura przy któej urządzenie się wyłączy awaryjnie
- linie 150 i 151 - parametry Pid - agg - agresywne i cons - łagodne - urządzenie startuje z agresywnymi parametrami - 5 stopni przed docelową tempereaturą przechodzi w tryb łagodny. - parametry do dostrojenia.


GPIOS

- linia 141 - const byte RelayPin = 27; - pin pod który podłączamy SSR grzałki
- linia 183 - const int oneWireBus = 2; - pin pod DS18B20
- linia 106 - int outputGPIOs[NUM_OUTPUTS] = {18, 19, 12, 14, 40, 41}; - pin 18 - zadymiacz, pin 19 - wentylator termoobiegu, pin 12 - wentylator suszenia, pin 14 - oświetlenie.





