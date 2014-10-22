/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Vincent Barichard <Vincent.Barichard@univ-angers.fr>
 *
 *  Copyright:
 *     Vincent Barichard, 2013
 *
 *  Last modified:
 *     $Date$ by $Author$
 *     $Revision$
 *
 *  This file is part of Quacode:
 *     http://quacode.barichard.com
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

#include <iostream>
#include <vector>

#include <quacode/qspaceinfo.hh>
#include <gecode/minimodel.hh>
#include <gecode/driver.hh>
#include <gecode/int/bool.hh>

#include <sibus/receivers/receiver-out.hh>
#include <sibus/receivers/receiver-nodecount.hh>

using namespace Gecode;
using namespace Int;
using namespace Int::Bool;

class ControlSubsumeGroupObject : public BasicGroupObject {
  protected:
    unsigned int _size;  //< Maximum number of propagators in group
    Propagator** _p;     //< Propagators in group

    /// Resize to at least \a n + 1 propagators
    void resize(Space& home, int n) {
      int m = std::max(n+1, (3*(int)_size)/2);
      _p = home.realloc<Propagator*>(_p,n_prop,static_cast<unsigned int>(m));
      _size = m;
    }
  public:
    /// Initialize and cloning
    ControlSubsumeGroupObject(Home& home)
      : BasicGroupObject(home), _size(0), _p(NULL) {}
    ControlSubsumeGroupObject(Space& home, bool share, ControlSubsumeGroupObject& co)
      : BasicGroupObject(home,share,co), _size(co.n_prop), _p(NULL) {
        if (_size > 0) _p = home.alloc<Propagator*>(_size);
      }
    virtual Actor* copy(Space& home, bool share) {
      return new (home) ControlSubsumeGroupObject(home,share,*this);
    }
    virtual void schedule(Space&home, Propagator& p, ModEvent )  {
      PropagatorQueue::enqueue(home,p);
    }
    virtual void notice(Space& home, Propagator& prop) {
      if (n_prop == _size)
        resize(home,n_prop+1);
      _p[n_prop++] = &prop;
    }
    virtual void ignore(Space&, Propagator& prop) {
      for (unsigned int i=0; i<n_prop; i++) {
        if (_p[i] == &prop) {
          std::swap(_p[i],_p[n_prop-1]);
          n_prop--;
          return;
        }
      }
    }
    virtual size_t dispose(Space& home) {
      int n = n_prop;
      n_prop = 0; // To prevent ignore() from removing the propagator
      for (int i=n; i--;) {
        PropagatorQueue::dispose(home,*_p[i]);
      }
      if (_p != NULL) {
        home.free<Propagator*>(_p, _size);
        _p = NULL;
      }
      return sizeof(*this);
    }
    void migrate(Space& home, Group ag) {
      int n = n_prop;
      n_prop = 0; // To prevent ignore() from removing the propagator
      for (int i=n; i--;) {
        _p[i]->migrate(ag);
        ag.notice(home,*_p[i]);
      }
      if (_p != NULL) {
        home.free<Propagator*>(_p, _size);
        _p = NULL;
      }
    }
};

class ControlSubsumeGroup : public Group {
public:
  ControlSubsumeGroup(void) : Group() {}
  ControlSubsumeGroup(Home home) : Group() {
    go(new (home) ControlSubsumeGroupObject(home));
  }
  ControlSubsumeGroup(const ControlSubsumeGroup& c) : Group(c) {}
  bool active(void) { return go() != NULL; }
  size_t dispose(Space& home) {
    assert(go());
    size_t s = static_cast<ControlSubsumeGroupObject*>(go())->dispose(home);
    go(NULL);
    return s;
  }
  void migrate(Space& home, Group ag) {
    assert(go());
    static_cast<ControlSubsumeGroupObject*>(go())->migrate(home,ag);
    go(NULL);
  }
};

