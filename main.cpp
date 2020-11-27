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
const auto r_E{6371.0f};           // Erdradius in [km]
constexpr auto rad{M_PI / 180.0f}; // Radiant (1 Grad = 180 Grad / pi)

/// Strukturen
// Richtung Elevation-Winkel (Nord/Süd):
enum class directionEl // Definition: Nord: 90 Grad bis 0 Grad, Süd: 0 Grad bis -90 Grad
{
    N,
    S
};

// Richtung Azimut-Winkel (West/Ost):
enum class directionAz // Definition: West: -180 Grad bis 0 Grad; Ost: 0 Grad bis 180 Grad
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
    Angle(double _angle) : angle(static_cast<uint16_t>(round(_angle))), min(static_cast<uint8_t>(static_cast<int16_t>((_angle - angle) * 60))), sec(static_cast<uint8_t>(static_cast<int16_t>((((_angle - angle) * 60) - min) * 60))) {}

    void print(void) const noexcept
    {
        std::cout << std::setw(3) << angle << "° " << std::setw(2) << static_cast<uint16_t>(min) << "' " << std::setw(2) << static_cast<uint16_t>(sec) << "'' ";
    }
};

// Elevation-Winkel:
struct AngleEl : Angle
{
    const directionEl dir;

    AngleEl(uint16_t _angle, uint8_t _min, uint8_t _sec, directionEl _dir) : Angle(_angle, _min, _sec), dir(_dir) {} // Konstruktor für Grad, Min., Sek.-Format
    AngleEl(double _angle) : dir((_angle < 0) ? (directionEl::S) : (directionEl::N)), Angle(fabs(_angle)) {}         // Konstruktor für Gleitkommazahl-Format

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

    AngleAz(uint16_t _angle, uint8_t _min, uint8_t _sec, directionAz _dir) : Angle(_angle, _min, _sec), dir(_dir) {} // Konstruktor für Grad, Min., Sek.-Format
    AngleAz(double _angle) : dir((_angle < 0) ? (directionAz::W) : (directionAz::O)), Angle(fabs(_angle)) {}         // Konstruktor für Gleitkommazahl-Format

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
    const int8_t no;        // fortlaufende Nummer/Buchstabe (wird als Referenz für Programmparameter genutzt)
    const std::string name; // Bezeichner aus .txt-Datei

    Coordinate(uint16_t phi_angle, uint8_t phi_min, uint8_t phi_sec, directionEl phi_dir, uint16_t lambda_angle, uint8_t lambda_min, uint8_t lambda_sec, directionAz lambda_dir, const std::string &bez, int8_t _no) : phi(phi_angle, phi_min, phi_sec, phi_dir), lambda(lambda_angle, lambda_min, lambda_sec, lambda_dir), name(bez), no(_no) // Konstruktor
    {
    }

    Coordinate(float angle_el, float angle_az, const std::string &bez, int8_t _no) : phi(angle_el), lambda(angle_az), name(bez), no(_no) {}

    void print(void) const noexcept
    {
        std::cout << no << ".) " << name << '\n';
        std::cout << "\t\u03A6: "; // phi
        phi.print();
        std::cout << "\t\u03BB: "; // lambda
        lambda.print();
        std::cout << '\n'
                  << std::endl;
    }
};

inline float deg2rad(float angle) noexcept
{
    return (angle * M_PI / 180.0f);
}

inline float rad2deg(float angle) noexcept
{
    return (angle * 180.0f / M_PI);
}

// Liefert Winkel als Dezimalzahl im Bogenmaß
inline float getAngle(const AngleAz &angle) noexcept
{
    const auto calc{deg2rad(angle.angle + (angle.min / 60.0f) + (angle.sec / 3600.0f))};
    return (angle.dir == directionAz::W) ? (-1 * calc) : calc;
}

// Liefert Winkel als Dezimalzahl im Bogenmaß
inline float getAngle(const AngleEl &angle) noexcept
{
    const auto calc{deg2rad(angle.angle + (angle.min / 60.0f) + (angle.sec / 3600.0f))}; // Winkel in Grad
    return (angle.dir == directionEl::S) ? (-1 * calc) : calc;
}

