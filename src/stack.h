/***************************************************************************
 * $Id: stack.h,v 1.2 2008/03/22 19:50:24 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: stack.h                                        Part of FieryMUD *
 *  Usage: Parameterized stack                                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_STACK_H
#define __FIERY_STACK_H

/*
 * This file defines two macro-based, type-parameterized implementations
 * of stacks.  They use unnamed structs, and thus can only be used in
 * the scope in which they are declared (and enclosed scopes).  It is
 * relatively unsafe to pass the stacks between functions.  The
 * parameterizing type can be any complete type, such as ints, named
 * structs, and pointers.
 *
 * An example of usage which will print out 0-20 in descending order:
 *
 * #include <stdio.h>
 * int main()
 * {
 *   array_stack(int) stack;
 *   int i;
 *
 *   as_init(stack, 10, 0);
 *   for (i = 0; i < 20; ++i)
 *     as_push(stack, i);
 *   while (!as_empty(stack))
 *     printf("%d\r\n", as_pop(stack));
 * }
 */

/*
 * Parameterized typedef for an array stack.  Pass the parameter type
 * to array_stack.
 */
#define array_stack(type)                                                                                              \
    struct {                                                                                                           \
        type *stack;                                                                                                   \
        int pos;                                                                                                       \
        int size;                                                                                                      \
        type null;                                                                                                     \
    }

/*
 * Initializes and allocates the given pre-declared array stack.  You
 * must specify the initial, nonzero size of the stack (which will
 * automatically grow as items are pushed onto it) and the default
 * null value to return by as_peek when the stack is empty.
 */
#define as_init(var, initsize, nullvalue)                                                                              \
    do {                                                                                                               \
        (var).stack = (typeof(*(var).stack) *)calloc((initsize), sizeof(typeof(*(var).stack)));                        \
        (var).null = (nullvalue);                                                                                      \
        (var).size = (initsize);                                                                                       \
        (var).pos = 0;                                                                                                 \
    } while (0)

/*
 * Pushes a value onto the given array stack.  If the stack is full,
 * it is automatically reallocated with double the capacity.
 */
#define as_push(var, val)                                                                                              \
    do {                                                                                                               \
        if ((var).pos >= (var).size) {                                                                                 \
            (var).size *= 2;                                                                                           \
            (var).stack = (typeof(*(var).stack) *)realloc((var).stack, sizeof(typeof(*(var).stack)) * (var).size);     \
        }                                                                                                              \
        (var).stack[(var).pos] = (val);                                                                                \
        ++(var).pos;                                                                                                   \
    } while (0)

/*
 * Pops a value off the stack.  Precondition: the stack is not empty.
 * Calling as_pop on an empty stack will result in undefined behavior.
 */
#define as_pop(var) ((var).stack[--(var).pos])

/*
 * Returns the size of the given array stack.
 */
#define as_size(var) ((var).pos)

/*
 * Peeks at the value on the top of the stack.  If the stack is empty,
 * returns the null value specified when the stack was initialized.
 */
#define as_peek(var) (as_size(var) ? (var).stack[(var).pos - 1] : (var).null)

/*
 * Determines whether the array stack is empty.
 */
#define as_empty(var) (!(var).pos)

/*
 * Returns the default null value specified when the array stack was
 * initialized.
 */
#define as_null(var) ((var).null)

/*
 * Releases dynamically-allocated resources consumed by this stack and
 * re-initializes it to an empty state with an initial size of 10.
 */
#define as_destroy(var)                                                                                                \
    do {                                                                                                               \
        free((var).stack);                                                                                             \
        (var).stack = (typeof(*(var).stack) *)calloc(10, sizeof(typeof(*(var).stack)));                                \
        (var).pos = 0;                                                                                                 \
        (var).size = 10;                                                                                               \
    } while (0)

/*
 * Parameterized typedef for a linked-node stack.  Pass the parameter
 * type to linked_stack.
 */
#define linked_stack(type)                                                                                             \
    struct {                                                                                                           \
        struct node {                                                                                                  \
            type value;                                                                                                \
            struct node *next;                                                                                         \
        } * stack, *temp;                                                                                              \
        type null;                                                                                                     \
        type last_pop;                                                                                                 \
    }

/*
 * Initializes and allocates the given pre-declared linked-node stack.
 * You must specify the default null value to return by ls_peek when
 * the stack is empty.
 */
#define ls_init(var, default)                                                                                          \
    do {                                                                                                               \
        (var).stack = NULL;                                                                                            \
        (var).null = (default);                                                                                        \
    } while (0)

/*
 * Pushes a value onto the given linked-node stack.
 */
#define ls_push(var, val)                                                                                              \
    do {                                                                                                               \
        (var).temp = (var).stack;                                                                                      \
        (var).stack = (typeof(*(var).stack) *)calloc(1, sizeof(typeof(*(var).stack)));                                 \
        (var).stack->value = (val);                                                                                    \
        (var).stack->next = (var).temp;                                                                                \
    } while (0)

/*
 * Pops a value off the stack and returns it.  Precondition: the stack
 * is not empty.  Calling ls_pop on an empty stack will result in
 * undefined behavior.
 */
#define ls_pop(var)                                                                                                    \
    (((((var).last_pop = (var).stack->value), TRUE) && (((var).temp = (var).stack), TRUE) &&                           \
      (((var).stack = (var).stack->next), TRUE) && (free((var).temp), TRUE))                                           \
         ? (var).last_pop                                                                                              \
         : (var).last_pop)

/*
 * Peeks at the value on the top of the stack.  If the stack is empty,
 * returns the null value specified when the stack was initialized.
 */
#define ls_peek(var) ((var).stack ? (var).stack->value : (var).null)

/*
 * Determines whether the linked-node stack if empty.
 */
#define ls_empty(var) (!(var).stack)

/*
 * Returns the default null value specified when the array stack was
 * initialized.
 */
#define ls_null(var) ((var).null)

/*
 * Releases dynamically-allocated resources consumed by this stack and
 * re-initializes it to an empty state.
 */
#define ls_destroy(var)                                                                                                \
    do {                                                                                                               \
        while ((var).stack) {                                                                                          \
            (var).temp = (var).stack;                                                                                  \
            (var).stack = (var).stack->next;                                                                           \
            free((var).temp);                                                                                          \
        }                                                                                                              \
        (var).stack = NULL;                                                                                            \
    } while (0)

#endif /* __FIERY_STACK_H */
/***************************************************************************
 * $Log: stack.h,v $
 * Revision 1.2  2008/03/22 19:50:24  myc
 * Added as_null and ls_null macros to check the initialized null values
 * on stacks.
 *
 * Revision 1.1  2008/03/22 19:24:43  myc
 * Initial revision
 *
 ***************************************************************************/
