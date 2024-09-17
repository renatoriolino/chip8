#include <iostream>
#include <memory>

#include "machine.h"

unsigned int StrCmp(const std::string & s1, const std::string & s2)
{
    const static std::string special_values{"NXY*"};

    unsigned int grade = 0;
    ssize_t len = std::min(s1.size(), s2.size());
    for (auto i=0; i<len; ++i)
        grade += (1<<(len-i)) * ((s1[i] == s2[i]) || std::find(special_values.begin(), special_values.end(), s2[i]) != special_values.end());

    return grade;
}

bool CHIP8::LoadROM(std::ifstream & is)
{
    uint32_t count = 0;

    // auto ret = std::copy(std::istream_iterator<uint8_t>{is}, std::istream_iterator<uint8_t>{}, ram.begin() + MEMORY_USABLE);
    // count = ret - (ram.begin() + MEMORY_USABLE);

    char byte;
    for (auto i=0; i<GetRamSize() && is; ++i)
    {
        is.read(&byte, 1);
        ram[MEMORY_USABLE + i] = byte;
        ++count;
    }

    std::cout << "Loaded " << count << " bytes!\n";

    std::size_t result = 0;
    std::hash<uint8_t> hasher;
    for (auto i=MEMORY_USABLE; i<(MEMORY_USABLE + count); ++i)
        result = result * 31 + ram[i];

    std::string key_map_file{std::to_string(result)};
    while (key_map_file.length() > 8)
        key_map_file.pop_back();
    key_map_file += std::string(".kmap");

    std::cout << "Trying to load " << key_map_file << " as key map..." << std::endl;
    input.LoadKeymap(key_map_file);

    return true;
}

std::ostream& operator<<(std::ostream& os, const struct CHIP8OpParse& Op)
{
    os << "[" << std::hex << std::setw(4) << std::setfill('0') << Op.op << "]";
    os << "[NNN=" << std::hex << +Op.NNN;
    os << ";NN=" << std::hex << +Op.NN;
    os << ";N=" << std::hex << +Op.N;
    os << ";X=" << std::hex << +Op.X;
    os << ";Y=" << std::hex << +Op.Y;
    os << "] ";
    return os;
}
