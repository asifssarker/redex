/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <gtest/gtest.h>
#include <sstream>
#include <tuple>

#include "FiniteAbstractDomain.h"
#include "ReducedProductAbstractDomain.h"

enum Elements0 { BOT0, TOP0 };
enum Elements1 { BOT1, A, B, TOP1 };
enum Elements2 { BOT2, C, D, E, F, TOP2 };

using Lattice0 = BitVectorLattice<Elements0, 2, std::hash<int>>;
using Lattice1 = BitVectorLattice<Elements1, 4, std::hash<int>>;
using Lattice2 = BitVectorLattice<Elements2, 6, std::hash<int>>;

/*
 *         TOP0
 *          |
 *         BOT0
 */
Lattice0 lattice0({BOT0, TOP0}, {{BOT0, TOP0}});

/*
 *         TOP1
 *        /    \
 *       A      B
 *        \    /
 *         BOT1
 */
Lattice1 lattice1({BOT1, A, B, TOP1},
                  {{BOT1, A}, {BOT1, B}, {A, TOP1}, {B, TOP1}});

/*
 *           TOP2
 *            |
 *            F
 *           / \
 *          D   E
 *           \ /
 *            C
 *            |
 *           BOT2
 */
Lattice2 lattice2({BOT2, C, D, E, F, TOP2},
                  {{BOT2, C}, {C, D}, {C, E}, {D, F}, {E, F}, {F, TOP2}});

using D0 =
    FiniteAbstractDomain<Elements0, Lattice0, Lattice0::Encoding, &lattice0>;
using D1 =
    FiniteAbstractDomain<Elements1, Lattice1, Lattice1::Encoding, &lattice1>;
using D2 =
    FiniteAbstractDomain<Elements2, Lattice2, Lattice2::Encoding, &lattice2>;

class D0xD1xD2 final
    : public ReducedProductAbstractDomain<D0xD1xD2, D0, D1, D2> {
 public:
  // Inherit constructors from ReducedProductAbstractDomain.
  using ReducedProductAbstractDomain::ReducedProductAbstractDomain;

  // For testing purposes we assume that A and C have disjoint denotations.
  void reduce() override {
    if ((get<1>().element() == A) && (get<2>().element() == C)) {
      set_to_bottom();
    }
  }

  static D0xD1xD2 bottom() {
    D0xD1xD2 p;
    p.set_to_bottom();
    return p;
  }

  static D0xD1xD2 top() {
    D0xD1xD2 p;
    p.set_to_top();
    return p;
  }
};

