/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Patrick Pekczynski <pekczynski@ps.uni-sb.de>
 *     Guido Tack <tack@gecode.org>
 *
 *  Copyright:
 *     Patrick Pekczynski, 2004
 *     Guido Tack, 2006
 *
 *  Last modified:
 *     $Date$ by $Author$
 *     $Revision$
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

#include "gecode/int/gcc.hh"

namespace Gecode { namespace Int { namespace GCC {

  /**
   * \brief Check whether gcc can be rewritten to distinct
   *
   * If the number of available values equals \f$ |x| \f$,
   * we can rewrite gcc to distinct if every value occurs exactly once.
   * If the number of available values is greater than \f$ |x| \f$,
   * we can rewrite gcc to distinct if every value occurs at least zero times
   * and atmost once, otherwise, there is no rewriting possible.
   */

  template<class Card, bool isView>
  forceinline bool
  check_alldiff(int n, ViewArray<Card>& k){
    if (k.size() == n) {
      for (int i=k.size(); i--;)
        if (k[i].min() != 1 || k[i].max() != 1)
          return false;      
    } else if (k.size() > n && !isView) {
      for (int i=k.size(); i--;)
        if (k[i].min() != 0 || k[i].max() != 1)
          return false;
    } else {
      return false;
    }
    return true;
  }

  /**
   * \brief Compute the cardinality of the union of all variable domains in \a x.
   *
   */
  template <class View>
  forceinline int
  x_card(ViewArray<View>& x, IntConLevel) {
    int n = x.size();
    GECODE_AUTOARRAY(ViewRanges<View>, xrange, n);
    for (int i = n; i--; ){
      ViewRanges<View> iter(x[i]);
      xrange[i] = iter;
    }

    Iter::Ranges::NaryUnion<ViewRanges<View> > drl(&xrange[0], x.size());
    return Iter::Ranges::size(drl);
  }

  /**
   * \brief Initialize the cardinalities for the values in \a k.
   *
   */

  template <class Card, class View>
  forceinline void
  initcard(Space* home, ViewArray<View>& x, ViewArray<Card>& k,
           int lb, int ub, IntConLevel icl) {
    GECODE_AUTOARRAY(ViewRanges<View>, xrange, x.size());
    for (int i = x.size(); i--; ){
      ViewRanges<View> iter(x[i]);
      xrange[i] = iter;
    }

    Iter::Ranges::NaryUnion<ViewRanges<View> > drl(&xrange[0], x.size());
    if (icl == ICL_BND) {
      int fstv = drl.min();
      int lstv = 0;
      for ( ; drl(); ++drl){
        lstv = drl.max();
      }
      for (int i = fstv; i <= lstv; i++) {
        k[i - fstv].init(home, lb, ub, i);
      }
    } else {
      int idx = 0;
      for ( ; drl(); ++drl){
        for (int v = drl.min(); v <= drl.max(); v++){
          k[idx].init(home, lb, ub, v);
          idx++;
        }
      }
    }
  }


  /**
   * \brief Reset already existing cardinalities to zero.
   *
   */

  template <class Card, class View, bool isView>
  forceinline void
  setcard(Space* home, ViewArray<View>& x, ViewArray<Card>& k,
          int xmin, int xmax) {

    int idx = 0;
    for (int v = xmin; v <= xmax; v++) {
      k[idx].card(v);
      k[idx].counter(0);
      idx++;
    }

    bool assigned = true;
    for (int i = x.size(); i--; ) {
      assigned &= x[i].assigned();
    }

    if (assigned) {
      // check validity
      int size = xmax - (xmin - 1);
      GECODE_AUTOARRAY(int, count, size);
      for (int i = size; i--; ) {
        count[i] = 0;
      }
      for (int i = x.size(); i--; ) {
        count[x[i].val() - xmin]++;
      }
      for (int i = k.size(); i--; ) {
        if (k[i].min() > count[i]) {
          home->fail();
        }
      }
    }
  }


  /**
   * \brief Check whether the cardinalities are consistent
   *
   * -# \f$\forall i\in\{0, \dots, |k| - 1\}: max(k_i) \leq |x|\f$
   * -# \f$\neg\exists i\in\{0, \dots, |k| - 1\}: min(k_i) > |x|\f$
   */

