#ifndef COMPUTOR_H
#define COMPUTOR_H

#include <string>
#include <unordered_map>
#include <optional>
#include <functional>
#include <cmath>

#include <iostream>
#include <vector>

namespace Computor {

const std::string Clear = "C/E";
const std::string History = "H";
const std::string Dot = ".";
const std::string Neg = "-/+";
const std::string Larr = "larr";
const std::string Div = "/";
const std::string Mul = "*";
const std::string Plus = "+";
const std::string Minus = "-";
const std::string Denom = "1/x";
const std::string Sqrt = "radic";
const std::string Exp2 = "x2";
const std::string Exp = "^";
const std::string Equals = "=";

std::string toString(double d) {
    auto s = std::to_string(d);
    if(std::find_if(s.begin(), s.end(), [](auto c) {return !(std::isdigit(c));}) == s.end()) //no dot there
        return s;
    auto it = s.end() - 1;
    while(*it == '0' && s.begin() + 1 <= it)
        --it;
    if(*it == '.')
        --it;
    s.erase(it + 1, s.end());
    return s;
}

class Computor {
    enum class State{Empty, Int0, Float0, Resp, Err, Int1, Float1, Op, Repeat};
    using BinaryOperation = std::unordered_map<std::string, std::function<double (double, double)>>;
    using UnaryOperation = std::unordered_map<std::string, std::function<double (double)>>;
    using StateFunction = std::function<State (const std::string&)>;
    using StateMachine = std::unordered_map<State, StateFunction>;
public:
    Computor() : m_value0(std::nan("")), m_value1(std::nan("")), m_memory(std::nan("")){}

    std::optional<double> push(const std::string& feed) {

  //const std::vector<std::string> ST{"Empty", "Int0", "Float0", "Resp", "Err", "Int1", "Float1", "Op", "Repeat"};
  //std::cerr << "C-StateA " << ST[(int) m_state] << " v0:"<<  m_value0 << " v1:"<<  m_value1 << std::endl;

        if(feed == Clear) {
            m_state = State::Empty;
        }
        if(feed == History) {
            if(std::isnan(m_memory))
                return value();
            if(m_state == State::Int1 || m_state == State::Float1 || m_state == State::Op)
                m_value1 = m_memory;
            else {
                m_value0 = m_memory;
                m_digits = 1;
                m_state = State::Float0;
            }
        }
        m_state = States.at(m_state)(feed);

  // std::cerr << "C-StateB " << ST[(int) m_state] << " v0:"<<  m_value0
  //           << " v1:"<<  m_value1 << " op:" << (m_bop == BinaryOps.end() ? "N/A" : m_bop->first) << std::endl;
        return value();
    }

    std::optional<double> value() const {
        switch (m_state) {
        case State::Empty: return 0;
        case State::Int0:
        case State::Float0:
        case State::Op:
        case State::Repeat:
        case State::Resp: return std::make_optional(m_value0);
        case State::Err: return std::nullopt;
        case State::Int1:
        case State::Float1: return std::make_optional(m_value1);
        }
        std::abort();
    }

    std::optional<double> memory() const {
        return std::isnan(m_memory) ? std::nullopt : std::make_optional(m_memory);
    }


private:
    double m_value0;
    double m_value1;
    BinaryOperation::const_iterator m_bop;
    int m_digits = 0;
    State m_state = State::Empty;
    double m_memory;

    const BinaryOperation BinaryOps = {
        {Div, [](auto a, auto b){return a / b;}},
        {Mul, [](auto a, auto b){return a * b;}},
        {Plus, [](auto a, auto b){return a + b;}},
        {Minus, [](auto a, auto b){return a - b;}},
        {Exp, [](auto a, auto b){return std::pow(a, b);}}
        };

    const UnaryOperation UnaryOps = {
        {Neg, [](auto a){return -a;}},
        {Denom, [](auto a){return a != 0.0 ? 1 / a : std::nan("");}},
        {Sqrt, [](auto a){return a >= 0 ? std::sqrt(a) : std::nan("");}},
        {Exp2, [](auto a){return a * a;}},
        {Larr, [](auto a){const auto s = toString(a); return s.length() > 1 ? std::stod(s.substr(0, s.length() - 1)) : 0;}}
        };

    static bool isNumber(const std::string& s) {
         return !s.empty() && std::find_if(s.begin(), s.end(), [](auto c) {return !(std::isdigit(c));}) == s.end();
    }

