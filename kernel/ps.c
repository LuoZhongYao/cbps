#include <csrtypes.h>
#include <ps.h>
#include <string.h>
#include <debug.h>

void PsFlood(void)
{
    NOT_IMPL();
}

u16 PsFreeCount(u16 len)
{
    NOT_IMPL();
    return len;
}

u16 PsFullRetrieve(u16 key, void *buff, u16 words)
{
    NOT_IMPL();
    return words;
}

u16 PsRetrieve(u16 key, void *buff, u16 words)
{
    NOT_IMPL();
    memset(buff, 0, words);
    return words;
}

u16 PsStore(u16 key, const void *buff, u16 words)
{
    NOT_IMPL();
    return words;
}

void PsuConfigure()
{
    NOT_IMPL();
}
void PsuGetVregEn()
{
    NOT_IMPL();
}
