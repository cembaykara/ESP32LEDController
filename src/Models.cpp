#include <Models.h>

String getColorString(Color color) {
    switch (color) {
        case Color::R: return "R";
        case Color::G: return "G";
        case Color::B: return "B";
        case Color::CW: return "CW";
        case Color::CC: return "CC";
        default: return "Unknown";
    }
}