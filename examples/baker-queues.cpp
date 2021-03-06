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

#include <sibus/receivers/receiver-out.hh>
#include <sibus/receivers/receiver-nodecount.hh>
#include <sibus/receivers/receiver-message_queue.hh>

using namespace Gecode;

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
    vlist_of() { }
    vlist_of(TComparisonType cmp) {
      this->push_back(TArgCmp(cmp));
    }
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
class BakerOptions : public Options {
public:
  int n; /// Parameter to be given on the command line
  /// Initialize options for example with name \a s
  BakerOptions(const char* s, int n0)
    : Options(s), n(n0) {}
  /// Parse options from arguments \a argv (number is \a argc)
  void parse(int& argc, char* argv[]) {
    Options::parse(argc,argv);
    if (argc < 2)
      return;
    n = atoi(argv[1]);
  }
  /// Print help message
  virtual void help(void) {
    Options::help();
    std::cerr << "\t(unsigned int) default: " << n << std::endl
              << "\t\tValue used to restrict the domain of w1 in order to make the problem harder" << std::endl;
  }
};

class QCSPBaker : public Script, public QSpaceInfo {
  IntVarArray X;

public:
  QCSPBaker(const BakerOptions& opt) : Script(), QSpaceInfo()
  {
    // DEBUT DESCRIPTION PB
    std::cout << "Loading problem" << std::endl;
    using namespace Int;
    SIBus::instance().sendVar(TVarBinder(EXISTS,"w1",TYPE_INT,TVal(1,40)));
    SIBus::instance().sendVar(TVarBinder(EXISTS,"w2",TYPE_INT,TVal(1,40)));
    SIBus::instance().sendVar(TVarBinder(EXISTS,"w3",TYPE_INT,TVal(1,40)));
    SIBus::instance().sendVar(TVarBinder(EXISTS,"w4",TYPE_INT,TVal(1,40)));
    SIBus::instance().sendVar(TVarBinder(FORALL,"f",TYPE_INT,TVal(1,40)));
    SIBus::instance().sendVar(TVarBinder(EXISTS,"c1",TYPE_INT,TVal(-1,1)));
    SIBus::instance().sendVar(TVarBinder(EXISTS,"c2",TYPE_INT,TVal(-1,1)));
    SIBus::instance().sendVar(TVarBinder(EXISTS,"c3",TYPE_INT,TVal(-1,1)));
    SIBus::instance().sendVar(TVarBinder(EXISTS,"c4",TYPE_INT,TVal(-1,1)));
    SIBus::instance().sendVar(TVarAux("o1",TYPE_INT));
    SIBus::instance().sendVar(TVarAux("o2",TYPE_INT));
    SIBus::instance().sendVar(TVarAux("o3",TYPE_INT));
    SIBus::instance().sendVar(TVarAux("o4",TYPE_INT));

    IntVarArgs w(*this,4,1,40);
    IntVar f(*this,1,40);
    setForAll(*this, f);
    IntVarArgs c(*this,4,-1,1);
    IntVarArgs vaX;
    vaX << w << f << c;
    X = IntVarArray(*this, vaX);

    IntVar o1(*this,-40,40), o2(*this,-40,40), o3(*this,-40,40), o4(*this,-40,40);
    rel(*this, w[0], IRT_GR, opt.n);
    rel(*this, w[0] * c[0] == o1);
    rel(*this, w[1] * c[1] == o2);
    rel(*this, w[2] * c[2] == o3);
    rel(*this, w[3] * c[3] == o4);
    rel(*this, o1 + o2 + o3 + o4 == f);

    SIBus::instance().sendConstraint(TIMES, vlist_of<TArg>(CMP_EQ)("w1")("c1")("o1"));
    SIBus::instance().sendConstraint(TIMES, vlist_of<TArg>(CMP_EQ)("w2")("c2")("o2"));
    SIBus::instance().sendConstraint(TIMES, vlist_of<TArg>(CMP_EQ)("w3")("c3")("o3"));
    SIBus::instance().sendConstraint(TIMES, vlist_of<TArg>(CMP_EQ)("w4")("c4")("o4"));
    SIBus::instance().sendConstraint(LINEAR, vlist_of<TArg>(CMP_EQ)(1)("o1")(1)("o2")(1)("o3")(1)("o4")(-1)("f")(0));

    branch(*this, X, INT_VAR_NONE(), INT_VALUES_MIN());

    // FIN DESCRIPTION PB
    SIBus::instance().sendCloseModeling();
  }

  QCSPBaker(bool share, QCSPBaker& p) : Script(share,p), QSpaceInfo(*this,share,p)
  {
    X.update(*this,share,p.X);
  }

  virtual Space* copy(bool share) { return new QCSPBaker(share,*this); }

  void eventNewInstance(void) const {
    TInstance instance;
    for (int i=0; i<X.size(); i++)
    {
      if (!X[i].varimp()->assigned())
        instance.push_back(TVal());
      else
        instance.push_back(TVal(X[i].varimp()->val()));
    }
    SIBus::instance().sendInstance(instance);
  }

  void print(std::ostream& os) const {
    strategyPrint(os);
  }
};

int main(int argc, char* argv[])
{
  ProcessSTDOUT pStdout;
  ProcessNodeCount pNodeCount;
  MessageQueueReceiver mqr;

  SIBus::create();
  SIBus::instance().addReceiver(pStdout);
  SIBus::instance().addReceiver(pNodeCount);
  SIBus::instance().addReceiver(mqr);

  BakerOptions opt("Baker Problem",0);
  opt.parse(argc,argv);
  Script::run<QCSPBaker,QDFS,BakerOptions>(opt);

  SIBus::kill();
  return 0;
}