template<class BV>
class GpSSNaryOrTrue : public BinaryPropagator<BV,PC_BOOL_VAL> {
  protected:
    using Propagator::group;
    using BinaryPropagator<BV,PC_BOOL_VAL>::x0;
    using BinaryPropagator<BV,PC_BOOL_VAL>::x1;
    /// Views not yet subscribed to
    ViewArray<BV> x;
    /// Groups
    ControlSubsumeGroup* _g;
    ControlSubsumeGroup _gx0;
    ControlSubsumeGroup _gx1;
    /// Update subscription
    ExecStatus resubscribe(Space& home, BV& x0, ControlSubsumeGroup& gx0, BV x1, ControlSubsumeGroup& gx1) {
      if (x0.zero()) {
        gx0.migrate(home,group);
        int n = x.size();
        for (int i=n; i--; )
          if (x[i].one()) {
            for (int j=n-1 ; j>i ; j--) _g[j].dispose(home);
            _g[i].migrate(home,group);
            for (int j=i ; j-- ; ) _g[j].dispose(home);
            return home.ES_SUBSUMED(*this);
          } else if (x[i].zero()) {
            x[i] = x[--n];
            _g[i].migrate(home,group);
            _g[i] = _g[n];
          } else {
            // Move to x0 and subscribe
            x0=x[i]; x[i]=x[--n];
            gx0=_g[i]; _g[i]=_g[n];
            x.size(n);
            x0.subscribe(home,*this,PC_BOOL_VAL,false);
            return ES_FIX;
          }
        // All views have been assigned!
        GECODE_ME_CHECK(x1.one(home));
        gx1.migrate(home,group);
        return home.ES_SUBSUMED(*this);
      }
      return ES_FIX;
    }
    /// Constructor for posting
    GpSSNaryOrTrue(Home home, ViewArray<BV>& b, std::vector<ControlSubsumeGroup>& g)
      : BinaryPropagator<BV,PC_BOOL_VAL>(home,b[0],b[1]), x(b), _g(NULL) {
        _gx0 = g[0];
        _gx1 = g[1];

        assert(x.size() > 2);
        _g = static_cast<Space&>(home).alloc<ControlSubsumeGroup>(x.size() - 2);
        for (int i=x.size()-2 ; i-- ; ) {
          _g[i] = g[i+2];
        }
        x.drop_fst(2);
      }
    /// Constructor for cloning \a p
    GpSSNaryOrTrue(Space& home, bool share, GpSSNaryOrTrue<BV>& p)
      : BinaryPropagator<BV,PC_BOOL_VAL>(home,share,p), _g(NULL) {
        x.update(home,share,p.x);
        _gx0.update(home,share,p._gx0);
        _gx1.update(home,share,p._gx1);
        if (x.size() > 0)_g = home.alloc<ControlSubsumeGroup>(x.size());
        for (int i=x.size(); i-- ; ) _g[i].update(home,share,p._g[i]);
      }
  public:
    /// Copy propagator during cloning
    virtual Actor* copy(Space& home, bool share) {
// Untested and probably wrong because of cloning of local object doesn't occur
// in that order.
//      int n = x.size();
//      if (n > 0) {
//        // Eliminate all zeros and find a one
//        for (int i=n; i--; )
//          if (x[i].one()) {
//            // Only keep the one
//            x[0]=x[i]; x.size(1);
//            _gx0.dispose(home);
//            _gx1.dispose(home);
//            for (int j=n-1 ; j>i ; j--) _g[j].dispose(home);
//            _g[i].migrate(home,group);
//            for (int j=i ; j-- ; ) _g[j].dispose(home);
//            return new (home) OrTrueSubsumed<BV>(home,share,*this,x0,x1);
//          } else if (x[i].zero()) {
//            // Eliminate the zero
//            x[i]=x[--n];
//            _g[i].dispose(home);
//            _g[i]=_g[n];
//          }
//        x.size(n);
//      }
      return new (home) GpSSNaryOrTrue<BV>(home,share,*this);
    }
    /// Cost function (defined as low unary)
    virtual PropCost cost(const Space&, const ModEventDelta&) const {
      return PropCost::binary(PropCost::LO);
    }
    /// Perform propagation
    virtual ExecStatus propagate(Space& home, const ModEventDelta&) {
      if (x0.one()) {
        _gx0.migrate(home,group);
        _gx1.dispose(home);
        for (int i=x.size() ; i-- ; ) _g[i].dispose(home);
        return home.ES_SUBSUMED(*this);
      }
      if (x1.one()) {
        _gx1.migrate(home,group);
        _gx0.dispose(home);
        for (int i=x.size() ; i-- ; ) _g[i].dispose(home);
        return home.ES_SUBSUMED(*this);
      }
      GECODE_ES_CHECK(resubscribe(home,x0,_gx0,x1,_gx1));
      GECODE_ES_CHECK(resubscribe(home,x1,_gx1,x0,_gx0));
      return ES_FIX;
    }
    /// Post propagator \f$ \bigvee_{i=0}^{|b|-1} b_i = 0\f$
    static  ExecStatus post(Home home, ViewArray<BV>& b, std::vector<ControlSubsumeGroup>& gb) {
      for (int i=b.size(); i--; )
        if (b[i].one()) {
          gb[i].migrate(home,home.group());
          for (int i=b.size() ; i-- ; ) gb[i].dispose(home);
          return ES_OK;
        }
        else if (b[i].zero()) {
          b.move_lst(i);
          gb[i].migrate(home,home.group());
          gb[i] = gb[gb.size()-1];
        }
      if (b.size() == 0)
        return ES_FAILED;
      if (b.size() == 1) {
        GECODE_ME_CHECK(b[0].one(home));
        gb[0].migrate(home,home.group());
      } else {
        (void) new (home) GpSSNaryOrTrue(home,b,gb);
      }
      return ES_OK;
    }
    /// Delete propagator and return its size
    virtual size_t dispose(Space& home) {
      (void) BinaryPropagator<BV,PC_BOOL_VAL>::dispose(home);
      if (_gx0.active()) _gx0.dispose(home);
      if (_gx1.active()) _gx1.dispose(home);
      for (int i=x.size() ; i-- ; ) if (_g[i].active()) _g[i].dispose(home);
      return sizeof(*this);
    }
};


