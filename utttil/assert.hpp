
#pragma once

#include <iostream>
#include "utttil/timestamp.hpp"

#define ASSERT_(         left, op, right             ) { auto l=(left); auto r=(right); if (!(l op (r))) { std::cerr << utttil::UTCTimestampISO8601() << " ----------ASSERT FAILED " << __FILE__ << ":" << __LINE__ << " : " << l << ' ' << #op << ' ' << r                << std::endl;         } }
#define ASSERT_ACT(      left, op, right,      action) { auto l=(left); auto r=(right); if (!(l op (r))) { std::cerr << utttil::UTCTimestampISO8601() << " ----------ASSERT FAILED " << __FILE__ << ":" << __LINE__ << " : " << l << ' ' << #op << ' ' << r                << std::endl; action; } }
#define ASSERT_ACT_QUIET(left, op, right,      action) { if (!((left) op (right))) { action; } }
#define ASSERT_MSG(    left, op, right, msg        ) { auto l=(left); auto r=(right); if (!(l op (r))) { std::cerr << utttil::UTCTimestampISO8601() << " ----------ASSERT FAILED " << __FILE__ << ":" << __LINE__ << " : " << l << ' ' << #op << ' ' << r << ", " << msg << std::endl;         } }
#define ASSERT_MSG_ACT(left, op, right, msg, action) { auto l=(left); auto r=(right); if (!(l op (r))) { std::cerr << utttil::UTCTimestampISO8601() << " ----------ASSERT FAILED " << __FILE__ << ":" << __LINE__ << " : " << l << ' ' << #op << ' ' << r << ", " << msg << std::endl; action; } }
#define ASSERTNOT_ACT(    left, op, right,      action) { auto l=(left); auto r=(right); if (l op (r)) { std::cerr << utttil::UTCTimestampISO8601() << " ----------ASSERT FAILED " << __FILE__ << ":" << __LINE__ << " : " << l << ' ' << #op << ' ' << r                << std::endl; action; } }
#define ASSERTNOT_MSG(    left, op, right, msg        ) { auto l=(left); auto r=(right); if (l op (r)) { std::cerr << utttil::UTCTimestampISO8601() << " ----------ASSERT FAILED " << __FILE__ << ":" << __LINE__ << " : " << l << ' ' << #op << ' ' << r << ", " << msg << std::endl;         } }
#define ASSERTNOT_MSG_ACT(left, op, right, msg, action) { auto l=(left); auto r=(right); if (l op (r)) { std::cerr << utttil::UTCTimestampISO8601() << " ----------ASSERT FAILED " << __FILE__ << ":" << __LINE__ << " : " << l << ' ' << #op << ' ' << r << ", " << msg << std::endl; action; } }