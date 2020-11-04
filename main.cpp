/// Standardbibliotheken
#include <cmath>    // PI
#include <iostream> // Dateneingabe Konsole
#include <fstream>  // std::ifstream
#include <cstdint>  // int-Typen
#include <vector>   // std::vector
#include <memory>   // SmartPointer
#include <string>   // std::string
#include <regex>    // string splitting
#include <iomanip> // Ausrichtung Zahlen Konsole

/// Makros
#define trennung "*******************" // Trennungszeichen für Konsolenausgabe
#define InputFile "in.txt"

/// Globale Variablen
const auto r_E{6378e3};
const auto rad{M_PI / 180.0f}; // Radiant (1 Grad = 180 Grad / pi)

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
    const uint8_t min; // Winkelminuten
    const uint8_t sec; // Winkelsekunden

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
            std::cout << 'N' << std::endl;
        else if (dir == directionEl::S)
            std::cout << 'S' << std::endl;
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
            std::cout << 'W' << std::endl;
        else if (dir == directionAz::O)
            std::cout << 'O' << std::endl;
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
        std::cout << "Breitengrad \u03A6:\t";
        phi.print();
        std::cout << "Längengrad \u03BB:\t";
        lambda.print();
        std::cout << std::endl;
    }
};

// Liefert Winkel als Dezimalzahl
inline float getAngle(const struct AngleAz &angle) noexcept
{
    const auto calc{angle.angle + angle.min / 60.0f + angle.sec / 3600.0f};
    return (angle.dir == directionAz::W) ? -1 * calc : calc;
}

// Liefert Winkel als Dezimalzahl
inline float getAngle(const struct AngleEl &angle) noexcept
{
    const auto calc{angle.angle + angle.min / 60.0f + angle.sec / 3600.0f};
    return (angle.dir == directionEl::S) ? -1 * calc : calc;
}

// Gibt Winkel zwischen zwei Punkten auf der Kugel zurück
float getZentriwinkel(const struct Coordinate &a, const struct Coordinate &b) noexcept
{
    const auto phi_a{getAngle(a.phi)};
    const auto phi_b{getAngle(b.phi)};

    const auto lambda_a{getAngle(a.lambda)};
    const auto lambda_b{getAngle(b.lambda)};

    // Zentriwinkel:
    const auto zeta{
        acosf((sinf(phi_a * rad) * sinf(phi_b * rad) + cosf(phi_a * rad) * cosf(phi_b * rad) + cosf((lambda_b - lambda_a) * rad)))
        /* Trigon. Fkt. in C++ verwenden ihren Parameter in Bogenmaß, deshalb hier noch umrechnen (* rad). Für acosf nicht nötig, da Wert bereits im Bogenmaß! */
    };

    return (zeta / rad); // Winkel in Grad umrechnen
}

// Gibt Kurswinkel zurück: Winkel zwischen Nordrichtung und Südrichtung (im Bogenmaß!), zeta in Grad
float getKurswinkel(const struct Coordinate &a, const struct Coordinate &b, const float zeta)
{
    if (zeta == 0)
        throw std::overflow_error("Division durch Null!"); // Division durch Null abfangen

    const auto phi_a{getAngle(a.phi)};
    const auto phi_b{getAngle(b.phi)};

    const auto angle{
        acosf((sinf(phi_b * rad) - sinf(phi_a * rad) * cosf(zeta)) / (cosf(phi_b * rad) * sinf(zeta * rad)))
        /* Vorsicht: Division durch Null möglich! */
    };

    return (angle / rad);
}

// Gibt Strecke des Winkels zurück
float getStrecke(const float zentriwinkel) noexcept
{
    return (zentriwinkel * rad * r_E);
}

// Gibt den nördlichsten Punkt
struct Coordinate getNoerdlichsterPunkt(const struct Coordinate &a, float kurswinkel)
{
    // Ausgangspunkt:
    const auto phi_a{getAngle(a.phi)};
    // Breitengrad nördlichster Punkt:
    const auto phi_n{acosf(sinf(fabs(kurswinkel) * rad) * cosf(phi_a * rad))};
    // Längengrad nördlichster Punkt:
    const auto lambda_n{getAngle(a.lambda) + std::copysign(1.0, kurswinkel) * fabs(acosf((tanf(phi_a * rad)) / tanf(phi_n * rad)))};

    // Liefert Nachkommastellen einer Gleitkommazahl:
    const auto getFraction = [](double d) -> double {
        double frac{0.0};
        const auto num = std::modf(d, &frac);
        return frac;
    };

    // Mit Nachkommastellen, Minuten und Sekunden der zwei Winkel berechnen:
    const auto phi_frac_min{getFraction(phi_n) * 60.0f};
    const auto phi_frac_sec{getFraction(phi_frac_min) * 60.0f};

    const auto lambda_frac_min{getFraction(lambda_n) * 60.0f};
    const auto lambda_frac_sec{getFraction(lambda_frac_min) * 60.0f};

    return Coordinate(static_cast<int16_t>(phi_n), static_cast<uint8_t>(phi_frac_min), static_cast<uint8_t>(phi_frac_sec), directionEl::N, static_cast<int16_t>(lambda_n), static_cast<uint8_t>(lambda_frac_min), static_cast<uint8_t>(lambda_frac_sec), directionAz::W, "Nördlichster Punkt", 0);
}

Coordinate getKoordinate(const std::vector<Coordinate>& coords, uint8_t i) {
    for(const auto& ele : coords)
        if (ele.no == i)
            return ele;

    throw std::logic_error("Kein zugehöriges Koordinatenobjekt gefunden!");
}


int main(void)
{
    // Einleitung:
    std::cout << trennung << "\n\n\tSPHÄRISCHE TRIGONOMETRIE\n\n\t\t" << trennung << "\n\n"
              << "Daten werden aus '" << InputFile << "' eingelesen...\n" << std::endl;

    std::ifstream file;

    file.open(InputFile);

    if (!file)
    {
        throw std::runtime_error("Input-Datei ist fehlerhaft bzw. existiert nicht.");
        return 1;
    }

    std::vector<Coordinate> coords;

    std::string input;

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

    char number{65 /* A */};
    while (std::getline(file, input))
    {
        if (input.at(0) == '#') // Kommentare (beginnen mit #) überspringen
            continue;

        // Hier Zeile aufsplitten:
        
        const auto n1{input.find(',')};
        const auto n2{input.find(',', n1 + 1)};

        const std::string Breitengrad{input.substr(0, n1)};
        const std::string Laengengrad{input.substr(n1+2, n2 - n1 - 1)};
        const std::string Bezeichnung{input.substr(n2 + 1, input.length())};

        const std::regex split("\\s");

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


    for(auto elem : coords)
        elem.print();

    

    const auto& ref {getKoordinate(coords, 'A')};
    const auto& ref2 {getKoordinate(coords, 'B')};

    const auto zentri {getZentriwinkel(ref, ref2)};

    std::cout << "Entfernung: " << getStrecke(zentri) << std::endl;
}