template<class BVA, class BVB, class BVC>
class GpOr : public BoolTernary<BVA,BVB,BVC> {
  protected:
    using Propagator::group;
    using BoolTernary<BVA,BVB,BVC>::x0;
    using BoolTernary<BVA,BVB,BVC>::x1;
    using BoolTernary<BVA,BVB,BVC>::x2;
    ControlSubsumeGroup _gx0;
    ControlSubsumeGroup _gx1;
    /// Constructor for posting
    GpOr(Home home, BVA b0, BVB b1, BVC b2, ControlSubsumeGroup gx0, ControlSubsumeGroup gx1)
      : BoolTernary<BVA,BVB,BVC>(home,b0,b1,b2) {
        _gx0 = gx0;
        _gx1 = gx1;
      }
    /// Constructor for cloning \a p
    GpOr(Space& home, bool share, GpOr& p)
      : BoolTernary<BVA,BVB,BVC>(home,share,p) {
        _gx0.update(home,share,p._gx0);
        _gx1.update(home,share,p._gx1);
      }
  public:
    /// Copy propagator during cloning
    virtual Actor* copy(Space& home, bool share) {
      return new (home) GpOr<BVA,BVB,BVC>(home,share,*this);
    }
    /// Perform propagation
    virtual ExecStatus propagate(Space& home, const ModEventDelta&) {
#define GECODE_INT_STATUS(S0,S1,S2) \
  ((BVA::S0<<(2*BVA::BITS))|(BVB::S1<<(1*BVB::BITS))|(BVC::S2<<(0*BVC::BITS)))
      switch ((x0.status() << (2*BVA::BITS)) | (x1.status() << (1*BVB::BITS)) |
          (x2.status() << (0*BVC::BITS))) {
        case GECODE_INT_STATUS(NONE,NONE,NONE):
          GECODE_NEVER;
        case GECODE_INT_STATUS(NONE,NONE,ZERO):
          GECODE_ME_CHECK(x0.zero_none(home));
          _gx0.migrate(home,group);
          GECODE_ME_CHECK(x1.zero_none(home));
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(NONE,NONE,ONE):
          return ES_FIX;
        case GECODE_INT_STATUS(NONE,ZERO,NONE):
          switch (bool_test(x0,x2)) {
            //ICIcase BT_SAME: _gx0.migrate(home,group); _gx1.migrate(home,group); return home.ES_SUBSUMED(*this);
            case BT_SAME: _gx0.migrate(home,group); _gx1.dispose(home); return home.ES_SUBSUMED(*this);
            case BT_COMP: return ES_FAILED;
            case BT_NONE: return ES_FIX;
            default: GECODE_NEVER;
          }
          GECODE_NEVER;
        case GECODE_INT_STATUS(NONE,ZERO,ZERO):
          GECODE_ME_CHECK(x0.zero_none(home));
          _gx0.migrate(home,group);
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(NONE,ZERO,ONE):
          GECODE_ME_CHECK(x0.one_none(home));
          _gx0.migrate(home,group);
          //ICI _gx1.migrate(home,group);
          _gx1.dispose(home);
          break;
        case GECODE_INT_STATUS(NONE,ONE,NONE):
          GECODE_ME_CHECK(x2.one_none(home));
          _gx0.dispose(home);
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(NONE,ONE,ZERO):
          return ES_FAILED;
        case GECODE_INT_STATUS(NONE,ONE,ONE):
          _gx0.dispose(home);
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(ZERO,NONE,NONE):
          switch (bool_test(x1,x2)) {
            //ICIcase BT_SAME: _gx0.migrate(home,group); _gx1.migrate(home,group); return home.ES_SUBSUMED(*this);
            case BT_SAME: _gx0.dispose(home); _gx1.migrate(home,group); return home.ES_SUBSUMED(*this);
            case BT_COMP: return ES_FAILED;
            case BT_NONE: return ES_FIX;
            default: GECODE_NEVER;
          }
          GECODE_NEVER;
        case GECODE_INT_STATUS(ZERO,NONE,ZERO):
          GECODE_ME_CHECK(x1.zero_none(home));
          _gx0.migrate(home,group);
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(ZERO,NONE,ONE):
          GECODE_ME_CHECK(x1.one_none(home));
          _gx0.dispose(home);
          //ICI_gx0.migrate(home,group);
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(ZERO,ZERO,NONE):
          GECODE_ME_CHECK(x2.zero_none(home));
          _gx0.migrate(home,group);
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(ZERO,ZERO,ZERO):
          _gx0.migrate(home,group);
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(ZERO,ZERO,ONE):
          return ES_FAILED;
        case GECODE_INT_STATUS(ZERO,ONE,NONE):
          GECODE_ME_CHECK(x2.one_none(home));
          _gx0.dispose(home);
          //ICI_gx0.migrate(home,group);
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(ZERO,ONE,ZERO):
          return ES_FAILED;
        case GECODE_INT_STATUS(ZERO,ONE,ONE):
          //ICI_gx0.migrate(home,group);
          _gx0.dispose(home);
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(ONE,NONE,NONE):
          GECODE_ME_CHECK(x2.one_none(home));
          _gx0.migrate(home,group);
          _gx1.dispose(home);
          break;
        case GECODE_INT_STATUS(ONE,NONE,ZERO):
          return ES_FAILED;
        case GECODE_INT_STATUS(ONE,NONE,ONE):
          _gx0.migrate(home,group);
          _gx1.dispose(home);
          break;
        case GECODE_INT_STATUS(ONE,ZERO,NONE):
          GECODE_ME_CHECK(x2.one_none(home));
          _gx0.migrate(home,group);
          //ICI_gx1.migrate(home,group);
          _gx1.dispose(home);
          break;
        case GECODE_INT_STATUS(ONE,ZERO,ZERO):
          return ES_FAILED;
        case GECODE_INT_STATUS(ONE,ZERO,ONE):
          _gx0.migrate(home,group);
          //ICI_gx1.migrate(home,group);
          _gx1.dispose(home);
          break;
        case GECODE_INT_STATUS(ONE,ONE,NONE):
          GECODE_ME_CHECK(x2.one_none(home));
          _gx0.migrate(home,group);
          _gx1.migrate(home,group);
          break;
        case GECODE_INT_STATUS(ONE,ONE,ZERO):
          return ES_FAILED;
        case GECODE_INT_STATUS(ONE,ONE,ONE):
          _gx0.migrate(home,group);
          _gx1.migrate(home,group);
          break;
        default:
          GECODE_NEVER;
      }
      return home.ES_SUBSUMED(*this);
#undef GECODE_INT_STATUS
    }
    /// Post propagator \f$ b_0 \lor b_1 = b_2 \f$
    static  ExecStatus post(Home home, BVA b0, BVB b1, BVC b2, ControlSubsumeGroup gx0, ControlSubsumeGroup gx1) {
      if (b2.zero()) {
        GECODE_ME_CHECK(b0.zero(home));
        gx0.migrate(home,home.group());
        GECODE_ME_CHECK(b1.zero(home));
        gx1.migrate(home,home.group());
      } else {
        if (b0.one() || b1.one()) {
          if (b0.none()) gx0.dispose(home);
          else if (b0.zero()) gx0.dispose(home); //ICI
          else gx0.migrate(home,home.group());
          if (b1.none()) gx1.dispose(home);
          else if (b1.zero()) gx1.dispose(home); //ICI
          else gx1.migrate(home,home.group());
          GECODE_ME_CHECK(b2.one(home));
        } else {
          (void) new (home) GpOr<BVA,BVB,BVC>(home,b0,b1,b2,gx0,gx1);
        }
      }
      return ES_OK;
    }
    /// Delete propagator and return its size
    virtual size_t dispose(Space& home) {
      (void) BoolTernary<BVA,BVB,BVC>::dispose(home);
      if (_gx0.active()) _gx0.dispose(home);
      if (_gx1.active()) _gx1.dispose(home);
      return sizeof(*this);
    }
};

