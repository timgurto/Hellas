#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <SDL.h>
#include <SDL_image.h>

Uint32 getPixel(const SDL_Surface *surface, int x, int y){
    int bytesPerPixel = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bytesPerPixel;

    switch(bytesPerPixel) {
        case 1: return *p;
        case 2: return *reinterpret_cast<Uint16 *>(p);
        case 4: return *reinterpret_cast<Uint32 *>(p);
        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;

        default:
            assert(false);
            return 0;
    }
}

#undef main
int main(int argc, char *argv[]){
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Surface *map = IMG_Load("map-4.png");
    if (map == nullptr){
        const char *error = SDL_GetError();
        return 1;
    }
    size_t
        w = map->w,
        h = map->h;

    std::ofstream f("map-4.xml", std::ios_base::out);
    
    f << "<root>\n\n<size x=\"" << w << "\" y=\"" << h << "\" />\n" << std::endl;

    for (size_t y = 0; y != h; ++y){
        f << "<row ";
        if (y < 10) f << ' ';
        if (y < 100) f << ' ';
        if (y < 1000) f << ' ';
        f << "y=\"" << y << "\" terrain = \"";
        std::string terrain = "";
        for (size_t x = 0; x != w; ++x){
            Uint32 color = getPixel(map, x, y);

            auto
                part1 = (color) & 0xff,
                part2 = (color >> 8) & 0xff,
                part3 = (color >> 16) & 0xff;
            color = (part1 << 16) + (part2 << 8) + part3;

            char thisTile;
            switch (color){
                case 0x9BAE77: thisTile = 'a'; break; // normal         
                case 0x659A63: thisTile = 'b'; break; // fertile        
                case 0xB09AB0: thisTile = 'c'; break; // rocky          
                case 0xA7898F: thisTile = 'd'; break; // mountain       
                case 0x5D9A62: thisTile = 'e'; break; // plains         
                case 0x2B6647: thisTile = 'f'; break; // plainsFertile  
                case 0x83928F: thisTile = 'g'; break; // plainsRocky    
                case 0x617B7C: thisTile = 'h'; break; // plainsMountain 
                case 0x87715B: thisTile = 'i'; break; // forest         
                case 0x2F5E44: thisTile = 'j'; break; // forestFertile  
                case 0x6D7B81: thisTile = 'k'; break; // forestRocky    
                case 0x4D5862: thisTile = 'l'; break; // forestMountain 
                case 0xA1C882: thisTile = 'm'; break; // rich           
                case 0x7D9166: thisTile = 'n'; break; // richFertile    
                case 0x89958C: thisTile = 'o'; break; // richRocky      
                case 0x655884: thisTile = 'p'; break; // richMountain   
                case 0xE5E4E4: thisTile = 'q'; break; // snow           
                case 0xCDC0CE: thisTile = 'r'; break; // snowRocky      
                case 0x7F6D92: thisTile = 's'; break; // snowMountain   
                case 0x3D7C6D: thisTile = 't'; break; // swamp          
                case 0x396F78: thisTile = 'u'; break; // swampFertile   
                case 0xF3F3CA: thisTile = 'x'; break; // desert         
                case 0xE3D7B4: thisTile = 'y'; break; // desertRocky    
                case 0xB3A19C: thisTile = 'z'; break; // desertMountain 
                case 0x6893D7: thisTile = 'S'; break; // beach coast    
                case 0xF3E9AD: thisTile = 'A'; break; // beach          
                case 0xAF9C99: thisTile = 'C'; break; // beachRocky     
                case 0x705F89: thisTile = 'D'; break; // beachMountain  
                case 0xC7B4B8: thisTile = 'E'; break; // ruins          
                case 0x91969E: thisTile = 'F'; break; // ruinsFertile   
                case 0xA896B2: thisTile = 'G'; break; // ruinsRocky     
                case 0x7B6A92: thisTile = 'H'; break; // ruinsMountain  
                case 0x5F5283: thisTile = 'J'; break; // caveRocky      
                case 0x302A5D: thisTile = 'K'; break; // caveMountain   
                case 0xA8CB88: thisTile = 'L'; break; // nile           
                case 0x405D60: thisTile = 'M'; break; // nileFertile    
                case 0x384F61: thisTile = 'R'; break; // volcanoFertile  
                case 0x392354: thisTile = 'N'; break; // volcanoRocky   
                case 0x52234E: thisTile = 'O'; break; // volcanoMountain
                case 0x5870C1: thisTile = 'P'; break; // water          
                case 0x45617C: thisTile = 'Q'; break; // deepWater   
                default: thisTile = '?';
            }
            terrain += thisTile;
        }
        f << terrain << "\" />" << std::endl;
    }

    f << "\n</root>" << std::endl;
    f.close();

    SDL_Quit();
    return 0;
}