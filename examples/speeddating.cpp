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
#include <sibus/receivers/receiver-compil-certif.hh>

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

/**
 * \brief Options taking one additional parameter
 */
class SpeedDatingOptions : public Options {
public:
  Driver::UnsignedIntOption _nbM;   /// Number of mens
  Driver::UnsignedIntOption _nbW;   /// Number of wemens
  Driver::UnsignedIntOption _nbMxD; /// Number of maximum delay allowed
  int n; /// Parameter to be given on the command line
  /// Initialize options for example with name \a s
  SpeedDatingOptions(const char* s, unsigned int nbM0, unsigned int nbW0, unsigned int nbMxD0)
    : Options(s),
      _nbM("-nbM","Number of mens", nbM0),
      _nbW("-nbW","Number of womens", nbW0),
      _nbMxD("-nbMxD","Number of maximum delay allowed", nbMxD0)
  {
    add(_nbM);
    add(_nbW);
    add(_nbMxD);
  }
  /// Parse options from arguments \a argv (number is \a argc)
  void parse(int& argc, char* argv[]) {
    Options::parse(argc,argv);
  }
};

void t(Space& ) { std::cout << "THEN" << std::endl; }
void e(Space& ) { std::cout << "ELSE" << std::endl; }

class QCSPSpeedDating : public Script, public QSpaceInfo {
  IntVarArray  debug;

  IntVarArray  rh, rf;
  BoolVarArray ph, pf;

  // Variable letsPlay is true if universal player played right
  BoolVar letsPlay;

