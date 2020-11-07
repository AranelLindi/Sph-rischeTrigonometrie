// Winkel werden standardmäßig im Bogenmaß übergeben!

/// Standardbibliotheken
#include <cmath>    // PI
#include <iostream> // Dateneingabe Konsole
#include <fstream>  // std::ifstream
#include <cstdint>  // int-Typen
#include <vector>   // std::vector
#include <memory>   // SmartPointer
#include <string>   // std::string
#include <regex>    // string splitting
#include <iomanip>  // Ausrichtung Zahlen Konsole

/// Makros
#define trennung "*******************" // Trennungszeichen für Konsolenausgabe
#define InputFile "in.txt"

/// Globale Variablen
const auto r_E{6371};              // Erdradius in [km]
constexpr auto rad{M_PI / 180.0f}; // Radiant (1 Grad = 180 Grad / pi)

/// Strukturen
// Richtung Elevation-Winkel (Nord/Süd):
enum class directionEl
{
    N,
    S
};

// Richtung Azimut-Winkel (West/Ost):
enum class directionAz
{
    W,
    O
};

// Winkel-Basisklasse:
struct Angle // nicht instanziieren, sondern abgeleitete Klassen AngleEl/AngleAz verwenden!
{
    const uint16_t angle; // Winkel
    const uint8_t min;    // Winkelminuten
    const uint8_t sec;    // Winkelsekunden

    Angle(uint16_t _angle, uint8_t _min, uint8_t _sec) : angle(_angle), min(_min), sec(_sec) {} // Konstruktor

    void print(void) const noexcept
    {
        std::cout << std::setw(3) << angle << "° " << std::setw(2) << static_cast<uint16_t>(min) << "' " << std::setw(2) << static_cast<uint16_t>(sec) << "'' ";
    }
};

// Elevation-Winkel:
struct AngleEl : Angle
{
    const directionEl dir;

    AngleEl(uint16_t _angle, uint8_t _min, uint8_t _sec, directionEl _dir) : Angle(_angle, _min, _sec), dir(_dir) {}

    void print(void) const noexcept
    {
        Angle::print();
        if (dir == directionEl::N)
            std::cout << 'N';
        else if (dir == directionEl::S)
            std::cout << 'S';
    }
};

// Azimut-Winkel:
struct AngleAz : Angle
{
    const directionAz dir;

    AngleAz(uint16_t _angle, uint8_t _min, uint8_t _sec, directionAz _dir) : Angle(_angle, _min, _sec), dir(_dir) {}

    void print(void) const noexcept
    {
        Angle::print();
        if (dir == directionAz::W)
            std::cout << 'W';
        else if (dir == directionAz::O)
            std::cout << 'O';
    }
};

struct Coordinate
{
    const AngleEl phi;      // Breitengrad
    const AngleAz lambda;   // Längengrad
    const int8_t no;        // fortlaufende Nummer/Buchstabe
    const std::string name; // Bezeichner aus .txt-Datei

    Coordinate(uint16_t phi_angle, uint8_t phi_min, uint8_t phi_sec, directionEl phi_dir, uint16_t lambda_angle, uint8_t lambda_min, uint8_t lambda_sec, directionAz lambda_dir, const std::string &bez, int8_t _no) : phi(phi_angle, phi_min, phi_sec, phi_dir), lambda(lambda_angle, lambda_min, lambda_sec, lambda_dir), name(bez), no(_no)
    {
    }

    void print(void) const noexcept
    {
        std::cout << no << ".) " << name << '\n';
        std::cout << "\t\u03A6: ";
        phi.print();
        std::cout << "\t\u03BB: ";
        lambda.print();
        std::cout << '\n'
                  << std::endl;
    }
};

inline float deg2rad(float angle)
{
    return (angle * M_PI / 180.0f);
}

inline float rad2deg(float angle)
{
    return (angle * 180.0f / M_PI);
}

// Liefert Winkel als Dezimalzahl im Bogenmaß
inline float getAngle(const AngleAz &angle) noexcept
{
    const auto calc{deg2rad(angle.angle + (angle.min / 60.0f) + (angle.sec / 3600.0f))};
    return (angle.dir == directionAz::O) ? (-1 * calc) : calc;
}