#ifdef GECODE_HAS_GIST
namespace Gecode { namespace Driver {
  /// Specialization for QDFS
  template<typename S>
  class GistEngine<QDFS<S> > {
  public:
    static void explore(S* root, const Gist::Options& opt) {
      (void) Gist::explore(root, false, opt);
    }
  };
}}
#endif

  // Struct utile pour simplifier la syntaxe de l'initialisation des vectors par des listes
  template <typename T>
  struct vlist_of : public std::vector<T> {
    vlist_of(const T& t) {
      (*this)(t);
    }
    vlist_of& operator()(const T& t) {
      this->push_back(t);
      return *this;
    }
    vlist_of& operator()(int t1, int t2) {
      this->push_back(TArg(t1,t2));
      return *this;
    }
  };

/**
 * \brief Options taking one additional parameter
 */
class DisjunctionOptions : public Options {
public:
  int n; /// Parameter to be given on the command line
  /// Problem name
  Gecode::Driver::StringOption _problem;
  /// Uses group or not
  Gecode::Driver::BoolOption _group;
  /// Initialize options for example with name \a s
  DisjunctionOptions(const char* s, int n0)
    : Options(s), n(n0),
      _problem("-problem","Name of the problem",1),
      _group("-group","Use constructive or based on groups",false)
  {
    _problem.add(1,"NimFibo","NimFibo problem (QCSP).");
    _problem.add(2,"AntiChain","AntiChain problem from article of Nightingale on WatchOr with subsume controller.");
    add(_problem);
    add(_group);
  }
  /// Parse options from arguments \a argv (number is \a argc)
  void parse(int& argc, char* argv[]) {
    Options::parse(argc,argv);
    if (argc < 2)
      return;
    n = atoi(argv[1]);
  }
  /// Return problem number
  int problem(void) const {
    return _problem.value();
  }
  /// Return true if problem will use constructive or
  bool group(void) const {
    return _group.value();
  }
  /// Print help message
  virtual void help(void) {
    Options::help();
    std::cerr << "\t(unsigned int) default: " << n << std::endl
              << "\t\t number of initial matchs" << std::endl;
  }
};

