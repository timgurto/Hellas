#include "testing.h"
#include "../client/Renderer.h"
#include "../client/Texture.h"

extern Renderer renderer;

TEST_CASE("Use texture after removing all others"){
    static const std::string IMAGE_FILE = "testing/tiny.png";
    {
        Texture t(IMAGE_FILE);
    }
    {
        Texture t(IMAGE_FILE);
    }
}

TEST_CASE("Renderer pixel data can be read"){
    renderer.setDrawColor(Color::YELLOW);
    renderer.fill();
    renderer.setDrawColor(Color::CYAN);
    renderer.fillRect(Rect(5, 2, 1, 1));

    renderer.present();

    Color c1 = renderer.getPixel(1, 1);
    Color c2 = renderer.getPixel(5, 2);

    CHECK(renderer.getPixel(1, 1) == Color::YELLOW);
    CHECK(renderer.getPixel(5, 2) == Color::CYAN);
}
