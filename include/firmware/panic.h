/*
 * Copyright 2015 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.0
 */

#ifndef __PANIC_H

#define __PANIC_H

#include <csrtypes.h>
/*! @file panic.h @brief Terminate the application unhappily.
**
**
These functions can be used to panic the application, forcing it to terminate abnormally.
*/
/*!
Panics the application if the value passed is not zero.
*/
#define PanicNotZero(x) PanicNotNull((const void *) (x))
/*!
Allocates memory equal to the size of T and returns a pointer to the memory if successful. If the
memory allocation fails, the application is panicked.
*/
#define PanicUnlessNew(T) (T*)PanicUnlessMalloc(sizeof(T))


/*!
    @brief Panics the application unconditionally.
*/
void Panic(void);

/*!
    @brief Panics the application if the pointer passed is NULL, otherwise returns the pointer.
*/
void *PanicNull(void *);

/*!
    @brief Panics the application if the cond passed is false, otherwise returns the cond.
*/
bool PanicFalse(bool cond);

/*!
    @brief Panics the application if the number passed is 0, otherwise returns the number.
*/
unsigned PanicZero(unsigned number);

/*!
    @brief Panics the application if the pointer passed is not NULL, otherwise returns.
*/
void PanicNotNull(const void *);

/*!
    @brief Allocates sz words and returns a pointer to the memory if successful. If
    the memory allocation fails, the application is panicked.
*/
void *PanicUnlessMalloc(size_t sz);



#endif