// Berechnet den loxodromischen Kurs von A nach B
inline float calcLoxodromicCourse(const Coordinate &A, const Coordinate &B)
{
    // sigma von phi: [sigma](phi) = [ln(tan(pi/4 + phi/2))] oben: phi, unten: 0 (gilt nur für e = 0 (Kugel!))
    const auto sigma = [](float phi) {
        return logf(tanf(M_PI / 4 + phi / 2));
    };

    return atan2f((getAngle(B.lambda) - getAngle(A.lambda)), (sigma(getAngle(B.phi)) - sigma(getAngle(A.phi))));
}

// Berechnet loxodromische Länge von A nach B
inline float calcLoxodromicLength(const Coordinate &A, const Coordinate &B)
{
    return r_E * fabsf(calcLoxodromicCourse(A, B));
}

// Gibt Winkel zwischen zwei Punkten auf der Kugel zurück (Zentriwinkel)
float calcGCDrad(const Coordinate &A, const Coordinate &B) noexcept
{
    const auto phi_a{getAngle(A.phi)};
    const auto phi_b{getAngle(B.phi)};

    const auto lambda_a{getAngle(A.lambda)};
    const auto lambda_b{getAngle(B.lambda)};

    // Zentriwinkel:
    return acosf(sinf(phi_a) * sinf(phi_b) + cosf(phi_a) * cosf(phi_b) * cosf(lambda_b - lambda_a));
    /* Trigon. Fkt. in C++ verwenden ihren Parameter in Bogenmaß, deshalb hier noch umrechnen (* rad). Für acosf nicht nötig, da Wert bereits im Bogenmaß! */
}

// Gibt Kurswinkel zurück: Winkel zwischen Nordrichtung und Südrichtung, zeta im Bogenmaß!
float calcAlphaRad(const struct Coordinate &A, const struct Coordinate &B)
{
    const auto zeta{calcGCDrad(A, B)}; // Zentriwinkel

    if (zeta == 0)
        throw std::overflow_error("Division durch Null!"); // Division durch Null abfangen

    const auto phi_a{getAngle(A.phi)};
    const auto lambda_a{getAngle(A.lambda)};

    const auto phi_b{getAngle(B.phi)};
    const auto lambda_b{getAngle(B.lambda)};

    const auto c{calcGCDrad(A, B)};

    return acosf((sinf(phi_b) - sinf(phi_a) * cosf(c)) / (cosf(phi_a) * sinf(c)));
}

// Gibt Strecke des Winkels zurück (Orthodrom!)
inline float calcBetaRad(const Coordinate &A, const Coordinate &B) noexcept
{
    return calcAlphaRad(B, A);
}

// Gibt Strecke des Winkels zurück (Orthodrom!)
inline float calcGCDkm(const Coordinate &A, const Coordinate &B) noexcept
{
    return (calcGCDrad(A, B) * r_E);
}

