#pragma once

#include <array>
#include <stack>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <optional>
#include <iterator>
#include <functional>

#include <chrono>
#include <thread>
#include <iostream>

#include "config.h"
#include "register.h"
#include "timer.h"
#include "input.h"
#include "display.h"
#include "instructions.h"

unsigned int StrCmp(const std::string & s1, const std::string & s2);

template <typename tOp, typename tIns>
class Machine
{
protected:
    typedef std::map<std::string, std::function<void(tOp)>> InstrMap;

    Register<uint64_t> PC;                      // Program Counter
    InstrMap instr;                             // Implemented Instructions

protected:
    virtual bool LoadROM(std::ifstream & is) = 0;

public:
    virtual std::size_t GetRamSize() const = 0;
    virtual std::optional<uint8_t> RamReadByte(uint64_t addr) const = 0;
    virtual bool RamWriteByte(uint64_t addr, uint8_t byte) = 0;

    virtual void Reset() = 0;

    virtual bool LoadROM(const std::string & rom)
    {
        std::ifstream is;

        is.open(rom, std::ios::in | std::ios::binary);
        if (!is.is_open()) {
            std::cerr << "Error loading ROM " << rom << "\n";
            return false;
        }

        // Get ROM size
        is.seekg(0, std::ios::end);
        auto length = is.tellg();
        is.seekg(0, std::ios::beg);

        if (length > GetRamSize())
        {
            is.close();
            std::cerr << "ROM " << rom << " won't fit on system ram!\n";
            return false;
        }

        auto ret = LoadROM(is);
        is.close();
        return ret;
    };

    virtual InstrMap::iterator FindBestInstruction(const std::string & sinstr)
    {
        auto best = instr.end();
        int best_matches = 0;

        for (auto it = instr.begin(); it != instr.end(); ++it)
        {
            const int matches = StrCmp(sinstr, it->first);
            if (matches > best_matches)
            {
                best_matches = matches;
                best = it;
            }
        }

        if (best == instr.end() || best_matches != 30)
            std::cout << "No match found for " << sinstr << "!\n";

        return best;
    }

    virtual void Task()
    {
        static bool FATAL = false;
        if (FATAL)
            return;

        std::optional<uint8_t> op[2];
        for (auto i=0; i<2; ++i)
        {
            if (!(op[i] = RamReadByte(PC)))
            {
                std::cerr << "Failed to read memory address 0x" << PC.print_hex() << "!\n";
                FATAL = true;
                return;
            }

            ++PC;
        }

        uint16_t opcode = op[0].value() << 8 | op[1].value();
        if (opcode == 0) {
            FATAL = true;
            return;
        }

        // Converts opcode to string
        std::stringstream ss;
        ss << std::hex << std::setw(4) << std::setfill('0') << opcode;
        auto ix = FindBestInstruction(ss.str());
        if (ix != instr.end())
            ix->second(opcode);
        else
            FATAL = true;
    };
};

struct CHIP8OpParse
{
    uint16_t op;
    uint16_t NNN;
    uint8_t NN;
    uint8_t N;
    uint8_t X;
    uint8_t Y;

    CHIP8OpParse(uint16_t op)
    {
        this->op = op;
        NNN = (op & 0x0FFF);
        NN = (op & 0x0FF);
        N = (op & 0x0F);
        X = ((op >> 8) & 0x0F);
        Y = ((op >> 4) & 0x0F);
    }
};

std::ostream& operator<<(std::ostream& os, const struct CHIP8OpParse& Op);

class CHIP8 : public Machine<uint16_t, uint16_t>
{
private:
    const unsigned int MEMORY_FONTS = 0x050;
    const unsigned int MEMORY_USABLE = 0x200;
    const unsigned int MEMORY_VIDEO = 0xF00;
    typedef std::array<uint8_t, 4096> MemorySpecs;

protected:
    MemorySpecs ram;                            // 0x000 - 0x200 = RESERVED FOR INTERPRETER (FONTS AT 0x050 ~ 0x09F)
                                                // 0xF00 - 0xFFF = DISPLAY REFRESH
                                                // 0xEA0 - 0xEFF = CALL STACK, INTERNAL USE, OTHER VARIABLES