  template<class Card, bool isView>
  forceinline ExecStatus
  card_cons(Space* home, ViewArray<Card>& k, int n) {
    // this should be the required min and allowed max
    int smin = 0;
    int smax = 0;
    int m    = k.size();
    for (int i = m; i--; ) {
      int ci = k[i].counter();
      if (ci > k[i].max() ) {
        // more occurrences of k[i].card() than allowed
        return ES_FAILED;
      } else {
        smax += (k[i].max() - ci);
        if (ci < k[i].min()) {
          smin += (k[i].min() - ci);
        }
      }
      if (k[i].min() > n) {
        // cannot satisfy requiremnts for k[i].card()
        return ES_FAILED;
      }
      if (!k[i].assigned()) {
        ModEvent me = k[i].lq(home, n);
        if (me_failed(me)) {
          return ES_FAILED;
        }
      }
    }

    if (n < smin) {
      // not enough variables to satisfy min req
      return ES_FAILED;
    }

    // since we always reduce the variables to the allowed values we always use all values
    if (smax < n) {
      // maximal occurrence for the cardinalities cannot be satisfied
      return ES_FAILED;
    }
    return ES_OK;
  }

  /**
   * \brief Template to post the global cardinality constraint
   *  for the different interfaces.
   *
   */
  template<class Card, bool isView>
  forceinline void
  post_template(Space* home, ViewArray<IntView>& x, ViewArray<Card>& k,
                IntConLevel& icl){
    int  n        = x_card(x, icl);
    bool rewrite  = false;
    rewrite = check_alldiff<Card,isView>(n, k);
    GECODE_ES_FAIL(home, (card_cons<Card, isView>(home, k, x.size())));
    if (rewrite) {
      IntVarArgs xv(x.size());
      for (int i = 0; i < x.size(); i++) {
        IntVar iv(x[i]);
        xv[i] = iv;
      }
      distinct(home, xv, icl);
    } else {
      switch (icl) {
      case ICL_BND: {
        GECODE_ES_FAIL(home, (GCC::Bnd<IntView, Card, isView>::post(home, x, k)));
        break;
      }
      case ICL_DOM: {
        GECODE_ES_FAIL(home, (GCC::Dom<IntView, Card, isView>::post(home, x, k)));
        break;
      }
      default: {
        GECODE_ES_FAIL(home, (GCC::Val<IntView, Card, isView>::post(home, x, k)));
      }
      }
    }
  }

}}

  using namespace Int;
  using namespace Int::GCC;
  using namespace Support;

  // variable cardinality

  void count(Space* home, const  IntVarArgs& x,
             const  IntVarArgs& c, const  IntArgs& v, 
             IntConLevel icl, PropKind) {

    // c = |cards| \forall i\in \{0, \dots, c - 1\}:  cards[i] = \#\{j\in\{0, \dots, |x| - 1\}  | vars_j = values_i\}
    
    // |cards| = |values|
    unsigned int vsize = v.size();
    unsigned int csize = c.size();
    if (vsize != csize)
      throw ArgumentSizeMismatch("Int::count");
    if (x.same())
      throw ArgumentSame("Int::count");
    
    if (home->failed())
      return;

    ViewArray<IntView> xv(home, x);
    
    // valid values for the variables in vars
    IntSet valid(&v[0], vsize);

    // \forall v \not\in values:  \#(v) = 0
    // remove all values from the domains of the variables in vars that are not mentionned in the array \a values   
    for (int i = xv.size(); i--; ) {
      IntSetRanges ir(valid);
      GECODE_ME_FAIL(home, xv[i].inter_r(home, ir));
    }

    linear(home, c, IRT_EQ, xv.size());

    ViewArray<CardView> cv(home, c);

    // set the cardinality
    for (int i = vsize; i--; ) {
      cv[i].card(v[i]);
      cv[i].counter(0);
    }
    GCC::post_template<CardView, true>(home, xv, cv, icl);
  }

  // domain is 0..|cards|- 1
  void count(Space* home, const IntVarArgs& x, const IntVarArgs& c,
             IntConLevel icl, PropKind) {
    IntArgs values(c.size());
    for (int i = c.size(); i--; )
      values[i] = i;
    count(home, x, c, values, icl);
  }

