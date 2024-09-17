#pragma once

#include <map>
#include <array>
#include <string>
#include <cctype>
#include <cstring>
#include <algorithm>
#include <string_view>

#include <SDL2/SDL.h>

const uint8_t gInputTotalKeys = 16;

class Input
{
public:
    enum class Key
    {
        K1=0x1, K2=0x2, K3=0x3, KC=0xc,
        K4=0x4, K5=0x5, K6=0x6, KD=0xd,
        K7=0x7, K8=0x8, K9=0x9, KE=0xe,
        KA=0xa, K0=0x0, KB=0xb, KF=0xf,
        _invalid
    };

    virtual bool IsPressed(Key k) = 0;
    virtual Key GetKey(bool wait=true) = 0;
    virtual bool LoadKeymap(const std::string & file) = 0;
};

class InputSDL : public Input
{
protected:
    std::map<Key, SDL_Scancode> to_sdl;
    std::map<SDL_Scancode, Key> from_sdl;

public:
    InputSDL(const std::array<std::string, gInputTotalKeys> & keymap = {    "X", "1", "2", "3",
                                                                            "Q", "W", "E", "A",
                                                                            "S", "D", "Z", "C",
                                                                            "V", "4", "R", "F" })
    {
        for (auto i=0; i<gInputTotalKeys; ++i)
        {
            auto k_index = Key((int)Key::K1 + i);
            auto k_scan = SDL_GetScancodeFromName(keymap[i].c_str());

            to_sdl[k_index] = k_scan;
            from_sdl[k_scan] = k_index;
        }
    }

    virtual bool IsPressed(Key k)
    {
        int numkeys = 0;
        const Uint8* sdl_keys = SDL_GetKeyboardState(&numkeys);

        auto k_scan = to_sdl[k];
        if (k_scan < numkeys)
            return sdl_keys[k_scan];
        return false;
    }

    virtual Key GetKey(bool wait=true)
    {
        int numkeys = 0;
        Key key = Key::_invalid;
        const Uint8* sdl_keys = SDL_GetKeyboardState(&numkeys);

        do
        {
            SDL_PumpEvents();
            for (auto ksdl : to_sdl)
            {
                if (ksdl.second < numkeys)
                    if (sdl_keys[ksdl.second])
                        key = ksdl.first;
            }

            if (wait)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        while (wait && key == Key::_invalid);

        while (key != Key::_invalid && !IsPressed(key))
        {
            SDL_PumpEvents();
        }

        return key;
    }

    bool LoadKeymap(const std::string & file)
    {
        std::ifstream f(file);
        if (!f.is_open())
            return false;

        std::map<Key, SDL_Scancode> new_to_sdl;
        std::map<SDL_Scancode, Key> new_from_sdl;

        const std::string comments{ "#/" };
        const std::string separator{ " \t"};

        std::string line;
        while (std::getline(f, line))
        {
            std::cout << "Parsing line: " << line << std::endl;

            auto end = std::find_first_of(line.begin(), line.end(), comments.begin(), comments.end());
            auto sep = std::find_first_of(line.begin(), end, separator.begin(), separator.end());
            if (sep != end)
            {
                std::string_view sv_key(line.begin(), sep);

                std::size_t key_end;
                char key = std::stol(std::string(sv_key), &key_end, 16) & 0xff;

                std::cout << "  sv_key=" << sv_key << " key=0x" << std::hex << +key << std::endl;

                auto sdl_key = line.find_first_not_of(separator, key_end);

                std::string_view sv_sdlkey(line.begin() + sdl_key, end);

                std::cout << "  sv_sdlkey=" << sv_sdlkey << std::endl;

                auto k_scan = SDL_GetScancodeFromName(std::string(sv_sdlkey).c_str());
                if (k_scan != SDL_SCANCODE_UNKNOWN)
                {
                    Key k_index = Key(key);
                    new_to_sdl[k_index] = k_scan;
                    new_from_sdl[k_scan] = k_index;
                }
            }
        }
        f.close();

        if (new_to_sdl.size() > 1)
        {
            to_sdl.clear();
            to_sdl = std::move(new_to_sdl);

            from_sdl.clear();
            from_sdl = std::move(new_from_sdl);
        }

        return true;
    }
};