/// Succeed the space
static void gf_success(Space& home) {
  Space::Branchers b(home);
  while (b()) {
    BrancherHandle bh(b.brancher());
    ++b;
    bh.kill(home);
  }
}

/// Dummy function
static void gf_dummy(Space& ) { }

/// Adding cut
static void cut(Space& home, const BoolExpr& expr) {
  BoolVar o(home,0,1);
  rel(home, o == expr);
  when(home, o, &gf_success, &gf_dummy);
}

class QCSPNimFibo : public Script, public QSpaceInfo {
  IntVarArray X;

public:
  QCSPNimFibo(const DisjunctionOptions& opt) : Script(), QSpaceInfo()
  {
    // DEBUT DESCRIPTION PB
    std::cout << "Loading problem NimFibo" << std::endl;
    using namespace Int;
    // Number of matches
    int NMatchs = opt.n;
    int nvars = (NMatchs%2)?NMatchs:NMatchs+1;

    if (opt.group())
    {
      IntVarArgs x;
      SIBus::instance().sendVar(TVarBinder(EXISTS,"x1",INT,TVal(1,NMatchs-1)));
      X = IntVarArray(*this,nvars,1,NMatchs-1);
      x << X[0];


      ControlSubsumeGroup g_oim1;
      BoolVar o_im1(*this,1,1);
      BoolExpr cutExpr1(BoolVar(*this,1,1));
      BoolExpr cutExpr2(BoolVar(*this,1,1));
      branch(*this, X[0], INT_VAR_NONE(), INT_VAL_MIN());

      for (int i=1; i < nvars; i++) {
        ControlSubsumeGroup g_ri(*this);
        ControlSubsumeGroup g_oi(*this);
        BoolVar oi(*this,0,1), ri(*this,0,1);

        x << X[i];

        //rel(Home(*this,tg_ri), ((X[i] <= (2*X[i-1])) && (sum(x) <= NMatchs)) == ri);
        BoolVar o1(*this,0,1), o2(*this,0,1);
        ControlSubsumeGroup g_o1(*this), g_o2(*this);
        rel(Home(*this,g_o1), X[i], IRT_LQ, expr(Home(*this,g_o1), 2*X[i-1]), o1);
        linear(Home(*this,g_o2), x, IRT_LQ, NMatchs, o2);
        // rel(Home(*this,g_ri), o1, BOT_AND, o2, ri);
        Space& home = *this;
        GECODE_ES_FAIL((GpOr<NegBoolView,NegBoolView,NegBoolView>::post(Home(home,g_ri),NegBoolView(o1),NegBoolView(o2),NegBoolView(ri),g_o1,g_o2)));

        if (i%2) {
          // Universal Player
          std::stringstream ss_y; ss_y << "y" << i/2 + 1;
          SIBus::instance().sendVar(TVarBinder(FORALL,ss_y.str(),INT,TVal(1,NMatchs-1)));
          setForAll(X[i]);
          Space& home = *this;
          GECODE_ES_FAIL((GpOr<NegBoolView,BoolView,BoolView>::post(Home(home,g_oim1),NegBoolView(ri),oi,o_im1,g_ri,g_oi)));
          branch(*this, X[i], INT_VAR_NONE(), INT_VAL_MIN());
        } else {
          // Existantial Player
          std::stringstream ss_x; ss_x << "x" << i/2 + 1;
          SIBus::instance().sendVar(TVarBinder(EXISTS,ss_x.str(),INT,TVal(1,NMatchs-1)));
          Space& home = *this;
          GECODE_ES_FAIL((GpOr<NegBoolView,NegBoolView,NegBoolView>::post(Home(home,g_oim1),NegBoolView(ri),NegBoolView(oi),NegBoolView(o_im1),g_ri,g_oi)));
          branch(*this, X[i], INT_VAR_NONE(), INT_VAL_MIN());
        }
        o_im1 = oi;
        g_oim1 = g_oi;
      }
    } else {
      IntVarArgs x;
      SIBus::instance().sendVar(TVarBinder(EXISTS,"x1",INT,TVal(1,NMatchs-1)));
      X = IntVarArray(*this,nvars,1,NMatchs-1);
      x << X[0];

      BoolVar o_im1(*this,1,1);
      BoolExpr cutExpr1(BoolVar(*this,1,1));
      BoolExpr cutExpr2(BoolVar(*this,1,1));
      branch<EXISTS>(*this, X[0], INT_VAR_NONE(), INT_VAL_MIN());

      for (int i=1; i < nvars; i++) {
        BoolVar oi(*this,0,1), o1(*this,0,1), o2(*this,0,1);

        x << X[i];

        rel(*this, X[i], IRT_LQ, expr(*this, 2*X[i-1]), o1);
        linear(*this, x, IRT_LQ, NMatchs, o2);

        if (i%2) {
          // Universal Player
          std::stringstream ss_y; ss_y << "y" << i/2 + 1;
          SIBus::instance().sendVar(TVarBinder(FORALL,ss_y.str(),INT,TVal(1,NMatchs-1)));
          rel(*this, o_im1 == (!o1 || !o2 || oi)); // <-> rel(*this, o_im1 == ((o1 && o2) >> oi));
          // Adding Cut
          cut(*this, cutExpr1 && cutExpr2 && !(o1 && o2));  // Universal player can't play
          branch<FORALL>(*this, X[i], INT_VAR_NONE(), INT_VAL_MIN());
        } else {
          // Existantial Player
          std::stringstream ss_x; ss_x << "x" << i/2 + 1;
          SIBus::instance().sendVar(TVarBinder(EXISTS,ss_x.str(),INT,TVal(1,NMatchs-1)));
          rel(*this, o_im1 != (!o1 || !o2 || !oi)); // <-> rel(*this, o_im1 == (o1 && o2 && oi));
          branch<EXISTS>(*this, X[i], INT_VAR_NONE(), INT_VAL_MIN());
        }
        cutExpr1 = cutExpr1 && o1;
        cutExpr2 = o2;
        o_im1 = oi;
      }
    }

    // FIN DESCRIPTION PB
    SIBus::instance().sendCloseModeling();
  }

