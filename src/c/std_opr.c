#include "header/std_opr.h"

int div(int a, int b) {
    return a / b;
}

int mod(int a, int n) {
   return a - n*(a/n);
}
