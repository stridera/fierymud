/***************************************************************************
 * $Id: math.c,v 1.3 2009/03/19 23:16:23 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: math.c                                         Part of FieryMUD *
 *  Usage: various mathematical functions                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include <stdlib.h>
#include <math.h>
#include "math.h"

#define NATURAL_E 2.718281828

/*
 * box_muller_transformation
 *
 * A Box-Muller transformation picks two numbers using a uniformly-random
 * random number generator, and then uses those two numbers to come up with
 * a normally-distributed number.  In essence, it is a normally-distributed
 * random number generator (using the standard normal [IE - avg = 0, std = 1])
 *
 * Returns a normally distributed number
 */

double box_muller_transformation() {
  double ur1, ur2, magnitude;
  static double cache;
  static int cached = 0;

  if (cached) {
    cached = 0;
    return cache;
  }

  do {
    ur1 = 2.0 * (random() % 10000) / 10000.0 - 1.0;
    ur2 = 2.0 * (random() % 10000) / 10000.0 - 1.0;
    magnitude = ur1 * ur1 + ur2 * ur2;
  } while (magnitude >= 1.0);

  magnitude = sqrt( (-2.0 * log( magnitude ) ) / magnitude );

  /*
   * It's MAGIC!  Actually, check out:
   * http://www.wikipedia.org/wiki/Box-Muller_transformation
   *
   * This is basically algorithm 2.
   */

  cache = ur2 * magnitude;
  cached = 1;

  return ur1 * magnitude;
}

/*
 *
 * normal_random
 *
 * Generates a random value based on the normal distribution centered at Avg
 * with standard deviation std.  NOTE: Technically, a normal distribution
 * extends to infinity in both directions.  However, to make this plausible in
 * game terms we cap the distribution at 3 standard-deviations in either
 * direction.  IE - if a magic missile is supposed to do an average of 12
 * damage, with standard-deviation of 2, it will never do less than
 * 12 - 2 * 3 = 6, and never do more than 12 + 2*3 = 18.
 *
 * avg - The average value we should give out.
 * std - The standard deviation of the distribution.
 *
 * Returns - a random value.
 *
 */

double normal_random(double avg, double std) {

  /* Get a normally distributed random number.  This is based on the standard
   * normal, so it will be between -3 and 3 99% of the time, the average will
   * be 0, and the standard deviation is 1.
   */
  double val = box_muller_transformation();

  /* We cap this value at +-3.0.  If we don't, an occaisonal call to this could
   * feasibly be 10+, which makes it really hard to deal with the formula.
   * (IE - imagine a magic missile that once every 1000 casts or so does 1000
   * damage...
   */
  while (val > 3.0 || val < -3.0)
    val = box_muller_transformation();

  /*
   * Now, we have a standard-normal number. But we want to translate this into
   * a normal with a given average and standard deviation.  The logic of the
   * following is as follows - the avg term merely shifts the whole to be
   * centered around our arbitrary average.  The std * val is tricky - this
   * may not actually be statisticaly valid, but it works well.  The idea is
   * to scale the deviation from the average based on the supplied standard
   * deviation.
   *
   * Example:  We want avg = 50, std = 10.  We grab a normally distributed
   * value of say, .5.  This translates into 50 + 10 * .5 == 55.  This is
   * one-half standard deviation right of the average of 50.  The original
   * is also one-half standard-deviation to the right of the average (of 0)
   * See? It's happy, and shiny, and good.
   */
  return avg + std * val;
}


unsigned int quick_random() {
  static unsigned int hi = 0xDEADBEEF;
  static unsigned int lo = 0xDEADBEEF ^ 0x49616E42;
  hi = (hi << 16) + (hi >> 16);
  hi += lo;
  lo += hi;
  return hi;
}


