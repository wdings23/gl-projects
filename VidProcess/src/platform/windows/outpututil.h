#pragma once

#define OUTPUT( X, ... ) debugOutput( X, __VA_ARGS__ )

void debugOutput( const char* szFormat, ... );