  QCSPNimFibo(bool share, QCSPNimFibo& p) : Script(share,p), QSpaceInfo(*this,share,p)
  {
    X.update(*this,share,p.X);
  }

  virtual Space* copy(bool share) { return new QCSPNimFibo(share,*this); }

  void eventNewInstance(void) const {
    TInstance instance;
    for (int i=0; i<X.size(); i++)
    {
      if (!X[i].varimp()->assigned())
        instance.push_back(TVal(X[i].varimp()->min(),X[i].varimp()->max()));
      else
        instance.push_back(TVal(X[i].varimp()->val()));
    }
    SIBus::instance().sendInstance(instance);
  }

  void print(std::ostream& os) const {
    strategyPrint(os);
  }
};

class AntiChain : public Script {
  IntVarArray X;

public:
  AntiChain(const DisjunctionOptions& opt) : Script()
  {
    // DEBUT DESCRIPTION PB
    std::cout << "Loading problem AntiChain" << std::endl;
    using namespace Int;
    int nbArray = 4;
    int sizeArray = 4;
    int sizeDomain = 3;

    if (opt.group())
    {
      X = IntVarArray(*this, sizeArray*nbArray, 0, sizeDomain-1);

      for (int i=0; i < nbArray-1; i++) {
        for (int j=i+1; j < nbArray; j++) {
          BoolVarArgs bva, bvb;
          std::vector<ControlSubsumeGroup> gaa, gba;
          ControlSubsumeGroup ga(*this), gb(*this);
          for (int k=0; k < sizeArray; k++) {
            BoolVar ba(*this, 0, 1), bb(*this, 0, 1);
            ControlSubsumeGroup ga(*this), gb(*this);
            bva << ba; gaa.push_back(ga);
            bvb << bb; gba.push_back(gb);
            rel(Home(*this,ga), X[i*sizeArray+k], IRT_LE, X[j*sizeArray+k], ba);
            rel(Home(*this,gb), X[i*sizeArray+k], IRT_GR, X[j*sizeArray+k], bb);
          }
          Space& home = *this;
          //rel(*this, BOT_OR, bva, 1);
          ViewArray<BoolView> va_bva(home,bva);
          GECODE_ES_FAIL((GpSSNaryOrTrue<BoolView>::post(*this,va_bva,gaa)));
          //rel(*this, BOT_OR, bvb, 1);
          ViewArray<BoolView> va_bvb(home,bvb);
          GECODE_ES_FAIL((GpSSNaryOrTrue<BoolView>::post(*this,va_bvb,gba)));
        }
      }
      IntVarArgs branchA;
      for (int k=0; k < sizeArray; k++)
        for (int i=0; i < nbArray; i++)
          branchA << X[i*sizeArray+k];
      branch(*this, branchA, INT_VAR_NONE(), INT_VALUES_MIN());
    } else {
      X = IntVarArray(*this, sizeArray*nbArray, 0, sizeDomain-1);

      for (int i=0; i < nbArray-1; i++) {
        for (int j=i+1; j < nbArray; j++) {
          BoolVarArgs bva, bvb;
          for (int k=0; k < sizeArray; k++) {
            BoolVar ba(*this, 0, 1), bb(*this, 0, 1);
            bva << ba;
            bvb << bb;
            rel(*this, X[i*sizeArray+k], IRT_LE, X[j*sizeArray+k], ba);
            rel(*this, X[i*sizeArray+k], IRT_GR, X[j*sizeArray+k], bb);
          }
          rel(*this, BOT_OR, bva, 1);
          rel(*this, BOT_OR, bvb, 1);
        }
      }
      IntVarArgs branchA;
      for (int k=0; k < sizeArray; k++)
        for (int i=0; i < nbArray; i++)
          branchA << X[i*sizeArray+k];
      branch(*this, branchA, INT_VAR_NONE(), INT_VALUES_MIN());
    }

    // FIN DESCRIPTION PB
  }

  AntiChain(bool share, AntiChain& p) : Script(share,p)
  {
    X.update(*this,share,p.X);
  }

  virtual Space* copy(bool share) { return new AntiChain(share,*this); }
};

int main(int argc, char* argv[])
{
  ProcessSTDOUT pStdout;
  ProcessNodeCount pNodeCount;

  SIBus::create();
  SIBus::instance().addReceiver(pStdout);
//  SIBus::instance().addReceiver(pNodeCount);

  DisjunctionOptions opt("Disjunction Sample Problem",2);
  opt.parse(argc,argv);
  if (opt.problem() == 1)
    Script::run<QCSPNimFibo,QDFS,DisjunctionOptions>(opt);
  else if (opt.problem() == 2)
    Script::run<AntiChain,DFS,DisjunctionOptions>(opt);

  SIBus::kill();
  return 0;
}