// Gibt den nördlichsten Punkt auf einem Großkreis zurück
Coordinate calcNorthPeakPoint(const Coordinate &A, const Coordinate &B)
{
    // Gegeben: A und alpha
    // A
    const auto phi_a{getAngle(A.phi)};
    const auto lambda_a{getAngle(A.lambda)};
    // B
    const auto lambda_b{getAngle(B.lambda)};

    // Auf Sonderfall prüfen:
    {
        if (lambda_a == getAngle(B.lambda)) // Sonderfall: Orthodrom == Meridian: Alle Punkte auf Großkreis haben gleichen Längengrad & Großkr. geht durch Nordpol, damit ist der nördlichste Scheitelpunkt der Nordpol selbst.
            return Coordinate{90, 0, 0, directionEl::N, 0, 0, 0, directionAz::O, "Nordpol", 0};
    }

    const auto alpha{calcAlphaRad(A, B)}; // Innenwinkel sp. Dreieck
    const auto beta{calcBetaRad(A, B)};   // Innenwinkel sp. Dreieck

    enum Lage
    {
        zwischen,
        vor_A,
        hinter_B
    } lage;

    if (((alpha > 0) && (alpha < M_PI / 2)) && ((beta > 0) && (beta < M_PI / 2)))
        lage = Lage::zwischen; // Zwischen A und B
    else if (((alpha > M_PI / 2) && (alpha < M_PI)) && ((beta > 0) && (beta < M_PI / 2)))
        lage = Lage::vor_A; // Vor A
    else if (((alpha > 0) && (alpha < M_PI / 2)) && ((beta > M_PI / 2) && (beta < M_PI)))
        lage = Lage::hinter_B; // hinter B

    // Umgeht Singularität von Wechsel -180 - +180 Grad
    const auto calcDeltaLambda = [lambda_a, lambda_b]() -> float {
        const auto dL{lambda_a - lambda_b};

        if (dL > M_PI)
            return dL - 2 * M_PI;
        else if (dL < -M_PI)
            return dL + 2 * M_PI;
        else
            return dL;
    };
    const auto deltaLambda{calcDeltaLambda()};

    // Berechnung Längengrad des Scheitelpunkts abhängig von dessen Lage
    const auto calcLambda_S = [lage, lambda_a, phi_a, deltaLambda](float phi_s) -> float {
        if (lage == Lage::zwischen || lage == Lage::hinter_B)
        {
            if (deltaLambda < 0)
                return lambda_a + acosf(tanf(phi_a) / tanf(phi_s)); // Flugrichtung: Oste
            else if (deltaLambda > 0)
                return lambda_a - acosf(tanf(phi_a) / tanf(phi_s)); // Flugrichtung: Westen
        }
        else if (lage == Lage::vor_A) // umgedrehte Fälle:
        {
            if (deltaLambda < 0)
                return lambda_a - acosf(tanf(phi_a) / tanf(phi_s)); // Flugrichtung: Westen
            else if (deltaLambda > 0)
                return lambda_a + acosf(tanf(phi_a) / tanf(phi_s)); // Flugrichtung: Osten
        }
    };

    // Scheitelpunkt s
    const auto phi_s{acosf(sinf(alpha) * cosf(phi_a))};
    const auto lambda_s{calcLambda_S(phi_s)};

    // Bestimmung der Hemmisphären: Punkt liegt IMMER auf der Nordhalbkugel, Länge anhand des Vorzeichens von lambda_s ermitteln
    //const directionEl dEl{directionEl::N};
    //const directionAz dAz{(lambda_s < 0) ? (directionAz::W) : (directionAz::O)};

    // Lambda um Nachkommastellen zu extrahieren:
    /*const auto getFraction = [](float d) -> float {
        float integral;
        const auto frac{std::fmod(d, integral)};
        return frac;
    };*/

    // Mit Nachkommastellen, Minuten und Sekunden der zwei Winkel berechnen:
    //const auto phi_frac_min{getFraction(rad2deg(fabs(phi_s))) * 60.0f};
    //const auto phi_frac_sec{getFraction(phi_frac_min) * 60.0f};

    //const auto lambda_frac_min{getFraction(rad2deg(fabs(lambda_s))) * 60.0f};
    //const auto lambda_frac_sec{getFraction(lambda_frac_min) * 60.0f};

    /*return Coordinate(static_cast<uint16_t>(rad2deg(fabs(phi_s))),
                      static_cast<uint8_t>(phi_frac_min),
                      static_cast<uint8_t>(phi_frac_sec),
                      dEl,
                      static_cast<uint8_t>(rad2deg(fabs(lambda_s))),
                      static_cast<uint8_t>(lambda_frac_min),
                      static_cast<uint8_t>(lambda_frac_sec),
                      dAz,
                      "Nördlichster Punkt",
                      0); /* Soll intuitiv eindeutig sein, dass die Koordinatenangabe nicht als Parameter verwendet werden kann */

    return Coordinate(phi_s, lambda_s, "Nördlichster Punkt", 0);
}