  int nbMen;
  int nbWomen;
  int nbMaxDelays;
  int nbMaxSlots;

public:
  QCSPSpeedDating(const SpeedDatingOptions& opt) : Script(), QSpaceInfo()
  {
    debug = IntVarArray(*this,10,-1,100);
    nbMen = opt._nbM.value();
    nbWomen = opt._nbW.value();
    nbMaxDelays = opt._nbMxD.value();
    // START PB DESCRIPTION 
    using namespace Int;

    // Forbidden meet
    bool conflicts[nbMen][nbWomen];
    for (int i=0; i < nbMen; i++)
      for (int j=0; j < nbWomen; j++)
        conflicts[i][j] = false;

    //conflicts[0][0] = true;

    // Computes the maximum number of slots
    int nbRDVMaxM = 0;
    int nbRDVMaxW = 0;
    for (int i=0; i < nbMen; i++) {
      int nbConflicts = 0;
      for (int j=0; j < nbWomen; j++) if (conflicts[i][j]) nbConflicts++;
      nbRDVMaxM = std::max(nbRDVMaxM, nbWomen-nbConflicts);
    }
    for (int i=0; i < nbWomen; i++) {
      int nbConflicts = 0;
      for (int j=0; j < nbMen; j++) if (conflicts[j][i]) nbConflicts++;
      nbRDVMaxW = std::max(nbRDVMaxW, nbMen-nbConflicts);
    }
    nbMaxSlots = std::max(nbRDVMaxM,nbRDVMaxW) + nbMaxDelays;
    std::cout << "NbMaxDelays: " << nbMaxDelays << " - NbMaxSlots: " << nbMaxSlots << std::endl;

    // Create variables
    // Variable letsPlay is true if universal player played right
    letsPlay = BoolVar(*this,0,1);
    // Number of men present at last slot
    IntVar nbPresentMen(*this,0,nbMen);
    // Number of women present at last slot
    IntVar nbPresentWomen(*this,0,nbWomen);
    // ph_i^k is true if man i is here at slot k
    ph = BoolVarArray(*this,nbMen*(nbMaxDelays+1),0,1);
    // pf_i^k is true if woman i is here at slot k
    pf = BoolVarArray(*this,nbWomen*(nbMaxDelays+1),0,1);
    // rh_i^k gives the number of the woman met by men i at slot k
    rh = IntVarArray(*this,nbMen*nbMaxSlots,-1,nbWomen-1);
    // rf_i^k gives the number of the man met by women i at slot k
    rf = IntVarArray(*this,nbWomen*nbMaxSlots,-1,nbMen-1);

    int k=0;
    for (k=0; k < nbMaxDelays; k++) {
      for (int i=0; i < nbMen; i++) {
        std::stringstream ss_p; ss_p << "PH_" << i << "^" << k;
        SIBus::instance().sendVar(TVarBinder(FORALL,ss_p.str(),TYPE_BOOL));
      }
      for (int i=0; i < nbWomen; i++) {
        std::stringstream ss_p; ss_p << "PF_" << i << "^" << k;
        SIBus::instance().sendVar(TVarBinder(FORALL,ss_p.str(),TYPE_BOOL));
      }
      for (int i=0; i < nbMen; i++) {
        std::stringstream ss_p; ss_p << "RH_" << i << "^" << k;
        SIBus::instance().sendVar(TVarBinder(EXISTS,ss_p.str(),TYPE_INT));
      }
      for (int i=0; i < nbWomen; i++) {
        std::stringstream ss_p; ss_p << "RF_" << i << "^" << k;
        SIBus::instance().sendVar(TVarBinder(EXISTS,ss_p.str(),TYPE_INT));
      }
      setForAll(*this, ph.slice(k*nbMen,1,nbMen));
      setForAll(*this, pf.slice(k*nbWomen,1,nbWomen));
      branch(*this, ph.slice(k*nbMen,1,nbMen), INT_VAR_NONE(), INT_VAL_MIN());
      branch(*this, pf.slice(k*nbWomen,1,nbWomen), INT_VAR_NONE(), INT_VAL_MIN());
      branch(*this, rh.slice(k*nbMen,1,nbMen), INT_VAR_NONE(), INT_VALUES_MAX());
      branch(*this, rf.slice(k*nbWomen,1,nbWomen), INT_VAR_NONE(), INT_VALUES_MAX());
    }
    for (int i=0; i < nbMen; i++) {
      std::stringstream ss_p; ss_p << "PH_" << i << "^" << k;
      SIBus::instance().sendVar(TVarBinder(FORALL,ss_p.str(),TYPE_BOOL));
    }
    for (int i=0; i < nbWomen; i++) {
      std::stringstream ss_p; ss_p << "PF_" << i << "^" << k;
      SIBus::instance().sendVar(TVarBinder(FORALL,ss_p.str(),TYPE_BOOL));
    }
    setForAll(*this, ph.slice(k*nbMen,1,nbMen));
    setForAll(*this, pf.slice(k*nbWomen,1,nbWomen));
    branch(*this, ph.slice(k*nbMen,1,nbMen), INT_VAR_NONE(), INT_VAL_MIN());
    branch(*this, pf.slice(k*nbWomen,1,nbWomen), INT_VAR_NONE(), INT_VAL_MIN());
    for (; k < nbMaxSlots; k++) {
      for (int i=0; i < nbMen; i++) {
        std::stringstream ss_p; ss_p << "RH_" << i << "^" << k;
        SIBus::instance().sendVar(TVarBinder(EXISTS,ss_p.str(),TYPE_INT));
      }
      for (int i=0; i < nbWomen; i++) {
        std::stringstream ss_p; ss_p << "RF_" << i << "^" << k;
        SIBus::instance().sendVar(TVarBinder(EXISTS,ss_p.str(),TYPE_INT));
      }
      branch(*this, rh.slice(k*nbMen,1,nbMen), INT_VAR_NONE(), INT_VALUES_MAX());
      branch(*this, rf.slice(k*nbWomen,1,nbWomen), INT_VAR_NONE(), INT_VALUES_MAX());
    }
    SIBus::instance().sendVar(TVarBinder(EXISTS,"letsPlay",TYPE_BOOL));

    // CONSTRAINTS
    // First we post constraints for each of the delayed slot allowed
    BoolVarArgs lLetsPlay;
    for (k=0; k < nbMaxDelays; k++) {
      for (int i=0; i < nbMen; i++) {
        // Check is move is valid
        BoolVar move(*this,0,1);
        lLetsPlay << move;
        // If a man is present at time k, he will stays until the end
        rel(*this, (ph[k*nbMen+i] >> ph[(k+1)*nbMen+i]) == move);

        // Integrity constraints
        for (int j=0; j < nbWomen; j++)
          rel(*this, ph[k*nbMen+i] >> ( (rh[k*nbMen+i] == j) >> (rf[k*nbWomen+j] == i) ));

        // If not presence, then no meet is possible
        rel(*this, (!ph[k*nbMen+i]) >> (rh[k*nbMen+i] == -1) );
      }
      for (int i=0; i < nbWomen; i++) {
        // Check is move is valid
        BoolVar move(*this,0,1);
        lLetsPlay << move;
        // If a woman is present at time k, she will stays until the end
        rel(*this, (pf[k*nbWomen+i] >> pf[(k+1)*nbWomen+i]) == move);

        // Integrity constraints
        for (int j=0; j < nbMen; j++)
          rel(*this, pf[k*nbWomen+i] >> ( (rf[k*nbWomen+i] == j) >> (rh[k*nbMen+j] == i) ));

        // If not presence, then no meet is possible
        rel(*this, (!pf[k*nbWomen+i]) >> (rf[k*nbWomen+i] == -1) );
      }

      // A woman can be met only once at one slot
      for (int j=0; j<nbWomen; j++)
        count(*this, rh.slice(k*nbMen,1,nbMen), j, IRT_LQ, 1);
      // A man can be met only once at one slot
      for (int j=0; j<nbMen; j++)
        count(*this, rf.slice(k*nbWomen,1,nbWomen), j, IRT_LQ, 1);
    }
    // Update letsPlay according to all previous moves
    if (nbMaxDelays > 0) {
      rel(*this, BOT_AND, lLetsPlay, letsPlay);
    } else {
      rel(*this, letsPlay, IRT_EQ, 1);
    }

    // Compute nbPresentMen and nbPresentWomen
    linear(*this, ph.slice(nbMaxDelays*nbMen,1), IRT_EQ, nbPresentMen);
    linear(*this, pf.slice(nbMaxDelays*nbWomen,1), IRT_EQ, nbPresentWomen);

    // Now we post constraints for the slots after the last allowed delay slot
    // here, k start with k == nbMaxDelays
    for (; k < nbMaxSlots; k++) {
      for (int i=0; i < nbMen; i++) {
        // Integrity constraints
        for (int j=0; j < nbWomen; j++)
          rel(*this, ph[nbMaxDelays*nbMen+i] >> ( (rh[k*nbMen+i] == j) >> (rf[k*nbWomen+j] == i) ));

        // If not presence, then no meet is possible
        rel(*this, (!ph[nbMaxDelays*nbMen+i]) >> (rh[k*nbMen+i] == -1) );
      }
      for (int i=0; i < nbWomen; i++) {
        // Integrity constraints
        for (int j=0; j < nbMen; j++)
          rel(*this, pf[nbMaxDelays*nbWomen+i] >> ( (rf[k*nbWomen+i] == j) >> (rh[k*nbMen+j] == i) ));

        // If not presence, then no meet is possible
        rel(*this, (!pf[nbMaxDelays*nbWomen+i]) >> (rf[k*nbWomen+i] == -1) );
      }

      // A woman can be met only once at one slot
      for (int j=0; j<nbWomen; j++)
        count(*this, rh.slice(k*nbMen,1,nbMen), j, IRT_LQ, 1);
      // A man can be met only once at one slot
      for (int j=0; j<nbMen; j++)
        count(*this, rf.slice(k*nbWomen,1,nbWomen), j, IRT_LQ, 1);
    }

    rel(*this, nbPresentMen == debug[1]);
    rel(*this, nbPresentWomen == debug[2]);
    // A man which is present must meet all women (with respect to conflicts)
    for (int i=0; i < nbMen; i++) {
      BoolVarArgs subPh = ph.slice(i,nbMen);
      IntVarArgs subRh = rh.slice(i,nbMen);
      IntVar cPh(*this,0,nbMaxDelays+1); // The number of slot where man i is present
      linear(*this, subPh, IRT_EQ, cPh);

      IntArgs subRh_values = IntArgs::create(nbWomen+1,-1,1);
      IntVarArray subRh_count(*this,nbWomen+1,0,nbMaxSlots);
      // If integrity constraints are satisfied (letsPlay) and man is present and a woman is present too, they must meet each other once
      // otherwise they can't meet each other
      for (int j=0; j<nbWomen; j++ ) {
        if (!conflicts[i][j]) {
          rel(*this, (letsPlay && (cPh > 0) && pf[nbMaxDelays*nbWomen+j]) >> (subRh_count[j+1] == 1));
          rel(*this, (letsPlay && (cPh > 0) && !pf[nbMaxDelays*nbWomen+j]) >> (subRh_count[j+1] == 0));
        } else {
          rel(*this, (letsPlay && (cPh > 0)) >> (subRh_count[j+1] == 0));
        }
      }
      // The number of variables in subRh that are equal to c_j is subRh_count_j
      count(*this, subRh, subRh_count, subRh_values);
    }

    // A Woman which is present must meet all men (with respect to conflicts)
    for (int i=0; i < nbWomen; i++) {
      BoolVarArgs subPf = pf.slice(i,nbWomen);
      IntVarArgs subRf = rf.slice(i,nbWomen);
      IntVar cPf(*this,0,nbMaxDelays+1); // The number of slot where woman i is present
      linear(*this, subPf, IRT_EQ, cPf);

      IntArgs subRf_values = IntArgs::create(nbMen+1,-1,1);
      IntVarArray subRf_count(*this,nbMen+1,0,nbMaxSlots);
      // If integrity constraints are satisfied (letsPlay) and woman is present and a man is present too, they must meet each other once
      // otherwise they can't meet each other
      for (int j=0; j<nbMen; j++ ) {
        if (!conflicts[j][i]) {
          rel(*this, (letsPlay && (cPf > 0) && ph[nbMaxDelays*nbMen+j])>> (subRf_count[j+1] == 1));
          rel(*this, (letsPlay && (cPf > 0) && !ph[nbMaxDelays*nbMen+j])>> (subRf_count[j+1] == 0));
        } else {
          rel(*this, (letsPlay && (cPf > 0)) >> (subRf_count[j+1] == 0));
        }
      }
      // The number of variables in subRf that are equal to c_j is subRf_count_j
      count(*this, subRf, subRf_count, subRf_values);
    }

    // END OF PB DESCRIPTION
    SIBus::instance().sendCloseModeling();
  }

