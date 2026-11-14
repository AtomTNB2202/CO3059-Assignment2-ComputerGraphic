#include "Expression.h"
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace
{
using Function = std::function<double(double, double)>;
class Parser
{
  public:
    explicit Parser(const std::string &source) : text(source) {}
    Function parse()
    {
        if (text.size() > 256)
            throw std::runtime_error("Expression must be at most 256 characters.");
        Function result = sum();
        skipSpaces();
        if (position != text.size())
            fail("Unexpected character");
        return result;
    }

  private:
    std::string text;
    size_t position = 0;
    int depth = 0;
    [[noreturn]] void fail(const std::string &message) const
    {
        throw std::runtime_error(message + " at character " + std::to_string(position + 1));
    }
    void skipSpaces()
    {
        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position])))
            ++position;
    }
    bool take(char c)
    {
        skipSpaces();
        if (position < text.size() && text[position] == c)
        {
            ++position;
            return true;
        }
        return false;
    }
    Function sum()
    {
        Function left = product();
        while (true)
        {
            if (take('+'))
            {
                auto right = product();
                left = [left, right](double x, double y) { return left(x, y) + right(x, y); };
            }
            else if (take('-'))
            {
                auto right = product();
                left = [left, right](double x, double y) { return left(x, y) - right(x, y); };
            }
            else
                return left;
        }
    }
    Function product()
    {
        Function left = unary();
        while (true)
        {
            if (take('*'))
            {
                auto right = unary();
                left = [left, right](double x, double y) { return left(x, y) * right(x, y); };
            }
            else if (take('/'))
            {
                auto right = unary();
                left = [left, right](double x, double y) { return left(x, y) / right(x, y); };
            }
            else
                return left;
        }
    }
    Function unary()
    {
        if (++depth > 64)
            fail("Expression nesting too deep");
        Function result;
        if (take('+'))
            result = unary();
        else if (take('-'))
        {
            auto child = unary();
            result = [child](double x, double y) { return -child(x, y); };
        }
        else
        {
            result = primary();
            if (take('^'))
            {
                auto exponent = unary();
                auto base = result;
                result = [base, exponent](double x, double y)
                { return std::pow(base(x, y), exponent(x, y)); };
            }
        }
        --depth;
        return result;
    }
    Function primary()
    {
        if (take('('))
        {
            auto result = sum();
            if (!take(')'))
                fail("Expected )");
            return result;
        }
        skipSpaces();
        if (position >= text.size())
            fail("Expected a value");
        if (std::isdigit(static_cast<unsigned char>(text[position])) || text[position] == '.')
        {
            const char *start = text.c_str() + position;
            char *end = nullptr;
            double value = std::strtod(start, &end);
            if (end == start || !std::isfinite(value))
                fail("Invalid number");
            position += static_cast<size_t>(end - start);
            return [value](double, double) { return value; };
        }
        size_t start = position;
        while (position < text.size() && std::isalpha(static_cast<unsigned char>(text[position])))
            ++position;
        std::string name = text.substr(start, position - start);
        if (name == "x")
            return [](double x, double) { return x; };
        if (name == "y")
            return [](double, double y) { return y; };
        if (name == "pi")
            return [](double, double) { return 3.141592653589793; };
        if (name == "e")
            return [](double, double) { return std::exp(1.0); };
        if (!take('('))
            fail("Expected x, y, a number, or a function");
        auto value = sum();
        if (!take(')'))
            fail("Expected )");
        if (name == "sin")
            return [value](double x, double y) { return std::sin(value(x, y)); };
        if (name == "cos")
            return [value](double x, double y) { return std::cos(value(x, y)); };
        if (name == "tan")
            return [value](double x, double y) { return std::tan(value(x, y)); };
        if (name == "sqrt")
            return [value](double x, double y) { return std::sqrt(value(x, y)); };
        if (name == "abs")
            return [value](double x, double y) { return std::abs(value(x, y)); };
        if (name == "exp")
            return [value](double x, double y) { return std::exp(value(x, y)); };
        if (name == "log")
            return [value](double x, double y) { return std::log(value(x, y)); };
        fail("Unknown function: " + name);
    }
};
} // namespace
Expression::Expression(const std::string &text) : function(Parser(text).parse()) {}
double Expression::evaluate(double x, double y) const
{
    double value = function(x, y);
    if (!std::isfinite(value) || std::abs(value) > 10000)
        throw std::runtime_error("Surface has an undefined or excessive height (limit 10000).");
    return value;
}
