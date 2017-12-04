#include <cassert>
#include <sstream>

#include "Client.h"
#include "ClientTerrain.h"
#include "../util.h"

ClientTerrain::ClientTerrain(const std::string &imageFile, size_t frames, ms_t frameTime):
_frames(frames),
_frame(0),
_frameTime(frameTime),
_frameTimer(frameTime)
{
    if (_frames == 1)
        _images.push_back(std::string("Images/Terrain/") + imageFile + ".png");
    else
        for (size_t i = 0; i != frames; ++i) {
            std::ostringstream oss;
            oss << "Images/Terrain/" << imageFile;
            //if (_frames > 100 && i < 100)
            //    oss << "0";
            if (_frames > 10 && i < 10)
                oss << "0";
            oss << i << ".png";
            _images.push_back(oss.str());
        }

    if (!isDebug())
        for (Texture &frame : _images) {
            frame.setBlend(SDL_BLENDMODE_ADD);
            frame.setAlpha(0x3f);
        }
}

void ClientTerrain::draw(const ScreenRect &loc, const ScreenRect &srcRect) const{
    _images[_frame].draw(loc, srcRect);
}

void ClientTerrain::draw(px_t x, px_t y) const{
    _images[_frame].draw(x, y);
}

void ClientTerrain::setHalfAlpha() const{
    _images[_frame].setAlpha(0x7f);
}

void ClientTerrain::setQuarterAlpha() const{
    _images[_frame].setAlpha(0x3f);
}

void ClientTerrain::advanceTime(ms_t timeElapsed) {
    if (_frameTime == 0)
        return;

    _frameTimer += timeElapsed;
    if (_frameTimer >= _frameTime * _frames)
        _frameTimer -= _frameTime * _frames;
    _frame = _frameTimer / _frameTime;
}
