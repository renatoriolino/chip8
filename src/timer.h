#pragma once

#include <vector>
#include <cstdint>
#include <type_traits>

#include <iostream>

#include <SDL2/SDL.h>

template<typename T, uint16_t HZ = 60, std::enable_if_t<std::is_integral<T>::value, bool> = true>
class Timer
{
protected:
    T timer;
    bool enable;

    virtual void TimeStart() const { };
    virtual void TimeOver() const { };

public:
    Timer() : timer(0), enable{true} { };

    virtual void Enable() { enable = true; };
    virtual void Disable() { enable = false; };
    virtual bool IsEnabled() const { return enable; };

    virtual T Get() const { return timer; };
    virtual void Set(T _v)
    {
        T old = timer;
        timer = _v;
        if (!old && timer)
            TimeStart();
    };

    virtual void Tick()
    {
        if (enable && timer)
            if (!--timer)
                TimeOver();
    }
};

template<typename T, uint16_t HZ = 60, std::enable_if_t<std::is_integral<T>::value, bool> = true>
class TimerSDL : public Timer<T, HZ>
{
protected:
    SDL_TimerID id;
public:
    TimerSDL() : id{0}
    {
        id = SDL_AddTimer(1000 / HZ, [](Uint32 interval, void *param) {
            Timer<T, HZ> * timer = static_cast<Timer<T, HZ>*>(param);
            if (timer)
            {
                timer->Tick();
                return (Uint32)(timer->IsEnabled() ? 1000 / HZ : 0);
            }

            return (Uint32)0;
        }, this);
    }

    virtual ~TimerSDL()
    {
        if (id)
            SDL_RemoveTimer(id);
    }
};

template<typename T, uint16_t HZ = 60, std::enable_if_t<std::is_integral<T>::value, bool> = true>
class TimerAudioSDL : public TimerSDL<T, HZ>
{
protected:
    SDL_AudioSpec spec;
    SDL_AudioDeviceID audio;
    uint16_t tone;
    uint32_t last_pos;

public:

    TimerAudioSDL(uint16_t tone=440, uint32_t frequency=22000) :    spec{
                                                                        .freq = static_cast<int>(frequency),
                                                                        .format = AUDIO_S16SYS,
                                                                        .channels = 1,
                                                                        .samples = 1,
                                                                        .userdata = this
                                                                    },
                                                                    audio{0},
                                                                    tone{tone},
                                                                    last_pos{0}
    {
        spec.callback = [](void* userdata, unsigned char* stream, int len)
        {
            TimerAudioSDL<T, HZ> * timer = static_cast<TimerAudioSDL<T, HZ> *>(userdata);
            if (!timer->IsEnabled() || !timer->timer)
            {
                // Fill with silence
                SDL_memset(stream, timer->spec.silence, len);
                return;
            }

            const auto amplitude = 0.5;

            int16_t * stream16 = (int16_t*)stream;
            len /= 2;
            for (auto i=0; i<len; ++i)
            {
                stream16[i] = static_cast<int16_t>(std::sin(timer->last_pos * timer->tone * M_PI * 2 / timer->spec.freq) * 32567 * amplitude);
                if (++timer->last_pos >= timer->spec.freq)
                    timer->last_pos = 0;
            }
        };

        audio = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
        SDL_PauseAudioDevice(audio, 0);
    }

    virtual ~TimerAudioSDL()
    {
        if (audio)
            SDL_CloseAudioDevice(audio);
        audio = 0;
    }
};
