/***************************************************************************
 *   File: math.h                                         Part of FieryMUD *
 *  Usage: various mathematical functions                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef _FIERY_MATH_
#define _FIERY_MATH_

/*
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
 */

double normal_random(double avg, double std);

#define DONE (-1)
#define LINEAR 0
#define LOG 1
#define SQUARED 2

struct evaluation_param {
    double x;
    double y;
    int type;
};

double evaluate(struct evaluation_param *eval, double x);
double linear_evaluation(double startx, double starty, double endx, double endy, double x);
double logarithmic_evaluation(double x1, double y1, double x2, double y2, double x);
double squared_evaluation(double x1, double y1, double x2, double y2, double x);
double natural_logarithm(double value);
double logarithm(double base, double value);
double power(double base, double exponent);

int random_number(int from, int to);
#define number(from, to) random_number(from, to)
int roll_dice(int number, int size);
#define dice(number, size) roll_dice(number, size)

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
#define LIMIT(a, b, c) MIN(MAX((a), (b)), (c))

#endif