TEST(ReducedProductAbstractDomainTest, latticeOperations) {
  D0xD1xD2 top = D0xD1xD2::top();
  EXPECT_TRUE(top.is_top());
  EXPECT_TRUE(top.equals(D0xD1xD2::top()));
  EXPECT_FALSE(top.is_bottom());
  EXPECT_FALSE(top.equals(D0xD1xD2::bottom()));
  {
    std::ostringstream expected, out;
    expected << "(" << TOP0 << ", " << TOP1 << ", " << TOP2 << ")";
    out << top;
    EXPECT_EQ(expected.str(), out.str());
  }

  D0xD1xD2 bottom = D0xD1xD2::bottom();
  EXPECT_TRUE(bottom.is_bottom());
  EXPECT_TRUE(bottom.equals(D0xD1xD2::bottom()));
  EXPECT_FALSE(bottom.is_top());
  EXPECT_FALSE(bottom.equals(D0xD1xD2::top()));
  {
    std::ostringstream expected, out;
    expected << "(" << BOT0 << ", " << BOT1 << ", " << BOT2 << ")";
    out << bottom;
    EXPECT_EQ(expected.str(), out.str());
  }

  EXPECT_TRUE(bottom.leq(top));
  EXPECT_FALSE(top.leq(bottom));

  D0xD1xD2 tad(make_tuple(D0(TOP0), D1(A), D2(D)));
  D0xD1xD2 tbe(make_tuple(D0(TOP0), D1(B), D2(E)));
  D0xD1xD2 join = tad.join(tbe);
  EXPECT_TRUE(tad.leq(join));
  EXPECT_TRUE(tbe.leq(join));
  EXPECT_FALSE(join.leq(tad));
  EXPECT_FALSE(join.leq(tbe));
  EXPECT_TRUE(join.get<0>().is_top());
  EXPECT_TRUE(join.get<1>().is_top());
  EXPECT_EQ(F, join.get<2>().element());
  EXPECT_TRUE(join.equals(tad.widening(tbe)));

  D0xD1xD2 bottom_meet = tad.meet(tbe);
  EXPECT_TRUE(bottom_meet.is_bottom());
  EXPECT_TRUE(bottom_meet.get<0>().is_bottom());
  EXPECT_TRUE(bottom_meet.get<1>().is_bottom());
  EXPECT_TRUE(bottom_meet.get<2>().is_bottom());

  D0xD1xD2 tte(make_tuple(D0(TOP0), D1(TOP1), D2(E)));
  D0xD1xD2 meet = tad.meet(tte);
  EXPECT_TRUE(meet.leq(tad));
  EXPECT_TRUE(meet.leq(tte));
  EXPECT_FALSE(tad.leq(meet));
  EXPECT_FALSE(tte.leq(meet));
  EXPECT_TRUE(meet.get<0>().is_top());
  EXPECT_EQ(A, meet.get<1>().element());
  EXPECT_EQ(C, meet.get<2>().element());
  EXPECT_TRUE(meet.equals(tad.narrowing(tte)));

  D0xD1xD2 bad(make_tuple(D0(BOT0), D1(A), D2(D)));
  EXPECT_TRUE(bad.is_bottom());
  EXPECT_TRUE(bad.get<0>().is_bottom());
  EXPECT_TRUE(bad.get<1>().is_bottom());
  EXPECT_TRUE(bad.get<2>().is_bottom());

  D0xD1xD2 tac_reduced(make_tuple(D0(BOT0), D1(A), D2(C)));
  EXPECT_TRUE(tac_reduced.is_bottom());
}

TEST(ReducedProductAbstractDomainTest, destructiveOperations) {
  D0xD1xD2 tad(make_tuple(D0(TOP0), D1(A), D2(D)));
  D0xD1xD2 tbe(make_tuple(D0(TOP0), D1(B), D2(E)));
  D0xD1xD2 ttf(make_tuple(D0(TOP0), D1(TOP1), D2(F)));
  D0xD1xD2 x = tad;
  D0xD1xD2 tbe1 = tbe;
  x.join_with(tbe);
  EXPECT_TRUE(x.equals(ttf));
  EXPECT_TRUE(tbe.equals(tbe1));
  x = tad;
  x.widen_with(tbe);
  EXPECT_TRUE(x.equals(ttf));
  EXPECT_TRUE(tbe.equals(tbe1));

  x.apply<1>([](D1* component) { component->set_to_bottom(); });
  EXPECT_TRUE(x.is_bottom());
  x.apply<1>([](D1* component) { component->set_to_top(); });
  EXPECT_TRUE(x.is_bottom());

  x = tad;
  x.apply<1>([](D1* component) { component->set_to_top(); });
  x.apply<2>([](D2* component) { component->set_to_top(); });
  EXPECT_TRUE(x.is_top());

  x = tad;
  x.meet_with(tbe);
  EXPECT_TRUE(x.is_bottom());
  EXPECT_TRUE(tbe.equals(tbe1));
  x = tbe;
  x.meet_with(ttf);
  EXPECT_TRUE(x.equals(tbe));
  x = tbe;
  x.narrow_with(ttf);
  EXPECT_TRUE(x.equals(tbe));
  EXPECT_TRUE(tbe.equals(tbe1));

  x.set_to_top();
  EXPECT_TRUE(x.is_top());
  x.set_to_bottom();
  EXPECT_TRUE(x.is_bottom());
  x.set_to_top();
  EXPECT_TRUE(x.is_top());

  D0xD1xD2 tae(make_tuple(D0(TOP0), D1(A), D2(E)));
  D0xD1xD2 tac = tad.meet(tae);
  EXPECT_TRUE(tac.get<0>().is_top());
  EXPECT_EQ(A, tac.get<1>().element());
  EXPECT_EQ(C, tac.get<2>().element());
  tac.reduce();
  EXPECT_TRUE(tac.is_bottom());
}
