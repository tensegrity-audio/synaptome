#pragma once

#ifdef OF_SDK_AVAILABLE
#include <events/ofEvents.h>
#else

constexpr int OF_KEY_RETURN = 13;
constexpr int OF_KEY_BACKSPACE = 8;
constexpr int OF_KEY_ESC = 27;
constexpr int OF_KEY_UP = 1000;
constexpr int OF_KEY_DOWN = 1001;
constexpr int OF_KEY_LEFT = 1006;
constexpr int OF_KEY_RIGHT = 1007;
constexpr int OF_KEY_PAGE_UP = 1002;
constexpr int OF_KEY_PAGE_DOWN = 1003;
constexpr int OF_KEY_HOME = 1004;
constexpr int OF_KEY_END = 1005;
constexpr int OF_KEY_SHIFT = 1008;
constexpr int OF_KEY_CONTROL = 1009;
constexpr int OF_KEY_ALT = 1010;
constexpr int OF_KEY_TAB = 9;
constexpr int OF_KEY_DEL = 1011;
constexpr int OF_KEY_F1 = 2001;
constexpr int OF_KEY_F2 = 2002;
constexpr int OF_KEY_F3 = 2003;
constexpr int OF_KEY_F4 = 2004;
constexpr int OF_KEY_F5 = 2005;
constexpr int OF_KEY_F6 = 2006;
constexpr int OF_KEY_F7 = 2007;
constexpr int OF_KEY_F8 = 2008;
constexpr int OF_KEY_F9 = 2009;
constexpr int OF_KEY_F10 = 2010;
constexpr int OF_KEY_F11 = 2011;
constexpr int OF_KEY_F12 = 2012;

#endif
