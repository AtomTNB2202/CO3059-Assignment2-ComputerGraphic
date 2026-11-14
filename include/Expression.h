#pragma once
#include <functional>
#include <string>

// A small arithmetic parser. It never executes C++ or shell commands.
class Expression
{
  public:
    explicit Expression(const std::string &text);
    double evaluate(double x, double y) const;

  private:
    std::function<double(double, double)> function;
};
