#pragma once

#include <bitset>
#include <string>
#include <sstream>

template <typename S>
class Register
{
    S reg;

public:
    Register() : reg{0} {};
    Register(S r) : reg{r} {};

    Register& operator=(const S& r) { reg = r; return *this; };
    operator S() const { return reg; };

    S operator+(const S& n) const { return reg + n; };
    S operator-(const S& n) const { return reg - n; };
    S operator*(const S& n) const { return reg * n; };
    S operator/(const S& n) const { return reg / n; };
    S operator%(const S& n) const { return reg % n; };

    S operator+(const Register& r) const { return reg + r.reg; };
    S operator-(const Register& r) const { return reg - r.reg; };
    S operator*(const Register& r) const { return reg * r.reg; };
    S operator/(const Register& r) const { return reg / r.reg; };
    S operator%(const Register& r) const { return reg % r.reg; };
    S operator!() const { return !reg; };

    S operator&(const Register& r) const { return reg & r.reg; };
    S operator|(const Register& r) const { return reg | r.reg; };
    S operator^(const Register& r) const { return reg ^ r.reg; };
    S operator~() const { return ~reg; };

    S operator<<(const uint8_t s) const { return reg << s; };
    S operator>>(const uint8_t s) const { return reg >> s; };

    S& operator++() { return ++reg; };
    S& operator--() { return --reg; };
    S operator++(int) { return reg++; };
    S operator--(int) { return reg--; };

    void operator+=(const Register& r) { reg += r.reg; };
    void operator-=(const Register& r) { reg -= r.reg; };
    void operator*=(const Register& r) { reg *= r.reg; };
    S operator/=(const Register& r) { S ret = (reg % r.reg); reg /= r.reg; return ret; };

    void operator&=(const Register& r) { reg &= r.reg; };
    void operator|=(const Register& r) { reg |= r.reg; };
    void operator^=(const Register& r) { reg ^= r.reg; };

    void operator<<=(const uint8_t s) { reg <<= s; };
    void operator>>=(const uint8_t s) { reg >>= s; };

    std::string print_bin() const { std::stringstream ss; ss << std::bitset<sizeof(S) * 8>(reg); return ss.str(); };
    std::string print_dec() const { std::stringstream ss; ss << std::dec << +reg; return ss.str(); };
    std::string print_hex() const { std::stringstream ss; ss << "0x" << std::hex << +reg; return ss.str(); };
};

template <typename S>
std::ostream& operator<<(std::ostream& os, const Register<S>& r)
{
    os << (S)r;
    return os;
};
