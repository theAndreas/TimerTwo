// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "timer_two/timer2.h"

#include <avr/interrupt.h>
#include <avr/io.h>

namespace timer_two {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
Timer2 timer2;

}  // namespace timer_two

ISR(TIMER2_OVF_vect) { timer_two::timer2.OnOverflow(); }
