// A core part of the xlsx serialisation routine is taking doubles from memory and stringifying them
// this has a few requirements
// - expect strings in the form 1234.56 (i.e. no thousands seperator, '.' used for the decimal seperator)
// - outputs up to 15 significant figures (excel only serialises numbers up to 15sf)

#include "benchmark/benchmark.h"
#include <cstring>
#include <locale>
#include <random>
#include <sstream>
#include <detail/serialization/serialisation_helpers.hpp>
#include <xlnt/internal/features.hpp>

namespace {

// setup a large quantity of random doubles as strings
template <bool Decimal_Locale = true>
class RandomFloats : public benchmark::Fixture
{
    static constexpr size_t Number_of_Elements = 1 << 20;
    static_assert(Number_of_Elements > 1'000'000, "ensure a decent set of random values is generated");

    std::vector<double> inputs;

    size_t index = 0;
    const char *locale_str = nullptr;

public:
    void SetUp(::benchmark::State &state)
    {
        locale_str = setlocale(LC_ALL, nullptr);
        if (Decimal_Locale)
        {
            setlocale(LC_ALL, "C");
        }
        else
        {
#ifdef XLNT_USE_LOCALE_COMMA_DECIMAL_SEPARATOR
            if (setlocale(LC_ALL, XLNT_LOCALE_COMMA_DECIMAL_SEPARATOR) == nullptr)
                state.SkipWithError(XLNT_LOCALE_COMMA_DECIMAL_SEPARATOR " locale not installed");
#else
            state.SkipWithError("Benchmarks that use a comma as decimal separator are disabled. Enable XLNT_USE_LOCALE_COMMA_DECIMAL_SEPARATOR if you want to run this benchmark.");
#endif
        }
        std::random_device rd; // obtain a seed for the random number engine
        std::mt19937 gen(rd());
        // doing full range is stupid (<double>::min/max()...), it just ends up generating very large numbers
        // uniform is probably not the best distribution to use here, but it will do for now
        std::uniform_real_distribution<double> dis(-1'000, 1'000);
        // generate a large quantity of doubles to deserialise
        inputs.reserve(Number_of_Elements);
        for (int i = 0; i < Number_of_Elements; ++i)
        {
            double d = dis(gen);
            inputs.push_back(d);
        }
    }

    void TearDown(const ::benchmark::State &state)
    {
        // restore locale
        setlocale(LC_ALL, locale_str);
        // gbench is keeping the fixtures alive somewhere, need to clear the data after use
        inputs = std::vector<double>{};
    }

    double &get_rand()
    {
        return inputs[++index & (Number_of_Elements - 1)];
    }
};

/// Takes in a double and outputs a string form of that number which will
/// serialise and deserialise without loss of precision
std::string serialize_number_to_string(double num)
{
    // more digits and excel won't match
    constexpr int Excel_Digit_Precision = 15; //sf
    std::stringstream ss;
    ss.precision(Excel_Digit_Precision);
    ss << num;
    return ss.str();
}

struct number_serialiser_production
{
    std::string serialise(double d)
    {
        return xlnt::detail::serialise(d);
    }
};

class number_serialiser_stream
{
    static constexpr int Excel_Digit_Precision = 15; //sf
    std::ostringstream ss;

public:
    explicit number_serialiser_stream()
    {
        ss.precision(Excel_Digit_Precision);
        ss.imbue(std::locale::classic());
    }

    std::string serialise(double d)
    {
        ss.str(""); // reset string buffer
        ss.clear(); // reset any error flags
        ss << d;
        return ss.str();
    }
};

// To resolve the locale issue with snprintf, a little preprocessing of the input is required.
// IMPORTANT: the string localeconv()->decimal_point might be longer than a single char
// in some locales (e.g. in the ps_AF locale which uses the arabic decimal separator).
class number_serialiser_mk2
{
    static constexpr int Excel_Digit_Precision = 15; //sf
    bool should_convert = false;

    void convert(std::string & buf)
    {
        size_t decimal_pos = buf.find(localeconv()->decimal_point);

        if (decimal_pos != std::string::npos)
        {
            buf.replace(decimal_pos, strlen(localeconv()->decimal_point), ".");
        }
    }

public:
    explicit number_serialiser_mk2()
        : should_convert(strcmp(localeconv()->decimal_point, ".") != 0)
    {
    }

    std::string serialise(double d)
    {
        std::string buf(Excel_Digit_Precision, '\0');
        int len = snprintf(&buf.at(0), buf.length() + 1, "%.15g", d);
        if (should_convert)
        {
            convert(buf);
        }
        return buf;
    }
};

using RandFloats = RandomFloats<true>;
using RandFloatsComma = RandomFloats<false>;
} // namespace

BENCHMARK_F(RandFloats, string_from_double_sstream)
(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(
            serialize_number_to_string(get_rand()));
    }
}

BENCHMARK_F(RandFloats, string_from_double_sstream_cached)
(benchmark::State &state)
{
    number_serialiser_stream ser;
    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(
            ser.serialise(get_rand()));
    }
}

BENCHMARK_F(RandFloats, string_from_double_snprintf)
(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        char buf[16];
        int len = snprintf(buf, sizeof(buf), "%.15g", get_rand());

        benchmark::DoNotOptimize(
            std::string(buf, len));
    }
}

BENCHMARK_F(RandFloats, string_from_double_snprintf_fixed)
(benchmark::State &state)
{
    number_serialiser_mk2 ser;
    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(
            ser.serialise(get_rand()));
    }
}

BENCHMARK_F(RandFloats, string_from_double_production)
(benchmark::State &state)
{
    number_serialiser_production ser;
    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(
            ser.serialise(get_rand()));
    }
}

#if XLNT_HAS_FEATURE(TO_CHARS)
#include <charconv>
BENCHMARK_F(RandFloats, string_from_double_std_to_chars)
(benchmark::State &state)
{
    while (state.KeepRunning())
    {
        char buf[16];
        std::to_chars_result result = std::to_chars(buf, buf + std::size(buf), get_rand());

        benchmark::DoNotOptimize(
            std::string(buf, result.ptr));
    }
}
#endif

BENCHMARK_F(RandFloatsComma, string_from_double_snprintf_fixed_comma)
(benchmark::State &state)
{
    number_serialiser_mk2 ser;
    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(
            ser.serialise(get_rand()));
    }
}

BENCHMARK_F(RandFloatsComma, string_from_double_production_comma)
(benchmark::State &state)
{
    number_serialiser_production ser;
    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(
            ser.serialise(get_rand()));
    }
}
