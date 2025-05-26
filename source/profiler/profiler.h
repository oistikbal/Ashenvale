#pragma once

#include <Windows.h>
#include <WinPixEventRuntime/pix3.h>

#define CONCAT_INTERNAL(x, y) x##y
#define CONCAT(x, y) CONCAT_INTERNAL(x, y)

#define PIX_SCOPED_EVENT(label) ashenvale::profiler::pix_scoped_event CONCAT(pix_event_, __COUNTER__)(label);

namespace ashenvale::profiler
{
    struct pix_scoped_event
    {
        pix_scoped_event(const char* label) 
        {
            PIXBeginEvent(0, label);
        }

        ~pix_scoped_event() 
        {
            PIXEndEvent();
        }
    };
}