double evaluate(struct evaluation_param *eval, double x)
{
  int pos = 0;

  while (eval[pos].type != DONE && x >= eval[pos].x) ++pos;
  if (pos > 0) --pos;

  if (eval[pos].type == LINEAR)
    return linear_evaluation(eval[pos].x, eval[pos].y,
                             eval[pos+1].x, eval[pos+1].y, x);
  else if (eval[pos].type == SQUARED)
    return squared_evaluation(eval[pos].x, eval[pos].y,
                                eval[pos+1].x, eval[pos+1].y, x);
  else if (eval[pos].type == LOG)
    return logarithmic_evaluation(eval[pos].x, eval[pos].y,
                                  eval[pos+1].x, eval[pos+1].y, x);
  else
    return 0;
}

double linear_evaluation(double startx, double starty,
                         double endx, double endy, double x)
{
  double slope;
  double offset;

  slope = (endy - starty) / (endx - startx);
  offset = x - startx;

  return (slope * offset) + starty;
}

double logarithmic_evaluation(double x1, double y1, double x2, double y2, double x)
{
  /* Derivation:
   *
   * Our formula, generically is  f(x) = a log(x + b)
   * We need this to be such that:
   *
   * y1 = log(a * x1 + b)
   * y2 = log(a * x2 + b)
   *
   * E^y1 = a * x1 + b
   * E^y2 = a * x2 + b
   *
   * E^y1 - a * x1 = b
   * E^y2 - a * x2 = b
   *
   * E^y1 - E^y2 =  a * x1 - a * x2
   * E^y1 - E^y2 =  (x1 - x2) * a
   *
   * (E^y1 - E^y2) / (x1 - x2) =  a
   */

  double a = (pow(NATURAL_E, y1) - pow(NATURAL_E, y2)) / (x1 - x2);
  double b = pow(NATURAL_E, y1) - a * x2;

  return log(a * x + b);

}

double squared_evaluation(double x1, double y1, double x2, double y2, double x)
{
  /* Derivation:
   *
   * We're solving for constants to make an equation
   * f(x) = n(x^2-x1^2) + yi, such that
   *
   * f(x1) = y1
   * f(x2) = y2
   *
   * and f is a degree 2 polynomial.
   *
   * For x = x1, the first term drops out, leaving yi, and we're done.
   * For x = x2, we need n such that:
   *
   * y1 = a x1^2 + b
   * y2 = a x2^2 + b
   *
   * Solving for a yields:
   * (y1 - y2) / (x1*x1 - x2*x2)
   *
   * Plugging that back in for a in the first equation, we get the following:
   */

  float a = (y1 - y2) / (x1*x1 - x2*x2);
  float b = y2 - a * x2 * x2;

  return ( a * x * x + b);
}


/* Generate a random number in the interval [from, to] */
int random_number(int from, int to)
{
  /* Error checking in case people call this function incorrectly */

  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
  }

  return ((random() % (to - from + 1)) + from);
}


int roll_dice(int number, int size)
{
  int sum = 0;

  if (size <= 0 || number <= 0)
    return 0;

  while (number-- > 0)
    sum += (random() % size) + 1;

  return sum;
}


int MIN(int a, int b)
{
  return a < b ? a : b;
}


int MAX(int a, int b)
{
  return a > b ? a : b;
}



/*
 * Supply mappings of the natural logarithm, since log() is normally 
 * used by our basic_mud_log function.
 */
#ifdef log
#undef log
#endif

double natural_logarithm(double value) {
  return log(value);
}

double logarithm(double base, double value) {
  return log(value) / log(base);
}

double power(double base, double exponent) {
  return pow(base, exponent);
}

/***************************************************************************
 * $Log: math.c,v $
 * Revision 1.3  2009/03/19 23:16:23  myc
 * Added a new random number generator based on "game rand".
 * It's fast and easy and has better randomness than rand().
 *
 * Revision 1.2  2009/03/09 04:50:38  myc
 * Make box-muller function cache the second generated value.
 *
 * Revision 1.1  2008/02/09 03:06:17  myc
 * Initial revision
 *
 ***************************************************************************/
