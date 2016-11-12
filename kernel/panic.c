#include <stdlib.h>
#include <csrtypes.h>
#include <panic.h>

void Panic(void)
{
    exit(1);
}

void *PanicNull(void *_ptr)
{
    if(_ptr == NULL)
        Panic();
    return _ptr;
}

bool PanicFalse(bool cond)
{
    if(!cond)
        Panic();
    return cond;
}

unsigned PanicZero(unsigned number)
{
    if(!number)
        Panic();
    return number;
}

void *PanicUnlessMalloc(size_t sz)
{
    return PanicNull(malloc(sz));
}
