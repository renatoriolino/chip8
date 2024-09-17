#pragma once

#include <cstdint>
#include <bitset>

#include <thread>
#include <chrono>

#include "config.h"
#include "display.h"
#include "register.h"

namespace Instructions
{

template <class IT>
void ClearDisplay(IT video_ram, uint8_t _W, uint8_t _H)
{
    auto end = video_ram;
    std::advance(end, (_W*_H) / 8);
    std::fill(video_ram, end, 0x0);
}

void Jump(Register<uint64_t> * PC, uint64_t addr);
void SkipNext(Register<uint64_t> * PC, bool condition);

template <typename T1, typename T2>
void AssignV(Register<T1> * to, T2 from)
{
    if (to)
        *to = from;
}

template <typename T1, typename T2>
void Assign(Register<T1> * to, Register<T2> * from)
{
    if (from)
        AssignV(to, (T2)*from);
}

template <typename T1, typename T2>
void AddV(Register<T1> * to, T2 from, Register<T2> * overflow)
{
    if (!to || !from)
        return;

    bool over = int64_t((T1)*to + from) > std::numeric_limits<T1>::max();

    *to += (T2)from;

    // Overflow needs to be set AFTER the Add
    if (overflow)
        *overflow = over;
}

template <typename T1, typename T2>
void Add(Register<T1> * to, Register<T2> * from, Register<T2> * overflow)
{
    if (from)
        AddV(to, (T2)*from, overflow);
}

template <typename T1, typename T2>
void SubV(Register<T1> * to, T2 from, Register<T2> * underflow)
{
    if (!to || !from)
        return;

    bool under = ((T1)*to < from);

    *to -= from;

    if (underflow)
        *underflow = !under;
}

template <typename T1, typename T2>
void SubVAlt(Register<T1> * to, T2 from, Register<T2> * underflow)
{
    if (!to || !from)
        return;

    bool under = (from < (T1)*to);

    *to = from - *to;

    if (underflow)
        *underflow = !under;
}

template <typename T1, typename T2>
void Sub(Register<T1> * to, Register<T2> * from, Register<T2> * underflow)
{
    if (from)
        SubV(to, (T2)*from, underflow);
}

template <typename T1, typename T2>
void RShiftV(Register<T1> * to, uint8_t from, Register<T2> * overflow)
{
    if (!to || !from)
        return;

    T1 mask = 1;
    for (auto i=1; i<from; ++i)
        mask = 1 | (mask << 1);

    mask &= (T1)*to;

    *to >>= from;

    if (overflow)
        *overflow = !!mask;
}

template <typename T1, typename T2>
void LShiftV(Register<T1> * to, uint8_t from, Register<T2> * overflow)
{
    if (!to || !from)
        return;

    T1 mask = 0x80;
    for (auto i=1; i<from; ++i)
        mask = 0x80 | (mask >> 1);

    mask &= (T1)*to;

    *to <<= from;

    if (overflow)
        *overflow = !!mask;
}

template <class IT, typename T1, typename T2, unsigned int N>
void Store(IT ram, const Register<uint16_t> & _I, T1 _X, std::array<Register<T2>, N> * _V)
{
    ram += _I;
    auto end = _V->begin() + _X + 1;

    std::copy(_V->begin(), end, ram);
}

template <class IT, typename T1, typename T2, unsigned int N>
void Fill(IT ram, const Register<uint16_t> & _I, T1 _X, std::array<Register<T2>, N> * _V)
{
    ram += _I;
    auto end = ram + _X + 1;

    std::copy(ram, end, _V->begin());
}

template <class IT>
void BCD(IT ram, uint8_t _N, const Register<uint16_t> & _I)
{
    for (int8_t i=2; i>=0; --i) {
        *(ram + _I + i) = _N % 10;
        _N /= 10;
    }
}

template <class IT, typename T1>
void Draw(IT ram, IT video_ram, Register<T1> & _VF, Register<uint8_t> & _X, Register<uint8_t> & _Y, Register<uint16_t> & _I, uint8_t _N, uint8_t _W, uint8_t _H)
{
    // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
    // Each row of 8 pixels is read as bit-coded starting from memory location I; I value does not
    // change after the execution of this instruction. As described above, VF is set to 1 if any
    // screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that does
    // not happen.

    // Normalize _X and _Y so that it can wrap around
    uint8_t X = ((uint8_t)_X) % _W;
    uint8_t Y = ((uint8_t)_Y) % _H;

    // Sets ram to the address of the first sprite
    ram += _I;

    debug << "-- Y is " << +Y << " and X is " << +X << " --" << std::endl;

    // Sets video_ram to the address of the first pixel
    video_ram += (Y * _W + X) / 8;

    while (_N-- && Y++ < _H)
    {
        debug << "-- DRAWING LINE " << +(Y-1) << "--" << std::endl;
        debug << "video_ram.........: " << std::bitset<8>(*video_ram) << std::bitset<8>(*(video_ram+1)) << std::endl;

        // Get screen current pixels data
        uint8_t screen_data = *video_ram << (X % 8);
        if (X % 8)
            screen_data |= *(video_ram+1) >> (8 - (X % 8));

        std::string prefix{};
        if ((X % 8))
            for (auto i=0; i<((X%8)); ++i)
                prefix.push_back('-');
        std::string sufix{};
        if (8 - (X % 8))
            for (auto i=0; i<(8-(X%8)); ++i)
                sufix.push_back('-');

        debug << "sprite (ram)......: " << prefix << std::bitset<8>(*ram) << sufix << std::endl;
        debug << "screen_data.......: " << prefix << std::bitset<8>(screen_data) << sufix << std::endl;

        // XOR between screen_data and the sprite in ram
        uint8_t screen_data_xored = screen_data ^ *ram;        

        debug << "screen_data_xored.: " << prefix << std::bitset<8>(screen_data_xored) << sufix << std::endl;

        // If sprite goes beyond screen width, clip it
        uint8_t screen_data_mask = 0xff;
        if ((X + 8) > _W)
            screen_data_mask <<= (8 - (_W - X));
        screen_data &= screen_data_mask;
        screen_data_xored &= screen_data_mask;

        debug << "-- CLIPPING --" << std::endl;
        debug << "screen_data.......: " << prefix << std::bitset<8>(screen_data) << sufix << std::endl;
        debug << "screen_data_xored.: " << prefix << std::bitset<8>(screen_data_xored) << sufix << std::endl;

        // Test if any bit flipped to 0
        _VF = !!((screen_data & screen_data_xored) != screen_data);

        debug << "VF (COLLISION): " << _VF.print_dec() << std::endl;

        // Clear video_ram area and then writes the XORed data
        *video_ram &= ~(screen_data_mask >> (X % 8));
        *video_ram |= screen_data_xored >> (X % 8);
        if (X % 8) {
            *(video_ram + 1) &= ~(screen_data_mask << (8 - (X % 8)));
            *(video_ram + 1) |= screen_data_xored << (8 - (X % 8));
        }

        debug << "-- AFTER DRAWING --" << std::endl;
        debug << "video_ram.........: " << std::bitset<8>(*video_ram) << std::bitset<8>(*(video_ram+1)) << std::endl;

        // Increment ram to get the next sprite
        ram += 1;
        // Increment video_ram to get to the next line
        video_ram += (_W / 8);
    }
}
}
