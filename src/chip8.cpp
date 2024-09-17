#include <memory>

#include <SDL2/SDL.h>
#include <config.h>

#include "src/machine.h"

#ifdef DEBUG
std::ostream &debug = std::cout;
#else
std::ofstream dev_null("/dev/null");
std::ostream &debug = dev_null;
#endif

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Please specify a ROM to load." << std::endl;
        std::cerr << "EX:" << std::endl;
        std::cerr << argv[0] << " [ROMFILE.ch8]" << std::endl;
        std::cerr << std::endl;
        return 0;
    }

    SDL_Init(SDL_INIT_EVERYTHING);

    auto m = std::shared_ptr<Machine<uint16_t, uint16_t>>(new CHIP8);
    bool running = m->LoadROM(argv[1]);

    SDL_Event event;
    while (running)
    {
        SDL_ClearError();
        if (SDL_WaitEventTimeout(&event, 1) == 0)
        {
            const char * err = SDL_GetError();
            if (err[0]) {
                std::cout << "Err:" << err << "\n";
                break;
            }
        }

        switch (event.type)
        {
        case SDL_QUIT:
            running = false;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.scancode == SDL_GetScancodeFromName("Escape"))
                running = false;
            if (event.key.keysym.scancode == SDL_GetScancodeFromName("F5"))
                m->Reset();
            std::cout << "Umapped key pressed: " << SDL_GetScancodeName(event.key.keysym.scancode) << std::endl;
            break;
        }

        m->Task();
    }

    SDL_Quit();

    return 0;
}