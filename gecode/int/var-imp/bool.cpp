/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Christian Schulte <schulte@gecode.org>
 *
 *  Copyright:
 *     Christian Schulte, 2006
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gecode/int.hh>

namespace Gecode { namespace Int {

  BoolVarImp BoolVarImp::s_one(1);
  BoolVarImp BoolVarImp::s_zero(0);

  ModEvent
  BoolVarImp::one_none(Space& home) {
    assert(none());
    bits() ^= (NONE ^ ONE);
    assert(one());
    IntDelta d(0);
    return notify(home,ME_BOOL_VAL,d);
  }

  ModEvent
  BoolVarImp::zero_none(Space& home) {
    assert(none());
    bits() ^= (NONE ^ ZERO);
    assert(zero());
    IntDelta d(1);
    return notify(home,ME_BOOL_VAL,d);
  }

  void
  BoolVarImp::subscribe(Space& home, Propagator& p, PropCond,
                        bool schedule) {
    // Subscription can be used with integer propagation conditions,
    // which must be remapped to the single Boolean propagation condition.
    BoolVarImpBase::subscribe(home,p,PC_BOOL_VAL,assigned(),schedule);
  }

  void
  BoolVarImp::reschedule(Space& home, Propagator& p, PropCond) {
    BoolVarImpBase::reschedule(home,p,PC_BOOL_VAL,assigned());
  }

  void
  BoolVarImp::subscribe(Space& home, Advisor& a, bool fail) {
    BoolVarImpBase::subscribe(home,a,assigned(),fail);
  }

}}

// STATISTICS: int-var