// Liefert Winkel als Dezimalzahl im Bogenmaß
inline float getAngle(const AngleEl &angle) noexcept
{
    const auto calc{deg2rad(angle.angle + (angle.min / 60.0f) + (angle.sec / 3600.0f))}; // Winkel in Grad
    return (angle.dir == directionEl::S) ? (-1 * calc) : calc;
}
// Gibt Winkel zwischen zwei Punkten auf der Kugel zurück (Zentriwinkel)
float getCentricAngle(const Coordinate &a, const Coordinate &b) noexcept
{
    const auto phi_a{getAngle(a.phi)};
    const auto phi_b{getAngle(b.phi)};

    const auto lambda_a{getAngle(a.lambda)};
    const auto lambda_b{getAngle(b.lambda)};

    // Zentriwinkel:
    const auto zeta{
        acosf(sinf(phi_a) * sinf(phi_b) + cosf(phi_a) * cosf(phi_b) * cosf(lambda_b - lambda_a))
        /* Trigon. Fkt. in C++ verwenden ihren Parameter in Bogenmaß, deshalb hier noch umrechnen (* rad). Für acosf nicht nötig, da Wert bereits im Bogenmaß! */
    };

    return zeta;
}

// Gibt Kurswinkel zurück: Winkel zwischen Nordrichtung und Südrichtung, zeta im Bogenmaß!
float getHeadingAngle(const struct Coordinate &a, const struct Coordinate &b)
{
    const auto zeta{getCentricAngle(a, b)}; // Zentriwinkel

    if (zeta == 0)
        throw std::overflow_error("Division durch Null!"); // Division durch Null abfangen

    const auto phi_a{getAngle(a.phi)};
    const auto phi_b{getAngle(b.phi)};

    const auto angle{
        acosf((sinf(phi_b) - sinf(phi_a) * cosf(zeta)) / (cosf(phi_a) * sinf(zeta)))
        /* Vorsicht: Division durch Null möglich! */
    };

    return angle;
}

// Gibt Strecke des Winkels zurück (Orthodrom!)
float getRouteLength(float zentriwinkel) noexcept
{
    return (zentriwinkel * r_E);
}

// Gibt den nördlichsten Punkt auf einem Großkreis zurück
Coordinate getNorthernmostPoint(const Coordinate &a, float kurswinkel)
{
    // Ausgangspunkt:
    const auto phi_a{getAngle(a.phi)};
    const auto lambda_a{getAngle(a.lambda)};
    // Breitengrad nördlichster Punkt:
    const auto phi_n{acosf(sinf(kurswinkel) * cosf(phi_a))};
    // Längengrad nördlichster Punkt:
    const auto lambda_n{asinf(cosf(kurswinkel) / sinf(phi_n)) + lambda_a};

    // Liefert Nachkommastellen einer Gleitkommazahl:
    const auto getFraction = [](double d) -> float {
        double frac{0.0};
        const auto num = std::modf(d, &frac);
        return num;
    };

    const directionEl dEl{(phi_n < 0) ? directionEl::S : directionEl::N};
    const directionAz dAz{(lambda_n < 0) ? directionAz::O : directionAz::W};

    // Mit Nachkommastellen, Minuten und Sekunden der zwei Winkel berechnen:
    const auto phi_frac_min{getFraction(rad2deg(fabs(phi_n))) * 60.0f};
    const auto phi_frac_sec{getFraction(phi_frac_min) * 60.0f};

    const auto lambda_frac_min{getFraction(rad2deg(fabs(lambda_n))) * 60.0f};
    const auto lambda_frac_sec{getFraction(lambda_frac_min) * 60.0f};

    return Coordinate(static_cast<uint16_t>(rad2deg(fabs(phi_n))), static_cast<uint8_t>(phi_frac_min), static_cast<uint8_t>(phi_frac_sec), dEl, static_cast<uint8_t>(rad2deg(fabs(lambda_n))), static_cast<uint8_t>(lambda_frac_min), static_cast<uint8_t>(lambda_frac_sec), dAz, "Nördlichster Punkt", '#');
}

// Gibt eine Koordinate aus der Sammlung zurück, die das entsprechende Index trägt:
Coordinate getCoordinate(const std::vector<Coordinate> &coords, uint8_t i)
{
    for (const auto &ele : coords)
        if (ele.no == i)
            return ele;

    // Am Ende der Schleife wurde kein passendes Element gefunden, dann eine Exception werfen:
    throw std::logic_error("Kein zugehöriges Koordinatenobjekt gefunden!");
}

void printCentricAngle(const Coordinate &a, const Coordinate &b) noexcept // Zentriwinkel
{
    std::cout << "Zentriwinkel zwischen " << a.name << " und " << b.name << " beträgt:\t" << std::setprecision(4) << rad2deg(getCentricAngle(a, b)) << " Grad" << std::endl;
}

