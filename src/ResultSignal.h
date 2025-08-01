#pragma once
#include <vector>
#include "Constants.h"
#include <cassert>
#include <iostream>
#include <sstream>
enum class Unit
{
    Hertz,
    Second_Per_Day,
    Second_Per_Month,
    Second_Per_Year,
    PPM,
};

// Pour affichage humain (optionnel mais pratique)
inline std::ostream &operator<<(std::ostream &os, const Unit &u)
{
    switch (u)
    {
    case Unit::Hertz:
        return os << "Hz";
    case Unit::Second_Per_Day:
        return os << "s/day";
    case Unit::Second_Per_Month:
        return os << "s/month";
    case Unit::Second_Per_Year:
        return os << "s/year";
    case Unit::PPM:
        return os << "ppm";
    }
    return os << "unknown";
}

inline std::string to_string(Unit u)
{
    std::ostringstream oss;
    oss << u;
    return oss.str();
}

struct Result
{
    std::vector<double> magnitudes;
    std::vector<double> phases;
    std::vector<double> frequencies;
    double time;
    double progress = 0.0;
    double nominal_frequency;
    double real_frequency;

    double getProgressPercent() { return progress * 100; }

    template <typename Converter>
    std::vector<double> compute_units(Converter &&converter) const
    {

        std::vector<double> res;
        res.reserve(frequencies.size());
        for (double freq : frequencies)
        {
            res.push_back(converter(freq, real_frequency));
        }
        return res;
    }
    std::vector<double> herz() const
    {
        return compute_units([](double freq, double nominal) { return freq - nominal; });
    }

    std::vector<double> sec_per_day() const
    {
        return compute_units([](double freq, double nominal)
            { return ((freq - nominal) * Constants::SECONDS_PER_DAY) / nominal; });
    }

    std::vector<double> sec_per_month() const
    {
        return compute_units([](double freq, double nominal)
            { return ((freq - nominal) * Constants::SECONDS_PER_MONTH) / nominal; });
    }

    std::vector<double> sec_per_year() const
    {
        return compute_units([](double freq, double nominal)
            { return ((freq - nominal) * Constants::SECONDS_PER_YEAR) / nominal; });
    }

    std::vector<double> ppm() const
    {
        return compute_units(
            [](double freq, double nominal) { return ((freq - nominal) * 1e6) / nominal; });
    }

    std::vector<double> result_unit(Unit u) const
    {
        switch (u)
        {
        case Unit::Hertz:
            return herz();
        case Unit::Second_Per_Day:
            return sec_per_day();
        case Unit::Second_Per_Month:
            return sec_per_month();
        case Unit::Second_Per_Year:
            return sec_per_year();
        case Unit::PPM:
            return ppm();
        default:
            throw std::invalid_argument("Unité non supportée");
        }
    }

    std::string unit_string(Unit u) const
    {
        std::stringstream ss;

        switch (u)
        {
        case Unit::Hertz:
            ss << "Frequency offset from " << nominal_frequency << " Hz (Hz)";
            break;
        case Unit::Second_Per_Day:
            ss << "Drift in seconds per day (spd)";
            break;
        case Unit::Second_Per_Month:
            ss << "Drift in seconds per month (spm)";
            break;
        case Unit::Second_Per_Year:
            ss << "Drift in seconds per year (spy)";
            break;
        case Unit::PPM:
            ss << "Frequency offset from " << nominal_frequency << "Hz (ppm)";
            break;
        default:
            throw std::invalid_argument("Unité non supportée");
        }
        return ss.str();
    }
};
