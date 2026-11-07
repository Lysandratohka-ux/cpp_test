#pragma once
#include "mathsolver.h"
namespace ms { // Conservative interval check: rejection can mean uncertainty.
void validateInterval(Expr, double, double);
}