// Berechnet den Scheitelpunkt auf einem Großkreis:
void printNorthernmostPoint(const Coordinate &a, const Coordinate &b) noexcept
{
    std::cout << "Nördlichster Punkt auf Großkreis von " << a.name << " nach " << b.name << ":\n";
    getNorthernmostPoint(a, getHeadingAngle(a, b)).print();

    const auto abflugswinkel{rad2deg(getHeadingAngle(a, b))};
    const auto anflugswinkel{rad2deg(getHeadingAngle(b, a))};

    const auto fallunterscheidung = [abflugswinkel, anflugswinkel, a, b]() -> void {
        std::cout << "Lage des Scheitelpunkts: ";
        if (((abflugswinkel > 0.0f) & (abflugswinkel < 90.0f)) & ((anflugswinkel > 0.0f) & (anflugswinkel < 90.0f)))
            std::cout << "innerhalb des Bogens " << a.no << b.no << std::endl;
        else if (((abflugswinkel > 90.0f) & (abflugswinkel < 180.0f)) & ((anflugswinkel > 0.0f) & (anflugswinkel < 90.0f)))
            std::cout << "vor " << a.no << std::endl;
        else if (((abflugswinkel > 0.0f) & (abflugswinkel < 90.0f)) & ((anflugswinkel > 90.0f) & (anflugswinkel < 180.0f)))
            std::cout << "hinter " << b.no << std::endl;
    };

    // Ausgeben, wo sich der Scheitelpunkt grob befindet:
    fallunterscheidung();

    std::cout << std::endl;
}

void printHeadingAngle(const Coordinate &a, const Coordinate &b) noexcept // Kurswinkel
{
    std::cout << "Kurswinkel auf Großkreis von " << a.name << " nach " << b.name << ":\t" << std::setprecision(4) << rad2deg(getHeadingAngle(a, b)) << " Grad" << std::endl;
}

void printRouteLength(const Coordinate &a, const Coordinate &b) noexcept
{
    std::cout << "Strecke von " << a.name << " nach " << b.name << ":\t" << std::setprecision(5) << getRouteLength(getCentricAngle(a, b)) << " km" << std::endl;
}

void printPosition(const Coordinate &a, const Coordinate &b, int i, bool interval = true)
{
}

