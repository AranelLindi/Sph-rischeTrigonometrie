/// Standardbibliotheken
#include <cmath>    // PI
#include <iostream> // Dateneingabe Konsole
#include <cstdint>  // int-Typen
#include <array>    // std::array

/// Makros
#define ANZAHL 3                       // Anzahl Koordinatenpaare, die über Konsole eingegeben werden müssen
#define trennung "*******************" // Trennungszeichen für Konsolenausgabe

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
    const uint16_t angle;
    const uint8_t min;
    const uint8_t sec;

    Angle(uint16_t _angle, uint8_t _min, uint8_t _sec) : angle(_angle), min(_min), sec(_sec) {}

    void print(void) const
    {
        std::cout << "Winkel:\t" << angle << "° " << min << "' " << sec << "''";
    }
};

// Elevation-Winkel:
struct AngleEl : Angle
{
    const directionEl dir;

    AngleEl(uint16_t _angle, uint8_t _min, uint8_t _sec, directionEl _dir) : Angle(_angle, _min, _sec), dir(_dir) {}

    void print(void) const
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

    void print(void) const
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
    const AngleAz phi;    // Breitengrad
    const AngleEl lambda; // Längengrad

    Coordinate(uint16_t phi_angle, uint8_t phi_min, uint8_t phi_sec, directionAz phi_dir, uint16_t lambda_angle, uint8_t lambda_min, uint8_t lambda_sec, directionEl lambda_dir) : phi(phi_angle, phi_min, phi_sec, phi_dir), lambda(lambda_angle, lambda_min, lambda_sec, lambda_dir)
    {
    }

    void print(void) const
    {
        std::cout << "Breitengrad:\n";
        phi.print();
        std::cout << '\n'
                  << "Längengrad:\n";
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
        return 1; // Division durch Null abfangen

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

    return Coordinate(static_cast<int16_t>(phi_n), static_cast<uint8_t>(phi_frac_min), static_cast<uint8_t>(phi_frac_sec), directionAz::W, static_cast<int16_t>(lambda_n), static_cast<uint8_t>(lambda_frac_min), static_cast<uint8_t>(lambda_frac_sec), directionEl::N);
}

int main(void)
{
    // Einleitung:
    std::cout << trennung << "\n\n\tSPHÄRISCHE TRIGONOMETRIE\n\n\t\t" << trennung << "\n\n";
    // Koordinatenpaare eingeben:
    std::cout << "Bitte " << ANZAHL << " Koordinatenpaare eingeben:\n";

    std::array<Coordinate, ANZAHL> arr();

    // Extra Scope um deklarierte Variablen schnell wieder zu zerstören:
    {
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
        const auto retDirAz = [](const char c) -> directionAz {
            if (c == 'W')
                return directionAz::W;
            else if (c == 'O')
                return directionAz::O;
            else
                throw std::logic_error("ERROR!");
        };

        // Wandelt einen char in entsprechende Richtung um (Elevation):
        const auto retDirEl = [](const char c) -> directionEl {
            if (c == 'N')
                return directionEl::N;
            else if (c == 'S')
                return directionEl::S;
            else 
                throw std::logic_error("ERROR!");
        };

        // Temporäre Container für Einleseprozess aus Konsole:
        std::array<int16_t, ANZAHL * 2> _angle;
        std::array<uint8_t, ANZAHL * 2> _min;
        std::array<uint8_t, ANZAHL * 2> _sec;
        std::array<directionAz, ANZAHL * 2> _dirAz;
        std::array<directionEl, ANZAHL * 2> _dirEl;

        char c;

        // Jedes Koordinatentupel enthält Längen- und Breitengrad:
        for (size_t i{0}; i < ANZAHL; i++)
            for (size_t f{0}; f < 2; f++)
            {
                if (f == 0)
                { // Längengrad
                    write("Längengrad:\n");
                    write("\tWinkel: ");
                    read(_angle[i + f]);
                    write("\r\tMinuten: ");
                    read(_min[i + f]);
                    write("\r\tSekunden: ");
                    read(_sec[i + f]);
                    write("\r\tRichtung: ");
                    read(c);
                    _dirAz[i + f] = retDirAz(c);
                }
                else
                {
                    write("Breitengrad:\n");
                    write("\tWinkel: ");
                    read(_angle[i + f]);
                    write("\r\tMinuten: ");
                    read(_min[i + f]);
                    write("\r\tSekunden: ");
                    read(_sec[i + f]);
                    write("\r\tRichtung: ");
                    read(c);
                    _dirEl[i + f] = retDirEl(c);
                }
            }
    }
}