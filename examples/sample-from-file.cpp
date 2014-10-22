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

// Pour compiler :
// g++ -DSIBUS_THREAD -Wall -I../ -I. -o traceFromFile sibus.cpp receivers/receiver-compil-certif.cpp receivers/receiver-network.cpp receivers/receiver-out.cpp sample-from-file.cpp -lboost_thread -lboost_system
// Utilisation :
// ./traceFromFile trace.txt

#include<sibus/sibus.hh>
#include<sibus/receivers/receiver-out.hh>
#include<sibus/receivers/receiver-network.hh>
#include<sibus/receivers/receiver-compil-certif.hh>
#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<sstream>

using namespace std;

int main(int argc, const char* argv[]) {
  bool bModuloProp = false;
  if (argc < 2) {
    cout << "Usage:" << std::endl;
    cout << " - First argument is optionnal and should be true or false." << std::endl;
    cout << "   If true, the guards use the trace of the search else they use the list of instances." << std::endl;
    cout << " - Second argument provide the name of trace file to read." << endl;
    return 1;
  }

  string fileName;
  if (argc == 3) {
    string flag(argv[1]);
    if (flag == "true") bModuloProp = true;
    fileName = argv[2];
  } else if (argc == 2) {
    fileName = argv[1];
  }

  ProcessSTDOUT pStdout;
//  ProcessNetwork pNetwork;
  ProcessCOMPIL pCompil(bModuloProp);

  SIBus::create();
  SIBus::instance().addReceiver(pStdout);
//  SIBus::instance().addReceiver(pNetwork);
  SIBus::instance().addReceiver(pCompil);

  vector<TVarInfo> binder;
  vector<TVarInfo> varAux;

  ifstream ifs(fileName.c_str());
  string line;
  // Begin reading your stream here
  while (getline(ifs, line)) {
    stringstream ssLine(line);
    string event;
    ssLine >> event;
    if (event == "VAR_AUX") {
      bool isDom = true;
      string word;
      ssLine >> word; // =
      while (ssLine >> word) {
        std::string::size_type posA = word.find('(',0) + 1;
        std::string::size_type posB = posA;
        string quant = word.substr(posA,1);
        posA = posB + 2;
        posB = word.find(',',posA);
        string type = word.substr(posA,posB-posA);
        posA = posB + 1;
        posB = word.find(',',posA);
        if (posB == std::string::npos) { // Not found
          posB = word.find(')',posA);
          isDom = false;
        }
        string name = word.substr(posA,posB-posA);
        TVal dom;
        if (isDom) {
          posA = posB + 1;
          posB = word.find('(',posA);
          string argType = word.substr(posA,posB-posA);
          posA = posB + 1;
          posB = word.find(')',posA);
          string argVal = word.substr(posA,posB-posA);
          if (argType == "bool") {
            dom = TVal((argVal=="0")?false:true);
          } else if (argType == "int") {
            int number;
            std::istringstream ss( argVal );
            ss >> number;
            dom = TVal(number);
          } else if (argType == "interval") {
            std::string::size_type pos = argVal.find(':',0);
            string s1 = argVal.substr(0,pos);
            string s2 = argVal.substr(pos+1,argVal.length()-pos-1);
            int number1, number2;
            std::istringstream ss1( s1 ), ss2( s2 );
            ss1 >> number1;
            ss2 >> number2;
            dom = TVal(number1,number2);
          }
        }

        if (type == "B")
          varAux.push_back(TVarAux(name,TYPE_BOOL,dom));
        else if (type == "I")
          varAux.push_back(TVarAux(name,TYPE_INT,dom));
      }
      SIBus::instance().sendVar(varAux[varAux.size()-1]);
    } else if (event == "VAR_BINDER") {
      bool isDom = true;
      string word;
      ssLine >> word; // =
      while (ssLine >> word) {
        std::string::size_type posA = word.find('(',0) + 1;
        std::string::size_type posB = posA;
        string quant = word.substr(posA,1);
        posA = posB + 2;
        posB = word.find(',',posA);
        string type = word.substr(posA,posB-posA);
        posA = posB + 1;
        posB = word.find(',',posA);
        if (posB == std::string::npos) { // Not found
          posB = word.find(')',posA);
          isDom = false;
        }
        string name = word.substr(posA,posB-posA);
        TVal dom;
        if (isDom) {
          posA = posB + 1;
          posB = word.find('(',posA);
          string argType = word.substr(posA,posB-posA);
          posA = posB + 1;
          posB = word.find(')',posA);
          string argVal = word.substr(posA,posB-posA);
          if (argType == "bool") {
            dom = TVal((argVal=="0")?false:true);
          } else if (argType == "int") {
            int number;
            std::istringstream ss( argVal );
            ss >> number;
            dom = TVal(number);
          } else if (argType == "interval") {
            std::string::size_type pos = argVal.find(':',0);
            string s1 = argVal.substr(0,pos);
            string s2 = argVal.substr(pos+1,argVal.length()-pos-1);
            int number1, number2;
            std::istringstream ss1( s1 ), ss2( s2 );
            ss1 >> number1;
            ss2 >> number2;
            dom = TVal(number1,number2);
          }
        }

        if (type == "B")
          binder.push_back(TVarBinder((quant=="E")?EXISTS:FORALL,name,TYPE_BOOL,dom));
        else if (type == "I")
          binder.push_back(TVarBinder((quant=="E")?EXISTS:FORALL,name,TYPE_INT,dom));
      }
      SIBus::instance().sendVar(binder[binder.size()-1]);
    } else if (event == "VAR_ARRAY_AUX") {
      bool isDom = true;
      string word;
      ssLine >> word; // =
      ssLine >> word; // size(xx) 
      std::string::size_type posA = word.find('(',0);
      std::string::size_type posB = posA + 1;
      string sizeWord = word.substr(0,posA);
      assert(sizeWord == "size");
      posA = posB;
      posB = word.find(')',posA);
      unsigned int size;
      string sizeVal = word.substr(posA,posB-posA);
      std::istringstream ss( sizeVal );
      ss >> size;
      while (ssLine >> word) {
        posA = word.find('(',0) + 1;
        posB = posA;
        string quant = word.substr(posA,1);
        posA = posB + 2;
        posB = word.find(',',posA);
        string type = word.substr(posA,posB-posA);
        posA = posB + 1;
        posB = word.find(',',posA);
        if (posB == std::string::npos) { // Not found
          posB = word.find(')',posA);
          isDom = false;
        }
        string name = word.substr(posA,posB-posA);
        TVal dom;
        if (isDom) {
          posA = posB + 1;
          posB = word.find('(',posA);
          string argType = word.substr(posA,posB-posA);
          posA = posB + 1;
          posB = word.find(')',posA);
          string argVal = word.substr(posA,posB-posA);
          if (argType == "bool") {
            dom = TVal((argVal=="0")?false:true);
          } else if (argType == "int") {
            int number;
            std::istringstream ss( argVal );
            ss >> number;
            dom = TVal(number);
          } else if (argType == "interval") {
            std::string::size_type pos = argVal.find(':',0);
            string s1 = argVal.substr(0,pos);
            string s2 = argVal.substr(pos+1,argVal.length()-pos-1);
            int number1, number2;
            std::istringstream ss1( s1 ), ss2( s2 );
            ss1 >> number1;
            ss2 >> number2;
            dom = TVal(number1,number2);
          }
        }

        if (type == "B") {
          for (unsigned int i=0; i<size; i++)  
            varAux.push_back(TVarAux(name,TYPE_BOOL,dom));
        } else if (type == "I") {
          for (unsigned int i=0; i<size; i++)  
            varAux.push_back(TVarAux(name,TYPE_INT,dom));
        }
      }
      SIBus::instance().sendVarArray(size,varAux[varAux.size()-1]);
    } else if (event == "VAR_ARRAY_BINDER") {
      bool isDom = true;
      string word;
      ssLine >> word; // =
      ssLine >> word; // size(xx) 
      std::string::size_type posA = word.find('(',0);
      std::string::size_type posB = posA + 1;
      string sizeWord = word.substr(0,posA);
      assert(sizeWord == "size");
      posA = posB;
      posB = word.find(')',posA);
      unsigned int size;
      string sizeVal = word.substr(posA,posB-posA);
      std::istringstream ss( sizeVal );
      ss >> size;
      while (ssLine >> word) {
        posA = word.find('(',0) + 1;
        posB = posA;
        string quant = word.substr(posA,1);
        posA = posB + 2;
        posB = word.find(',',posA);
        string type = word.substr(posA,posB-posA);
        posA = posB + 1;
        posB = word.find(',',posA);
        if (posB == std::string::npos) { // Not found
          posB = word.find(')',posA);
          isDom = false;
        }
        string name = word.substr(posA,posB-posA);
        TVal dom;
        if (isDom) {
          posA = posB + 1;
          posB = word.find('(',posA);
          string argType = word.substr(posA,posB-posA);
          posA = posB + 1;
          posB = word.find(')',posA);
          string argVal = word.substr(posA,posB-posA);
          if (argType == "bool") {
            dom = TVal((argVal=="0")?false:true);
          } else if (argType == "int") {
            int number;
            std::istringstream ss( argVal );
            ss >> number;
            dom = TVal(number);
          } else if (argType == "interval") {
            std::string::size_type pos = argVal.find(':',0);
            string s1 = argVal.substr(0,pos);
            string s2 = argVal.substr(pos+1,argVal.length()-pos-1);
            int number1, number2;
            std::istringstream ss1( s1 ), ss2( s2 );
            ss1 >> number1;
            ss2 >> number2;
            dom = TVal(number1,number2);
          }
        }

        if (type == "B") {
          for (unsigned int i=0; i<size; i++)  
            binder.push_back(TVarBinder((quant=="E")?EXISTS:FORALL,name,TYPE_BOOL,dom));
        } else if (type == "I") {
          for (unsigned int i=0; i<size; i++)  
            binder.push_back(TVarBinder((quant=="E")?EXISTS:FORALL,name,TYPE_INT,dom));
        }
      }
      SIBus::instance().sendVarArray(size,binder[binder.size()-1]);
    } else if (event == "CLOSE_MODELING") {
      SIBus::instance().sendCloseModeling();
    } else if (event == "CONSTRAINT") {
        static char s_ComparisonType[][20] = { "_NQ_", "_EQ_", "_LQ_", "_LE_", "_GQ_", "_GR_" };
        static char s_ConstraintTypeName[][20] = { "AND", "OR", "IMP", "XOR", "EQ", "TIMES", "LINEAR", "RE_AND", "RE_OR", "RE_IMP", "RE_XOR", "RE_EQ", "RE_TIMES", "RE_LINEAR", "ELEMENT" };
        int type;
        std::vector<TArg> args;
        string word;
        ssLine >> word; // =
        ssLine >> word; // Constraint name
        for (type=0; word != s_ConstraintTypeName[type]; type++);
        while (ssLine >> word) {
          std::string::size_type posA = word.find('(',0) + 1;
          std::string::size_type posB = word.find(')',posA);
          string argType = word.substr(0,posA-1);
          string argVal = word.substr(posA,posB-posA);
          if (argType == "bool") {
            args.push_back(TArg((argVal=="0")?false:true));
          } else if (argType == "int") {
            int number;
            std::istringstream ss( argVal );
            ss >> number;
            args.push_back(TArg(number));
          } else if (argType == "var") {
            args.push_back(TArg(argVal.c_str()));
          } else if (argType == "interval") {
            int pos = argVal.find(':',0);
            string s1 = argVal.substr(0,pos);
            string s2 = argVal.substr(pos+1,argVal.length()-pos-1);
            int number1, number2;
            std::istringstream ss1( s1 ), ss2( s2 );
            ss1 >> number1;
            ss2 >> number2;
            args.push_back(TArg(number1,number2));
          } else {
            unsigned int cmpType;
            for (cmpType=0; argType != s_ComparisonType[cmpType]; cmpType++);
            args.push_back(TArgCmp(cmpType));
          }
        }
        SIBus::instance().sendConstraint(type,args);
    } else if (event == "FAIL") {
        SIBus::instance().sendFailure();
    } else if (event == "GLOBAL_FAILURE") {
        SIBus::instance().sendGlobalFailure();
    } else if (event == "SUCCESS") {
        SIBus::instance().sendSuccess();
    } else if (event == "CHOICE") {
        string word;
        ssLine >> word; // =
        ssLine >> word;
        std::string::size_type posA = word.find('(',0) + 1;
        std::string::size_type posB = word.find(',',posA);
        string argType = word.substr(0,posA-1);
        string name = word.substr(posA,posB-posA);
        posA = posB + 1;

        int id = 0;
        vector<TVarInfo>::iterator it    = binder.begin();
        vector<TVarInfo>::iterator itEnd = binder.end();
        for ( ; (it != itEnd) && ((*it).name != name) ; ++it ) id++;

        TVal v;
        if (argType == "val") {
          posB = word.find(')',posA);
          string val = word.substr(posA,posB-posA);

          switch (binder[id].t) {
            case TYPE_BOOL:
              v.type = VAL_BOOL;
              if (val == "0") v.val.b = false; 
              else if (val == "1") v.val.b = true; 
              break;
            case TYPE_INT:
              {
                v.type = VAL_INT;
                std::istringstream sval( val );
                sval >> v.val.z;
              }
              break;
          }
        } else if (argType == "interval") {
          v.type = VAL_INTERVAL;
          posB = word.find(',',posA);
          string val1 = word.substr(posA,posB-posA);
          posA = posB + 1;
          posB = word.find(')',posA);
          string val2 = word.substr(posA,posB-posA);
          std::istringstream sval1( val1 );
          sval1 >> v.val.bounds[0];
          std::istringstream sval2( val2 );
          sval2 >> v.val.bounds[1];
        }

        SIBus::instance().sendChoice(id,v);
    } else if (event == "DOMAIN") {
        std::string word;
        unsigned int idVar;
        ssLine >> word; // =

        ssLine >> word;
        std::string::size_type posA = word.find('(',0) + 1;
        std::string::size_type posB = word.find(')',posA);
        std::string argType = word.substr(0,posA-1);
        std::string argVal = word.substr(posA,posB-posA);
        if (argType == "idVar") {
            std::istringstream( argVal ) >> idVar;
        }
        std::vector<TVal> domain;
        switch (binder[idVar].t) {
          case TYPE_BOOL:
            while (ssLine >> word) {
              if (word == "true") {
                domain.push_back(TVal(true));
              } else {
                domain.push_back(TVal(false));
              }
            }
            break;
          case TYPE_INT:
          {
            int i;
            while (ssLine >> word) {
              std::istringstream( word ) >> i;
              domain.push_back(TVal(i));
            }
            break;
          }
        }
        SIBus::instance().sendDomainDescription(idVar,domain);
    } else if (event == "SWAP_ASK") {
        std::string word;
        unsigned int idVar, idVal1, idVal2, i = 0;
        ssLine >> word; // =
        while (ssLine >> word) {
          std::string::size_type posA = word.find('(',0) + 1;
          std::string::size_type posB = word.find(')',posA);
          std::string argType = word.substr(0,posA-1);
          std::string argVal = word.substr(posA,posB-posA);
          if (argType == "idVar") {
            std::istringstream( argVal ) >> idVar;
          } else if (argType == "idVal") {
            if (i == 0)
              std::istringstream( argVal ) >> idVal1;
            else
              std::istringstream( argVal ) >> idVal2;
            i++;
          }
        }
        SIBus::instance().sendSwapAsk(idVar,idVal1,idVal2);
    } else if (event == "SWAP_DONE") {
        std::string word;
        unsigned int idVar, idVal1, idVal2, i = 0;
        ssLine >> word; // =
        while (ssLine >> word) {
          std::string::size_type posA = word.find('(',0) + 1;
          std::string::size_type posB = word.find(')',posA);
          std::string argType = word.substr(0,posA-1);
          std::string argVal = word.substr(posA,posB-posA);
          if (argType == "idVar") {
            std::istringstream( argVal ) >> idVar;
          } else if (argType == "idVal") {
            if (i == 0)
              std::istringstream( argVal ) >> idVal1;
            else
              std::istringstream( argVal ) >> idVal2;
            i++;
          }
        }
        SIBus::instance().sendSwapDone(idVar,idVal1,idVal2);
    } else { // INSTANCE
      TInstance instance;
      string word;
      ssLine >> word; // =
      while (ssLine >> word) {
        std::string::size_type posA = word.find('(',0) + 1;
        std::string::size_type posB = word.find(',',posA);
        string name = word.substr(posA,posB-posA);
        posA = posB + 1;
        posB = word.find(')',posA);
        string val = word.substr(posA,posB-posA);

        int id = 0;
        vector<TVarInfo>::iterator it    = binder.begin();
        vector<TVarInfo>::iterator itEnd = binder.end();
        for ( ; (it != itEnd) && ((*it).name != name) ; ++it ) id++;

        TVal vv;
        if (val == "NOVALUE") vv.type = VAL_NONE; 
        else {
          switch (binder[id].t) {
            case TYPE_BOOL:
              if (val == "0") { vv.type = VAL_BOOL; vv.val.b = false; }
              else if (val == "1") { vv.type = VAL_BOOL; vv.val.b = true; }
              break;
            case TYPE_INT:
              vv.type = VAL_INT;
              std::istringstream( val ) >> vv.val.z;
              break;
          }
        }
        instance.push_back(vv);
      }
      SIBus::instance().sendInstance(instance);
    }
  }
  SIBus::instance().kill();
  return 0;
}