    const StateFunction handle0Opt = [this](const std::string& v) {
        const auto uop = UnaryOps.find(v);
        if(uop != UnaryOps.end()) {
            m_value0 = uop->second(m_value0);
            m_digits = 1;
            return !std::isnan(m_value0) ? State::Resp : State::Err;
        }
        const auto bop = BinaryOps.find(v);
        if(bop != BinaryOps.end()) {
            m_bop = bop;
            m_digits = 1;
            return State::Op;
        }
        return m_state;
    };

    const StateFunction handle1Opt = [this](const std::string& v) {
        const auto uop = UnaryOps.find(v);
        if(uop != UnaryOps.end()) {
            m_value0 = m_bop->second(m_value0, m_value1);
            if(!std::isnan(m_value0))
                m_value0 = uop->second(m_value0);
            m_digits = 1;
            return std::isnan(m_value0) ? State::Resp : State::Err;
        }
        const auto bop = BinaryOps.find(v);
        if(bop != BinaryOps.end()) {
            if(m_state == State::Repeat) {
                m_value0 = m_bop->second(m_value0,  m_value1);
                m_bop = bop;
                return State::Repeat;
            }
            if(std::isnan(m_value1)) {  // two binonp on row, we use laste value instead
                m_value1 = m_value0;    //m_bop->second(m_value0,  m_value0);
                m_value0 = m_bop->second(m_value0,  m_value1);
                m_bop = bop;
                return State::Repeat;
            }
            m_value0 = m_bop->second(m_value0,  m_value1);
            m_bop = bop;
            m_digits = 1;
            m_value1 = std::nan("");
           return !std::isnan(m_value0) ? State::Op : State::Err;
           }
        if(v == Equals) {
            m_digits = 1;
            if(std::isnan(m_value0) || std::isnan(m_value1))
                return State::Err;
            m_value0 = m_bop->second(m_value0, m_value1);
            m_memory = m_value0;
            return State::Resp;
        }
        return m_state;
    };

    State initState(const std::string& v) {
        m_value1 = std::nan("");
        m_bop = BinaryOps.end();
        if(isNumber(v)) {
            m_value0 = std::stod(v);
            return State::Int0;
        } else if(v == Dot) {
            m_value0 = 0.;
            return State::Float0;
        }
        return State::Empty;
    }

    template<State I, State F>
    State handleInt(const std::string& v, double& nv, const std::function<State (std::string)>& optHandler) {
        if(isNumber(v)) {
          if(std::isnan(nv))
               nv = 0;
           nv *= 10.0;
           nv += std::stod(v);
           return I;
        }
        if(v == Dot) {
            if(std::isnan(nv))
                nv = 0;
           m_digits = 1;
           return F;
        }
        return optHandler(v);
    }

   State handleFloat(const std::string& v, double& nv, const std::function<State (std::string)>& optHandler) {
        if(isNumber(v)) {
           if(std::isnan(nv))
               nv = 0;
           const auto des = std::stod(v) * std::pow(10.0, static_cast<double>(-m_digits));
           ++m_digits;
           nv += des;
        }
        return optHandler(v);
    }

    const StateMachine States = {
        {State::Empty, [this](const std::string& v) {return initState(v);}},
        {State::Err, [this](const std::string& v) {return initState(v);}},
        {State::Int0, [this](const std::string& v) {return handleInt<State::Int0, State::Float0>(v, m_value0, handle0Opt);}},
        {State::Int1, [this](const std::string& v) {return handleInt<State::Int1, State::Float1>(v, m_value1, handle1Opt);}},
        {State::Resp, [this](const std::string& v) {
             if(isNumber(v) || v == Dot) {
                 return initState(v);
             }
             if(v == Equals) {
                return State::Resp;
             }
            /* const auto bop = BinaryOps.find(v);
             if(bop != BinaryOps.end()) {
                return handle1Opt(v);
             }*/
             m_value1 = std::nan("");
             return handle0Opt(v);
         }},
       {State::Op, [this](const std::string& v) {return handleInt<State::Int1, State::Float1>(v, m_value1, handle1Opt);}},
       {State::Repeat, [this](const std::string& v) {return handleInt<State::Int1, State::Float1>(v, m_value1, handle1Opt);}},
       {State::Float0, [this](const std::string& v) {return handleFloat(v, m_value0, handle0Opt);}},
       {State::Float1, [this](const std::string& v) {return handleFloat(v, m_value1, handle1Opt);}}
    };
};
}

#endif // COMPUTOR_H