    std::array<Register<uint8_t>, 16> V;        // V0 .. VF
                                                // The VF register doubles as a flag for some instructions; thus, it should be avoided
                                                // In an addition operation, VF is the carry flag, while in subtraction, it is the "no borrow" flag.
                                                // In the draw instruction VF is set upon pixel collision.

    Register<uint16_t> I;                       // The address register, which is named I, is 12 bits wide and is used with several opcodes that
                                                // involve memory operations.

    std::stack<uint16_t> stack;

    TimerSDL<uint8_t, 60> delay;                // 60hz timer
    TimerAudioSDL<uint8_t, 60> audio;           // 60hz timer with audio
    TimerSDL<uint8_t, 60> disp_wait;            // 60hz display refresh

    InputSDL input;

    DisplaySDL display;

protected:
    virtual bool LoadROM(std::ifstream & is);

public:
    CHIP8() : ram{}, V{}, I{}, delay{}, audio{600}, disp_wait{}, input{}, display{64, 32, 10}
    {
        std::array<uint8_t, 16*5> builtin_fonts
        {
            // Built in fonts
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };
        std::move(builtin_fonts.begin(), builtin_fonts.end(), ram.begin() + MEMORY_FONTS);

        instr["0NNN"] = [this](CHIP8OpParse op)
        {
            debug << op << "Calls machine code routine at address 0x" << std::hex << +op.NNN << "\n";
        };
        instr["00e0"] = [this](CHIP8OpParse op)
        {
            debug << op << "Clears the screen\n";
            Instructions::ClearDisplay<MemorySpecs::iterator>(ram.begin() + MEMORY_VIDEO, display.GetW(), display.GetH());
        };
        instr["00ee"] = [this](CHIP8OpParse op)
        {
            debug << op << "Returns from a subroutine\n";

            if (stack.size() == 0)
                return;

            Instructions::AssignV<uint64_t, uint16_t>(&PC, stack.top());
            stack.pop();
        };
        instr["1NNN"] = [this](CHIP8OpParse op)
        {
            debug << op << "Jumps to address 0x" << std::hex << +op.NNN << "\n";
            Instructions::AssignV<uint64_t, uint16_t>(&PC, op.NNN);
        };
        instr["2NNN"] = [this](CHIP8OpParse op)
        {
            debug << op << "Calls subroutine at 0x" << std::hex << +op.NNN << "\n";
            stack.push(PC);
            Instructions::AssignV<uint64_t, uint16_t>(&PC, op.NNN);
        };
        instr["3XNN"] = [this](CHIP8OpParse op)
        {
            debug << op << "Skips the next instruction if V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") equals 0x" << std::hex << +op.NN << "\n";
            Instructions::SkipNext(&PC, (V[op.X] == op.NN));
        };
        instr["4XNN"] = [this](CHIP8OpParse op)
        {
            debug << op << "Skips the next instruction if V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") does not equal 0x" << std::hex << +op.NN << "\n";
            Instructions::SkipNext(&PC, (V[op.X] != op.NN));
        };
        instr["5XY0"] = [this](CHIP8OpParse op)
        {
            debug << op << "Skips the next instruction if V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") equals V" << std::hex << +op.Y << " (" << V[op.Y].print_hex() << ")\n";
            Instructions::SkipNext(&PC, (V[op.X] == V[op.Y]));
        };
        instr["6XNN"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to " << std::dec << +op.NN << "\n";
            Instructions::AssignV<uint8_t, uint8_t>(&V[op.X], op.NN);
        };
        instr["7XNN"] = [this](CHIP8OpParse op)
        {
            debug << op << "Adds " << std::dec << +op.NN << " to V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") (VF is not changed)\n";
            Instructions::AddV<uint8_t, uint8_t>(&V[op.X], op.NN, nullptr);
        };
        instr["8XY0"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to the value of V" << std::hex << +op.Y << " (" << V[op.Y].print_hex() << ")\n";
            Instructions::Assign<uint8_t, uint8_t>(&V[op.X], &V[op.Y]);
        };
        instr["8XY1"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") or V" << std::hex << +op.Y << " (" << V[op.Y].print_hex() << ")\n";
            Instructions::AssignV<uint8_t, uint8_t>(&V[op.X], V[op.X] | V[op.Y]);
            V[0xf] = 0;
        };
        instr["8XY2"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") and V" << std::hex << +op.Y << " (" << V[op.Y].print_hex() << ")\n";
            Instructions::AssignV<uint8_t, uint8_t>(&V[op.X], V[op.X] & V[op.Y]);
            V[0xf] = 0;
        };
        instr["8XY3"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") xor V" << std::hex << +op.Y << " (" << V[op.Y].print_hex() << ")\n";
            Instructions::AssignV<uint8_t, uint8_t>(&V[op.X], V[op.X] ^ V[op.Y]);
            V[0xf] = 0;
        };
        instr["8XY4"] = [this](CHIP8OpParse op)
        {
            debug << op << "8XY4: Adds V" << std::hex << +op.Y << " (" << V[op.Y].print_hex() << ") to V" << std::hex << +op.X << " (" << V[op.X].print_hex() << "). VF is set to 1 when there's an overflow, and to 0 when there is not\n";
            Instructions::Add<uint8_t, uint8_t>(&V[op.X], &V[op.Y], &V[0xf]);
        };
        instr["8XY5"] = [this](CHIP8OpParse op)
        {
            debug << op << "V" << std::hex << +op.Y << " (" << V[op.Y].print_hex() << ") is subtracted from V" << std::hex << +op.X << " (" << V[op.X].print_hex() << "). VF is set to 0 when there's an underflow, and 1 when there is not\n";
            Instructions::Sub<uint8_t, uint8_t>(&V[op.X], &V[op.Y], &V[0xf]);
        };
        instr["8XY6"] = [this](CHIP8OpParse op)
        {
            V[op.X] = (uint8_t)V[op.Y];
            debug << op << "Shifts V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to the right by 1, then stores the least significant bit of V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") prior to the shift into VF\n";
            Instructions::RShiftV<uint8_t, uint8_t>(&V[op.X], 1, &V[0xf]);
        };
        instr["8XY7"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to V" << std::hex << +op.Y << " (" << V[op.Y].print_hex() << ") minus V" << std::hex << +op.X << " (" << V[op.X].print_hex() << "). VF is set to 0 when there's an underflow, and 1 when there is not\n";
            Instructions::SubVAlt<uint8_t, uint8_t>(&V[op.X], V[op.Y], &V[0xf]);
        };
        instr["8XYe"] = [this](CHIP8OpParse op)
        {
            V[op.X] = (uint8_t)V[op.Y];
            debug << op << "Shifts V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to the left by 1, then sets VF to 1 if the most significant bit of V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") prior to that shift was set, or to 0 if it was unset\n";
            Instructions::LShiftV<uint8_t, uint8_t>(&V[op.X], 1, &V[0xf]);
        };
        instr["9XY0"] = [this](CHIP8OpParse op)
        {
            debug << op << "Skips the next instruction if V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") does not equal VV" << std::hex << +op.Y << " (" << V[op.Y].print_hex() << ")\n";
            Instructions::SkipNext(&PC, (V[op.X] != V[op.Y]));
        };
        instr["aNNN"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets I to the address 0x" << std::hex << +op.NNN << "\n";
            Instructions::AssignV<uint16_t, uint16_t>(&I, op.NNN);
        };
        instr["bNNN"] = [this](CHIP8OpParse op)
        {
            uint8_t target_register = op.X; // 0;
            debug << op << "Jumps to the address 0x" << std::hex << +op.NNN << " plus V" << std::hex << +target_register << " (0x" << std::hex << V[target_register] << ")\n";
            Instructions::Jump(&PC, op.NNN + V[0]);
        };
        instr["cXNN"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to the result of a bitwise and operation on a random number and 0x" << std::hex << +op.NN << "\n";
            Instructions::AssignV<uint8_t, uint8_t>(&V[op.X], uint8_t(std::rand() & 0xff) & op.NN);
        };
        instr["dXYN"] = [this](CHIP8OpParse op)
        {
            if (disp_wait.Get())
            {
                PC -= 2;
                return;
            }

            debug << op << "Draws a sprite at coordinate x=V" << std::hex << +op.X << " (" << V[op.X].print_dec() << ") and y=V" << std::hex << +op.Y << " (" << V[op.Y].print_dec() << ") with width of 8 by height of " << std::dec << +op.N << " pixels\n";
            Instructions::Draw<MemorySpecs::iterator, uint8_t>(ram.begin(), ram.begin() + MEMORY_VIDEO, V[0xF], V[op.X], V[op.Y], I, op.N, display.GetW(), display.GetH());

            display.Draw(ram.begin() + MEMORY_VIDEO, ram.begin() + MEMORY_VIDEO + display.GetW() * display.GetH());
            disp_wait.Set(1);
        };
        instr["eX9e"] = [this](CHIP8OpParse op)
        {
            debug << op << "Skips the next instruction if the key stored in V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") is pressed\n";
            Instructions::SkipNext(&PC, (input.IsPressed(Input::Key(uint8_t(V[op.X])))));
        };
        instr["eXa1"] = [this](CHIP8OpParse op)
        {
            debug << op << "Skips the next instruction if the key stored in V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") is not pressed\n";
            Instructions::SkipNext(&PC, (!input.IsPressed(Input::Key(uint8_t(V[op.X])))));
        };
        instr["fX07"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to the value of the delay timer (" << std::dec << +delay.Get() << ")\n";
            Instructions::AssignV<uint8_t, uint8_t>(&V[op.X], delay.Get());
        };
        instr["fX0a"] = [this](CHIP8OpParse op)
        {
            static bool msg_showed = false;
            if (!msg_showed) {
                debug << op << "A key press is awaited, and then stored in V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") (blocking operation)\n";
                msg_showed = true;
            }

            auto key = input.GetKey(false);
            if (key == Input::Key::_invalid)
            {
                PC -= 2;
                return;
            }
            Instructions::AssignV<uint8_t, uint8_t>(&V[op.X], uint8_t(key));
            msg_showed = false;
        };
        instr["fX15"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets the delay timer (" << std::dec << +delay.Get() << ") to V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ")\n";
            delay.Set(V[op.X]);
        };
        instr["fX18"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets the sound timer to V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ")\n";
            audio.Set(V[op.X]);
        };
        instr["fX1e"] = [this](CHIP8OpParse op)
        {
            debug << op << "Adds V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") to I. VF is not affected\n";
            Instructions::AddV<uint16_t, uint8_t>(&I, V[op.X], nullptr);
        };
        instr["fX29"] = [this](CHIP8OpParse op)
        {
            debug << op << "Sets I to the location of the sprite for the character in V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ")\n";
            I = MEMORY_FONTS + ((uint8_t)V[op.X] * 5);
        };
        instr["fX33"] = [this](CHIP8OpParse op)
        {
            debug << op << "Stores the binary-coded decimal representation of V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ")\n";
            Instructions::BCD<MemorySpecs::iterator>(ram.begin(), V[op.X], I);
        };
        instr["fX55"] = [this](CHIP8OpParse op)
        {
            debug << op << "Stores from V0 to V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") (including Vx) in memory, starting at address I\n";
            Instructions::Store<MemorySpecs::iterator, uint8_t, uint8_t, 16>(ram.begin(), I, op.X, &V);
            I += op.X + 1;
        };
        instr["fX65"] = [this](CHIP8OpParse op)
        {
            debug << op << "Fills from V0 to V" << std::hex << +op.X << " (" << V[op.X].print_hex() << ") (including Vx) with values from memory, starting at address I\n";
            Instructions::Fill<MemorySpecs::iterator, uint8_t, uint8_t, 16>(ram.begin(), I, op.X, &V);
            I += op.X + 1;
        };

        Reset();
    };

    virtual std::size_t GetRamSize() const { return (ram.size() - MEMORY_USABLE); };
    virtual std::optional<uint8_t> RamReadByte(uint64_t addr) const { if (addr >= ram.size()) return std::nullopt; return ram.at(addr); };
    virtual bool RamWriteByte(uint64_t addr, uint8_t byte) { if (addr >= ram.size()) return false; ram[addr] = byte; return true; };

    virtual void Reset()
    {
        PC = MEMORY_USABLE;
        std::srand(12345);
    }

    virtual void Task()
    {
        Machine::Task();
        // auto vram = ram.begin();
        // std::advance(vram, MEMORY_VIDEO);
        // display.Draw(vram, ram.end());
    };
};