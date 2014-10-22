/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *    Vincent Barichard <Vincent.Barichard@univ-angers.fr>
 *
 *  Copyright:
 *    Vincent Barichard, 2013
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
#include <sstream>
#include <sibus/receivers/receiver-out.hh>

#define OSTREAM std::cerr

void ProcessSTDOUT::eventVar(const TVarInfo& var) {
  if (var.inBinder) {
    binder.push_back(var);
    OSTREAM << "VAR_BINDER       =";
  } else {
    varAux.push_back(var);
    OSTREAM << "VAR_AUX          =";
  }
  OSTREAM << " var(" << ((var.q==EXISTS)?"E":"F") << "," << ((var.t==TYPE_BOOL)?"B":"I") << "," << var.name;
  switch (var.dom.type) {
    case VAL_NONE:
      break;
    case VAL_BOOL:
      OSTREAM << ",bool(" << var.dom.val.b << ")";
      break;
    case VAL_INT:
      OSTREAM << ",int(" << var.dom.val.z << ")";
      break;
    case VAL_INTERVAL:
      OSTREAM << ",interval(" << var.dom.val.bounds[0] << ":" << var.dom.val.bounds[1] << ")";
      break;
  }
  OSTREAM << ")" << std::endl;
}

void ProcessSTDOUT::eventVarArray(unsigned int size, const TVarInfo& var) {
  if (var.inBinder) {
    for (unsigned int i=0; i<size; i++) {
      TVarInfo v(var);
      std::stringstream ss_v; ss_v << i;
      v.name += ss_v.str();
      binder.push_back(v);
    }
    OSTREAM << "VAR_ARRAY_BINDER =";
  } else {
    for (unsigned int i=0; i<size; i++) {
      TVarInfo v(var);
      std::stringstream ss_v; ss_v << i;
      v.name += ss_v.str();
      varAux.push_back(v);
    }
    OSTREAM << "VAR_ARRAY_AUX    =";
  }
  OSTREAM << " size(" << size << ")";
  OSTREAM << " var(" << ((var.q==EXISTS)?"E":"F") << "," << ((var.t==TYPE_BOOL)?"B":"I") << "," << var.name;
  switch (var.dom.type) {
    case VAL_NONE:
      break;
    case VAL_BOOL:
      OSTREAM << ",bool(" << var.dom.val.b << ")";
      break;
    case VAL_INT:
      OSTREAM << ",int(" << var.dom.val.z << ")";
      break;
    case VAL_INTERVAL:
      OSTREAM << ",interval(" << var.dom.val.bounds[0] << ":" << var.dom.val.bounds[1] << ")";
      break;
  }
  OSTREAM << ")" << std::endl;
}

void ProcessSTDOUT::eventCloseModeling() {
  OSTREAM << "CLOSE_MODELING" << std::endl; 
}

void ProcessSTDOUT::eventConstraint(TConstraintType type, const std::vector<TArg>& args) {
  // Names of constraints (useful to convert TConstraintType to string.
  static char s_ComparisonType[][20] = { "_NQ_", "_EQ_", "_LQ_", "_LE_", "_GQ_", "_GR_" };
  static char s_ConstraintTypeName[][20] = { "AND", "OR", "IMP", "XOR", "EQ", "TIMES", "LINEAR", "RE_AND", "RE_OR", "RE_IMP", "RE_XOR", "RE_EQ", "RE_TIMES", "RE_LINEAR", "ELEMENT" };

  OSTREAM << "CONSTRAINT       ="; 
  OSTREAM << " " << s_ConstraintTypeName[type];

  std::vector<TArg>::const_iterator it =    args.begin();
  std::vector<TArg>::const_iterator itEnd = args.end();
  for ( ; it != itEnd ; ++it ) {
    switch ((*it).type) {
      case VAL_NONE:
        break;
      case VAL_BOOL:
        OSTREAM << " bool(" << (*it).val.b << ")";
        break;
      case VAL_INT:
        OSTREAM << " int(" << (*it).val.z << ")";
        break;
      case VAL_VAR:
        OSTREAM << " var(" << (*it).name << ")";
        break;
      case VAL_INTERVAL:
        OSTREAM << " interval(" << (*it).val.bounds[0] << ";" << (*it).val.bounds[1] << ")";
        break;
      case VAL_CMP:
        OSTREAM << " " << s_ComparisonType[(*it).cmp];
        break;
    }
  }
  OSTREAM << std::endl;
}

void ProcessSTDOUT::eventChoice(int idx, TVal val) {
  OSTREAM << "CHOICE           =";
  if (val.type == VAL_INTERVAL)
    OSTREAM << " interval(" << binder[idx].name << "," << val.val.bounds[0] << "," << val.val.bounds[1] << ")" << std::endl;
  else if (val.type == VAL_BOOL)
    OSTREAM << " val(" << binder[idx].name << "," << val.val.b << ")" << std::endl;
  else
    OSTREAM << " val(" << binder[idx].name << "," << val.val.z << ")" << std::endl;
}

void ProcessSTDOUT::eventFailure() {
  OSTREAM << "FAIL" << std::endl;
}

void ProcessSTDOUT::eventGlobalFailure() {
  OSTREAM << "GLOBAL_FAILURE" << std::endl;
}

void ProcessSTDOUT::eventInstance(const TInstance& instance) {
  OSTREAM << "INSTANCE         =";
  for (unsigned int i=0; i < instance.size(); i++)
    if (instance[i].type == VAL_NONE)
      OSTREAM << " val(" << binder[i].name << ",NOVALUE)";
    else if (instance[i].type == VAL_INTERVAL) 
      OSTREAM << " interval(" << binder[i].name << "," << instance[i].val.bounds[0] << "," << instance[i].val.bounds[1] << ")";
    else if (instance[i].type == VAL_BOOL)
      OSTREAM << " val(" << binder[i].name << "," << instance[i].val.b << ")";
    else
      OSTREAM << " val(" << binder[i].name << "," << instance[i].val.z << ")";
  OSTREAM << std::endl;
}

void ProcessSTDOUT::eventSuccess() {
  OSTREAM << "SUCCESS" << std::endl;
}

void ProcessSTDOUT::eventDomainDescription(unsigned int idVar, const std::vector<TVal>& domain) {
  OSTREAM << "DOMAIN           = idVar(" << idVar << ")";
  std::vector<TVal>::const_iterator it =    domain.begin();
  std::vector<TVal>::const_iterator itEnd = domain.end();
  for ( ; it != itEnd ; ++it ) {
    if (binder[idVar].t == TYPE_BOOL)
      OSTREAM << " " << ((*it).val.b?"true":"false");
    else
      OSTREAM << " " << (*it).val.z;
  }
  OSTREAM << std::endl;
}

void ProcessSTDOUT::eventSwapRequest(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) {
  OSTREAM << "SWAP_ASK         = idVar(" <<  idVar << ") idVal(" << idVal1 << ") idVal(" << idVal2 << ")" << std::endl;
}

void ProcessSTDOUT::eventSwapDone(unsigned int idVar, unsigned int idVal1, unsigned int idVal2) {
  OSTREAM << "SWAP_DONE        = idVar(" <<  idVar << ") idVal(" << idVal1 << ") idVal(" << idVal2 << ")" << std::endl;
}

ProcessSTDOUT::~ProcessSTDOUT() { }