  // constant cards
  void count(Space* home, const IntVarArgs& x,
             const IntSetArgs& c, const IntArgs& v,
             IntConLevel icl, PropKind) {
    unsigned int vsize = v.size();
    unsigned int csize = c.size();
    if (vsize != csize)
      throw ArgumentSizeMismatch("Int::count");
    
    if (x.same())
      throw ArgumentSame("Int::count");

    if (home->failed())
      return;
       
    // c = |cards| \forall i\in \{0, \dots, c - 1\}:  cards[i] = \#\{j\in\{0, \dots, |x| - 1\}  | vars_j = values_i\}
    
    // |cards| = |values|
    ViewArray<IntView> xv(home, x);
    
    // valid values for the variables in vars
    IntSet valid(&v[0], vsize);

    // \forall v \not\in values:  \#(v) = 0
    // remove all values from the domains of the variables in vars that are not mentionned in the array \a values   
    for (int i = xv.size(); i--; ) {
      IntSetRanges ir(valid);
      GECODE_ME_FAIL(home, xv[i].inter_r(home, ir));
    }
    
    bool hole_found = false;
    for (int i = vsize; i--; ) {
      hole_found |= (c[i].size() > 1);
    }

    if (hole_found) {
      // create temporary variables
      ViewArray<CardView> cv(home, vsize);
      IntVarArgs cvargs(vsize);
      for (int i = vsize; i--; ) {
        IntVar card(home, c[i]);
        cvargs[i] = card;
        IntView viewc(card);
        cv[i] = viewc;
      }

      // set the cardinality
      for (int i = vsize; i--; ) {
        cv[i].card(v[i]);
        cv[i].counter(0);
      }

      linear(home, cvargs, IRT_EQ, xv.size());

      GCC::post_template<CardView, true>(home, xv, cv, icl);
    } else {
      // all specified cardinalites are ranges

      ViewArray<OccurBndsView> cv(home, csize);
      // compute number of zero entries
      int z = 0;

      for (int i = csize; i--; ) {
        cv[i].card(v[i]);
        cv[i].counter(0);
        cv[i].min(c[i].min());
        cv[i].max(c[i].max());
        if (cv[i].max() == 0){
          z++;
        }
      }

      // if there are zero entries
      if (z > 0) {
        // reduce the occurences
        ViewArray<OccurBndsView> red(home, cv.size() - z);
        IntArgs rem(z);
        z = 0;
        int c = 0;
        int r = red.size() - 1;
        for (int j = cv.size(); j--;) {
          if (cv[j].max() == 0){
            rem[z] = cv[j].card();
            z++;
          } else {
            red[r]= cv[j];
            r--;
          }
          c++;
        }

        IntSet zero(&rem[0], z);
        int n = xv.size();
        for (int i = n; i--; ) {
          IntSetRanges remzero(zero);
          GECODE_ME_FAIL(home, xv[i].minus_r(home, remzero, false));
        }
        GCC::post_template<OccurBndsView,false>(home, xv, red, icl);
      } else {
        GCC::post_template<OccurBndsView,false>(home, xv, cv, icl);
      }
    }
  }

  // domain is 0..|cards|- 1
  void count(Space* home, const IntVarArgs& x, const IntSetArgs& c,
             IntConLevel icl, PropKind) {
    IntArgs values(c.size());
    for (int i = c.size(); i--; ) 
      values[i] = i;
    count(home, x, c, values, icl);   
  }

  void count(Space* home, const IntVarArgs& x,
             const IntSet& c, const IntArgs& v,
             IntConLevel icl, PropKind) {
    IntSetArgs cards(v.size());
    for (int i = v.size(); i--; )
      cards[i] = c;
    count(home, x, cards, v, icl);    
  }

  GECODE_REGISTER3(Int::GCC::Dom<Gecode::Int::IntView, Gecode::Int::GCC::OccurBndsView, false>);
  GECODE_REGISTER3(Int::GCC::Dom<Gecode::Int::IntView, Gecode::Int::GCC::CardView, true>);
  GECODE_REGISTER3(Int::GCC::Val<Gecode::Int::IntView, Gecode::Int::GCC::OccurBndsView, false>);
  GECODE_REGISTER3(Int::GCC::Val<Gecode::Int::IntView, Gecode::Int::GCC::CardView, true>);
  GECODE_REGISTER4(Int::GCC::BndImp<Gecode::Int::IntView, Gecode::Int::GCC::OccurBndsView, false, false>);
  GECODE_REGISTER4(Int::GCC::BndImp<Gecode::Int::IntView, Gecode::Int::GCC::OccurBndsView, false, true>);
  GECODE_REGISTER4(Int::GCC::BndImp<Gecode::Int::IntView, Gecode::Int::GCC::CardView, true, false>);
  GECODE_REGISTER4(Int::GCC::BndImp<Gecode::Int::IntView, Gecode::Int::GCC::CardView, true, true>);
}


// STATISTICS: int-post
