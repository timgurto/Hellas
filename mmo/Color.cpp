#include "Color.h"
#include "util.h"

const Color Color::BLACK  (0x00, 0x00, 0x00);
const Color Color::BLUE   (0x00, 0x00, 0xff);
const Color Color::GREEN  (0x00, 0xff, 0x00);
const Color Color::CYAN   (0x00, 0xff, 0xff);
const Color Color::RED    (0xff, 0x00, 0x00);
const Color Color::MAGENTA(0xff, 0x00, 0xff);
const Color Color::YELLOW (0xff, 0xff, 0x00);
const Color Color::WHITE  (0xff, 0xff, 0xff);

const Color Color::BLUE_HELL(0x18, 0x52, 0xa1);
const Color Color::NO_KEY (0x01, 0x02, 0x03);

Color::Color(Uint8 r, Uint8 g, Uint8 b):
_r(r),
_g(g),
_b(b){}

Color::Color(const SDL_Color &rhs):
_r(rhs.r),
_g(rhs.g),
_b(rhs.b){}

Color::operator SDL_Color() const{
    SDL_Color c = {_r, _g, _b, 0};
    return c;
}

Color::operator Uint32() const{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    return
        (_r << 16) |
        (_g << 8) |
        (_b);
#else
    return
        (_r) |
        (_g << 8) |
        (_b << 16);
#endif
}

Color Color::operator/(double s) const {
    if (s < 0)
        return BLACK;
    int
        r = static_cast<int>(_r/s + .5),
        g = static_cast<int>(_g/s + .5),
        b = static_cast<int>(_b/s + .5);
    if (r > 0xff) r = 0xff;
    if (g > 0xff) g = 0xff;
    if (b > 0xff) b = 0xff;
    return Color(r, g, b);
}

Color Color::operator/(int s) const {
    return *this / static_cast<double>(s);
}

Color Color::operator*(double d) const {
    if (d < 0)
        return BLACK;
    int
        r = static_cast<int>(_r * d + .5),
        g = static_cast<int>(_g * d + .5),
        b = static_cast<int>(_b * d + .5);
    if (r > 0xff) r = 0xff;
    if (g > 0xff) g = 0xff;
    if (b > 0xff) b = 0xff;
    return Color(r, g, b);
}

Color Color::operator*(int s) const {
    return *this * static_cast<double>(s);
}

Color operator+(const Color &lhs, const Color &rhs){
    Uint16 r = lhs.r();
    Uint16 g = lhs.g();
    Uint16 b = lhs.b();
    r = min(r + rhs.r(), 0xff);
    g = min(g + rhs.g(), 0xff);
    b = min(b + rhs.b(), 0xff);
    return Color(static_cast<Uint8>(r),
                 static_cast<Uint8>(g),
                 static_cast<Uint8>(b));
}