// Berechnet Zwischenpunkt auf Großkreis (v == Speed, k == Verbrach)
Coordinate calcCrashPoint(const Coordinate &A, const Coordinate &B, float v, float k, bool interval)
{
    // A
    const auto phi_a{getAngle(A.phi)};
    const auto lambda_a{getAngle(A.lambda)};

    // B
    const auto phi_b{getAngle(B.phi)};
    const auto lambda_b{getAngle(B.lambda)};

    // alpha
    const auto alpha{calcAlphaRad(A, B)};

    // deltaLambda
    const auto deltaLambda{lambda_a - lambda_b};

    // Attribute für Streckenberechnung des Zwischenpunkts:
    const auto time{calcGCDkm(A, B) / v}; // So lange reicht der Treibstoff [h]
    const auto m{k * time};               // Treibstoffmenge (L)

    const auto distance{(m * v) / (k * r_E)}; // Strecke von A nach B in rad

    // Annahme: Bewegung von A in Richtung B!
    //const auto calcDelta = [interval, distance]() -> double {
    /*if (interval)
        {                                               // Quasi wie hin und hergeschossener Ball: Bewegt sich nur im Intervall AB
            const auto prop{range / distance};          // >1: Bewegung über Punkt B hinaus (zurück oder weiter je nach [interval]); <1: Bewegung im Interval [A,B)
            float integral;                             // nicht benötigt
            const auto frac{std::fmod(prop, integral)}; // Nachkommastellen des Verhältnisses

            if ((static_cast<uint32_t>(prop)) % 2) // Zahl ist ungerade: von A ausgehend
                return frac;
            else // Zahl ist gerade: von B ausgehend
                return 1 - frac;
        }
        else                         // fliegt - sofern Reichweite ausreicht - über B hinaus
            return range / distance;*/
    // einfaches Streckenverhältnis zurückgeben
    //};

    // Längengrad muss abhängig von Vorzeichen berechnet werden:
    const auto calcLambda_p = [deltaLambda, distance, lambda_a, phi_a](float phi_p) {
        if (((deltaLambda < 0) && (distance <= M_PI)) || ((deltaLambda > 0) && (distance > M_PI)))                 // arccos() bildet nur auf Interval [0; M_PI) ab, daher dessen Argument zusätzlich in Abhängigkeit von d behandeln!
            return lambda_a + acosf((cosf(distance) - sinf(phi_a) * sinf(phi_p)) / (cosf(phi_a) * cosf(phi_p))); // Flugrichtung: Osten
        else
            return lambda_a - acosf((cosf(distance) - sinf(phi_a) * sinf(phi_p)) / (cosf(phi_a) * cosf(phi_p))); // Flugrichtung: Westen
    };

    // ***************************** SONDERFALL *****************************
    // Sonderfall: beide Koordinaten liegen auf gleichem Großkreis: Flugrichtung ist dann direkt nach Norden bzw. Süden entlang eines Meridians
    // nachfolgend ausschließlich diverse Lambdas, die erforderliche Funktionen definieren
    // endgültige Berechnung erfolgt zum Funktionsende

    // Der Breitegrad wird auf das Intervall (-pi; pi] gemappt:
    const auto transformPhi = [](float phi_x, float lambda_x) {
        if ((phi_x >= 0) && (lambda_x < 0))
            return M_PI - phi_x;
        else if ((phi_x < 0) & (lambda_x < 0))
            return -M_PI - phi_x;
    };

    // mathematisch kürzeste Flugstrecke ist dann dPhi = phi_a - phi_b, dazu noch auf entsprechendes Interval mappen:
    const auto calcDeltaPhiDach = [](float deltaPhi_dach) -> float {
        if (deltaPhi_dach > M_PI)
            return deltaPhi_dach - 2 * M_PI;
        else if (deltaPhi_dach < -M_PI)
            return deltaPhi_dach + 2 * M_PI;
        else
            return deltaPhi_dach;
    };

    // transformierter Breitengrad des Zielpunktes:
    const auto calcTransformPhi = [distance](float deltaPhi_dach, float phi_a_dach) {
        if (deltaPhi_dach >= 0)
            return phi_a_dach - distance; // Flugrichtung: Süden (distance ist in rad)
        else
            return phi_a_dach + distance; // Flugrichtung: Norden
    };

    //const auto deltaPhi_dach{calcDeltaPhiDach(transformPhi(phi_a, lambda_a) - transformPhi(phi_b, lambda_b))};

    // Breitengrad wird auf Interval (-pi; pi] reduziert und auf Ausgangsinterval zurück transformiert:
    const auto calcInterval = [](float phi_p_dach) -> float {
        if (phi_p_dach > (M_PI / 2))
            return M_PI - phi_p_dach;
        else if (phi_p_dach < -(M_PI / 2))
            return -M_PI - phi_p_dach;
        else
            return phi_p_dach;
    };

    // Längengrad des Zielpunktes ergibt sich aus Flugdistanz zu:
    const auto calcLambda_p_special = [lambda_a, phi_a, distance]() -> float {
        if ((distance < ((M_PI / 2) - phi_a)) || (distance > ((1.5 * M_PI) - phi_a)))
            return lambda_a;
        else
            return lambda_a + M_PI;
    };

    // Berechnet den Breitengrad des Zwischenpunktes abhängig davon ob Sonderfall (auf Meridian) oder kein Sonderfall (nicht auf Meridian) vorliegt:
    const auto resPhi_p = [lambda_a, phi_a, lambda_b, phi_b, calcInterval, calcTransformPhi, calcDeltaPhiDach, transformPhi, distance, alpha]() {
        if (lambda_a == lambda_b)
            return calcInterval(calcTransformPhi(calcDeltaPhiDach(transformPhi(phi_a, lambda_a) - transformPhi(phi_b, lambda_b)), transformPhi(phi_a, lambda_a))); // Zwischenpunkt liegt auf Meridian
        else
            return asinf(cosf(distance) * sinf(phi_a) + sinf(distance) * cos(phi_a) * cosf(alpha)); // Zwischenpunkt nicht auf Meridian
    };

    // Berechnet den Längengrad des Zwischenpunktes abhängig davon ob Sonderfall (auf Meridian) oder kein Sonderfall (nicht auf Meridian) vorliegt:
    const auto resLambda_p = [lambda_a, lambda_b, calcLambda_p_special, calcLambda_p](float phi_p) {
        if (lambda_a == lambda_b)
            return calcLambda_p_special();
        else
            return calcLambda_p(phi_p);
    };

    const auto phi_p{resPhi_p()};
    const auto lambda_p{resLambda_p(phi_p)};

    return Coordinate(phi_p, lambda_p, "Zwischenpunkt", 0);
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

// Gibt den loxodromischen Kurs von A nach B auf Konsole aus:
inline void printLoxodromicCourse(const Coordinate &A, const Coordinate &B) noexcept
{
    std::cout << "Loxodromischer Kurs von " << A.name << " nach " << B.name << " beträgt:\t" << std::setprecision(4) << rad2deg(calcLoxodromicCourse(A, B)) << " Grad" << std::endl;
}

inline void printLoxodromicLength(const Coordinate &A, const Coordinate &B) noexcept
{
    std::cout << "Loxodromische Länge von " << A.name << " nach " << B.name << " beträgt:\t" << std::setprecision(5) << calcLoxodromicLength(A, B) << " km" << std::endl;
}

// Gibt den Zentriwinkel zwischen A und B auf Konsole aus:
inline void printCentricAngle(const Coordinate &A, const Coordinate &B) noexcept // Zentriwinkel
{
    std::cout << "Zentriwinkel zwischen " << A.name << " und " << B.name << " beträgt:\t" << std::setprecision(4) << rad2deg(calcGCDrad(A, B)) << " Grad" << std::endl;
}

// Gibt den Scheitelpunkt auf einem Großkreis aus:
void printNorthernmostPoint(const Coordinate &A, const Coordinate &B) noexcept
{
    std::cout << "Nördlichster Punkt auf Großkreis von " << A.name << " nach " << B.name << ":\n";
    calcNorthPeakPoint(A, B).print();

    const auto abflugswinkel{rad2deg(calcAlphaRad(A, B))};
    const auto anflugswinkel{rad2deg(calcAlphaRad(B, A))};

    const auto fallunterscheidung = [&abflugswinkel, &anflugswinkel, &A, &B]() -> void {
        std::cout << "Lage des Scheitelpunkts: ";
        if (((abflugswinkel > 0.0f) && (abflugswinkel < 90.0f)) && ((anflugswinkel > 0.0f) && (anflugswinkel < 90.0f)))
            std::cout << "innerhalb des Bogens " << A.no << B.no << "." << std::endl;
        else if (((abflugswinkel > 90.0f) && (abflugswinkel < 180.0f)) && ((anflugswinkel > 0.0f) && (anflugswinkel < 90.0f)))
            std::cout << "vor " << A.no << " (" << A.name << ")." << std::endl;
        else if (((abflugswinkel > 0.0f) && (abflugswinkel < 90.0f)) && ((anflugswinkel > 90.0f) && (anflugswinkel < 180.0f)))
            std::cout << "hinter " << B.no << " (" << B.name << ")." << std::endl;
    };

    // Ausgeben, wo sich der Scheitelpunkt grob befindet:
    fallunterscheidung();

    std::cout << std::endl;
}

inline void printHeadingAngle(const Coordinate &A, const Coordinate &B) noexcept // Kurswinkel
{
    std::cout << "Kurswinkel auf Großkreis von " << A.name << " nach " << B.name << ":\t" << std::setprecision(4) << rad2deg(calcAlphaRad(A, B)) << " Grad" << std::endl;
}

inline void printRouteLength(const Coordinate &A, const Coordinate &B) noexcept
{
    std::cout << "Strecke auf Großkreis von " << A.name << " nach " << B.name << ":\t" << std::setprecision(5) << calcGCDkm(A, B) << " km" << std::endl;
}

void printCrashPoint(const Coordinate &A, const Coordinate &B, float speed, float verbrauch, bool interval = true)
{
    std::cout << "Zwischenpunkt auf Großkreis von " << A.name << " nach " << B.name << " liegt ";
    calcCrashPoint(A, B, speed, verbrauch, interval).print();
    std::cout << std::endl;
}

int main(void)
{
    // Einleitung:
    std::cout << trennung << "\n\n\tSPHÄRISCHE TRIGONOMETRIE\n\n\t\t" << trennung << "\n\n"
              << "  Daten werden aus '" << InputFile << "' eingelesen...\n"
              << std::endl;

    // Koordinaten aus Datei einlesen:
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

    try
    {
        while (std::getline(file, input))
        {

            if (input.at(0) == '#')
            {              // Kommentare (beginnen mit #) überspringen, nur am Zeilenanfang möglich!
                counter++; // Auch bei Kommentaren Zeilen mitzählen, damit bei Fehlerausgabe exakte Zeilennummer ausgegeben wird
                continue;
            }

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

            counter++;
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Einlesefehler in Zeile " << static_cast<uint32_t>(counter) << '!' << std::endl;
        return 1;
    }

    // Alle eingelesenen Koordinaten anzeigen:
    for (auto elem : coords)
        elem.print();

    write("\nBitte Funktionscode mit Parametern eingeben: (z.B. 1 A B)\n");
    write("********************************************\n");

    // Mögliche Operationen posten
    write("Zentriwinkel: \t1 [Bezeichner] [Bezeichner]\n");
    write("Kurswinkel: \t2 [Bezeichner] [Bezeichner]\n");
    write("Scheitelpunkt: \t3 [Bezeichner] [Bezeichner]\n");
    write("Streckenlänge: \t4 [Bezeichner] [Bezeichner]\n");
    write("Loxodromischer Kurs: \t5 [Bezeichner] [Bezeichner]\n");
    write("Loxodromische Länge:\t6 [Bezeichner] [Bezeichner]\n");
    write("Zwischenpunkt: \t7 [Bezeichner] [Bezeichner] [Geschwindigkeit (double)] [Treibstoff (double)] [Verbrauch (double)] [Interval (bool)]\n");
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

            const auto A{getCoordinate(coords, static_cast<uint8_t>(userEingabe[1].c_str()[0]))};
            const auto B{getCoordinate(coords, static_cast<uint8_t>(userEingabe[2].c_str()[0]))};

            switch (cmd)
            {
            case 1:
                printCentricAngle(A, B);
                break;
            case 2:
                printHeadingAngle(A, B);
                break;
            case 3:
                printNorthernmostPoint(A, B);
                break;
            case 4:
                printRouteLength(A, B);
                break;
            case 5:
                printLoxodromicCourse(A, B);
                break;
            case 6:
                printLoxodromicLength(A, B);
                break;
            case 7:
                const auto Speed{std::atof(userEingabe[3].c_str())};     // Geschwindigkeit in km/h
                const auto Fuel{std::atof(userEingabe[4].c_str())};      // Treibstoff in t
                const auto Verbrauch{std::atof(userEingabe[5].c_str())}; // Verbrauch in L/h
                const auto Interval{std::atoi(userEingabe[6].c_str())};  // darf sich nur im Intervall (true) oder auf ganzem Großkreis bewegen

                //const auto distance{calcGCDkm(A, B)};

                printCrashPoint(A, B, Speed, Verbrauch, static_cast<bool>(Interval));

                break;
            }
        }
        catch (const std::exception &ex)
        {
            std::cerr << "Ungültige Parameter eingegeben!" << std::endl;
            continue;
        }
    } while (1);
    std::cout << std::endl;
}