#include <pebble.h>
#include "math.h"

float my_sqrt( float num )
{
  float a, p, e = 0.001, b;

  a = num;
  p = a * a;
  while( p - num >= e )
  {
    b = ( a + ( num / a ) ) / 2;
    a = b;
    p = a * a;
  }
  return a;
}

double Asin(double x)
{
    double x2 = x * x;
    double x3 = x2 * x;
    const double piover2 = 1.5707963267948966;
    const double a = 1.5707288;
    const double b = -0.2121144;
    const double c = 0.0742610;
    const double d = -0.0187293;
    return piover2 - my_sqrt(1 - x) * (a + b * x + c * x2 + d * x3);
}