int main(void)
{
    // Einleitung:
    std::cout << trennung << "\n\n\tSPHÄRISCHE TRIGONOMETRIE\n\n\t\t" << trennung << "\n\n"
              << "  Daten werden aus '" << InputFile << "' eingelesen...\n"
              << std::endl;

    std::ifstream file;

    file.open(InputFile);

    if (!file)
    {
        throw std::runtime_error("Input-Datei ist fehlerhaft bzw. existiert nicht.");
        return 1;
    }

    std::vector<Coordinate> coords; // Enthält später alle Koordinaten, die eingelesen wurden

    std::string input; // temporer String, der jeweils eine Zeile der Input.txt-Datei enthält

    // Extra Scope um deklarierte Variablen schnell wieder zu zerstören:
    // Schreibt in die Konsole:
    const auto write = [](std::string str) -> void {
        std::cout << str;
    };

    // Liest aus Konsole in Variable ein (Referenz!):
    const auto read = [](auto &var) -> void {
        std::cin >> var;
    };

    // Prüft, ob Wert innerhalb eines Intervals liegt:
    const auto cond = [](const auto min, const auto max, const auto value) noexcept -> bool {
        if (value >= min && value <= max)
            return true;
        else
            return false;
    };

    // Wandelt einen char in entsprechende Richtung um (Azimut):
    const auto retDirAz = [](const char *c) -> directionAz {
        if (*c == 'W')
            return directionAz::W;
        else if (*c == 'O')
            return directionAz::O;
        else
            throw std::logic_error("Ungültige Richtung (Azimut)!");
    };

    // Wandelt einen char in entsprechende Richtung um (Elevation):
    const auto retDirEl = [](const char *c) -> directionEl {
        if (*c == 'N')
            return directionEl::N;
        else if (*c == 'S')
            return directionEl::S;
        else
            throw std::logic_error("Ungültige Richtung (Elevation)!");
    };

    const std::regex split("\\s"); // wird mehrmals zum aufsplitten von Eingaben verwendet

    char number{65 /* A */}; // Koordinaten "durchnummerieren" und mit 'A' beginnen (wird später für Zuweisung verwendet)

    uint8_t counter{1}; // Zähler für Zeilen

    while (std::getline(file, input))
    {
        try
        {
            if (input.at(0) == '#') // Kommentare (beginnen mit #) überspringen, nur am Zeilenanfang möglich!
                continue;

            // Hier Zeile aufsplitten:
            const auto n1{input.find(',')};
            const auto n2{input.find(',', n1 + 1)};

            const std::string Breitengrad{input.substr(0, n1)};
            const std::string Laengengrad{input.substr(n1 + 2, n2 - n1 - 1)};
            const std::string Bezeichnung{input.substr(n2 + 2, input.length())};

            // Breitengrad untersuchen:
            const std::vector<std::string> resultB{
                std::sregex_token_iterator(Breitengrad.begin(), Breitengrad.end(), split, -1), {}};

            // Längengrad untersuchen:
            const std::vector<std::string> resultL{
                std::sregex_token_iterator(Laengengrad.begin(), Laengengrad.end(), split, -1), {}};

            coords.push_back(Coordinate{static_cast<uint16_t>(std::atoi(resultB[0].c_str())),
                                        static_cast<uint8_t>(std::atoi(resultB[1].c_str())),
                                        static_cast<uint8_t>(std::atoi(resultB[2].c_str())),
                                        retDirEl(resultB[3].c_str()),
                                        static_cast<uint16_t>(std::atoi(resultL[0].c_str())),
                                        static_cast<uint8_t>(std::atoi(resultL[1].c_str())),
                                        static_cast<uint8_t>(std::atoi(resultL[2].c_str())),
                                        retDirAz(resultL[3].c_str()),
                                        Bezeichnung,
                                        number++});
        }
        catch (const std::exception &ex)
        {
            std::cerr << "Einlesefehler in Zeile " << static_cast<uint32_t>(counter) << '!' << std::endl;
            return 1;
        }
        counter++;
    }

    // Alle eingelesenen Koordinaten anzeigen:
    for (auto elem : coords)
        elem.print();

    write("\nBitte Funktionscode mit Parametern eingeben:\n");
    write("********************************************\n");

    // Mögliche Operationen posten
    write("1: Zentriwinkel berechnen:\t[BezKoord] [BezKoord]\n");
    write("2: Kurswinkel berechnen:\t[BezKoord] [BezKoord]\n");
    write("3: Scheitelpunkt berechnen:\t[BezKoord] [BezKoord]\n");
    write("4: Streckenlänge berechnen:\t[BezKoord] [BezKoord]\n");
    write("5: Zwischenpunkt berechnen:\t[BezKoord] [BezKoord] [Geschw_Double] [Treibstoff_Double] [Interval_Bool 1/0]\n");
    write("\n");
    write("0 == exit\n\n");

    std::string cinput; // Enthält Benutzereingabe auf Konsole

    do
    {

        // Benutzereingabe einlesen:
        std::getline(std::cin, cinput);

        // aufsplitten und in vector einlesen:
        const std::vector<std::string> userEingabe{
            std::sregex_token_iterator(cinput.begin(), cinput.end(), split, -1), {}};

        try
        {
            const auto cmd{std::atoi(userEingabe[0].c_str())};

            if (cmd == 0)
                break;

            const auto param1{getCoordinate(coords, static_cast<uint8_t>(userEingabe[1].c_str()[0]))};
            const auto param2{getCoordinate(coords, static_cast<uint8_t>(userEingabe[2].c_str()[0]))};

            switch (cmd)
            {
            case 1:
                printCentricAngle(param1, param2);
                break;
            case 2:
                printHeadingAngle(param1, param2);
                break;
            case 3:
                printNorthernmostPoint(param1, param2);
                break;
            case 4:
                printRouteLength(param1, param2);
                break;
            case 5:
                const auto param3{std::atof(userEingabe[3].c_str())}; // Geschwindigkeit in km/h
                const auto param4{std::atof(userEingabe[4].c_str())}; // Treibstoff in t
                const auto param5{std::atoi(userEingabe[5].c_str())}; // Punkt im Intervall von a und b oder außerhalb des Intervalls

                const auto distance{getRouteLength(getCentricAngle(param1, param2))};

                break;
            }
        }
        catch (const std::exception &ex)
        {
            std::cerr << "Ungültige Parameter eingegeben!" << std::endl;
            continue;
        }
    } while (1);

    //const auto &ref{getCoordinate(coords, 'A')};
    //const auto &ref2{getCoordinate(coords, 'B')};

    //const auto zentri{getCentricAngle(ref, ref2)};

    //std::cout << "Zentriwinkel: " << zentri << std::endl;
    //std::cout << "Entfernung: " << getRouteLength(zentri) << std::endl;
    //std::cout << "Kurswinkel: " << getKurswinkel(ref, ref2, zentri) << std::endl;

    // Mögliche Berechnungen:
    // 1.) Entfernung zwischen zwei Koordinaten
    // 1.1.) Zentriwinkel berechnen getCentricAngle()
    // 1.2.) Strecke berechnen getRouteLength()
    //
    // 2.) Nördlichster Punkt auf einem Großkreis berechnen
    // 2.1.) Zentriwinkel berechnen getCentricAngle()
    // 2.2.) Kurswinkel berechnen getHeadingAngle()
    // 2.3.) Nördlichster Punkt berechnen getNorthernmostPoint()
    //
    //
}