  QCSPSpeedDating(bool share, QCSPSpeedDating& p) : Script(share,p), QSpaceInfo(*this,share,p),
    nbMen(p.nbMen), nbWomen(p.nbWomen), nbMaxDelays(p.nbMaxDelays), nbMaxSlots(p.nbMaxSlots)
  {
    debug.update(*this,share,p.debug);
    letsPlay.update(*this,share,p.letsPlay);
    ph.update(*this,share,p.ph);
    pf.update(*this,share,p.pf);
    rh.update(*this,share,p.rh);
    rf.update(*this,share,p.rf);
  }

  virtual Space* copy(bool share) { return new QCSPSpeedDating(share,*this); }

  void eventNewInstance(void) const {
    std::cout << "DEBUG: ";
    for (int p=0; p<debug.size(); p++) std::cout << debug[p] << " ";
    std::cout << std::endl;

    TInstance instance;
    int k = 0;
    for (k=0; k < nbMaxDelays; k++) {
      for (int i=0; i < nbMen; i++) {
        if (!ph[k*nbMen+i].varimp()->assigned())
          instance.push_back(TVal());
        else
          instance.push_back(TVal(ph[k*nbMen+i].varimp()->one()));
      }
      for (int i=0; i < nbWomen; i++) {
        if (!pf[k*nbWomen+i].varimp()->assigned())
          instance.push_back(TVal());
        else
          instance.push_back(TVal(pf[k*nbWomen+i].varimp()->one()));
      }
      for (int i=0; i < nbMen; i++) {
        if (!rh[k*nbMen+i].varimp()->assigned())
          instance.push_back(TVal());
        else
          instance.push_back(TVal(rh[k*nbMen+i].varimp()->val()));
      }
      for (int i=0; i < nbWomen; i++) {
        if (!rf[k*nbWomen+i].varimp()->assigned())
          instance.push_back(TVal());
        else
          instance.push_back(TVal(rf[k*nbWomen+i].varimp()->val()));
      }
    }
    for (int i=0; i < nbMen; i++) {
      if (!ph[k*nbMen+i].varimp()->assigned())
        instance.push_back(TVal());
      else
        instance.push_back(TVal(ph[k*nbMen+i].varimp()->one()));
    }
    for (int i=0; i < nbWomen; i++) {
      if (!pf[k*nbWomen+i].varimp()->assigned())
        instance.push_back(TVal());
      else
        instance.push_back(TVal(pf[k*nbWomen+i].varimp()->one()));
    }
    for (; k < nbMaxSlots; k++) {
      for (int i=0; i < nbMen; i++) {
        if (!rh[k*nbMen+i].varimp()->assigned())
          instance.push_back(TVal());
        else
          instance.push_back(TVal(rh[k*nbMen+i].varimp()->val()));
      }
      for (int i=0; i < nbWomen; i++) {
        if (!rf[k*nbWomen+i].varimp()->assigned())
          instance.push_back(TVal());
        else
          instance.push_back(TVal(rf[k*nbWomen+i].varimp()->val()));
      }
    }
    if (!letsPlay.varimp()->assigned())
      instance.push_back(TVal());
    else
      instance.push_back(TVal(letsPlay.varimp()->one()));

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
  ProcessCOMPIL pCompil;

  SIBus::create();
  SIBus::instance().addReceiver(pStdout);
  SIBus::instance().addReceiver(pNodeCount);
  SIBus::instance().addReceiver(pCompil);

  SpeedDatingOptions opt("SpeedDating Problem",2,2,1);
  opt.parse(argc,argv);
  Script::run<QCSPSpeedDating,QDFS,SpeedDatingOptions>(opt);

  SIBus::kill();
  return 0;
}

