#include <stdio.h>

extern "C" [[maybe_unused]] int _fprintf(FILE* const stream, char const* const format, va_list list)
{
    return fprintf(stream, format, list);
}

FILE iob[] = {*stdin, *stdout, *stderr};

extern "C" FILE* _iob(void)
{ return iob; }
