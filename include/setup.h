/**
 * @file setup.h 
 * @author Adrian Del Maestro
 * @date 04.06.2011
 *
 * @brief Parses command line input and sets up the details of the simulation.
 */

#ifndef SETUP_H 
#define SETUP_H

#include "common.h"
#include "factory.h"
#include <boost/program_options.hpp>
#include <boost/any.hpp>

/** The three possible states of a parameter */
enum ParamState {UNSET,DEFAULTED,SET};

namespace po = boost::program_options;
namespace pt = boost::property_tree;

class Container; 
class PotentialBase;
class WaveFunctionBase;
class ActionBase;
class Path;
class LookupTable;
class MoveBase;
class EstimatorBase;

ostream& operator<<(ostream&, const vector<string>&);
string getList (const vector<string> &, char sep=' ');
bool isStringInVector(const string, const vector<string>&);

// ========================================================================  
// Parameters Class
// ========================================================================  
/** 
 * Simulation Parameters.
 *
 * @see http://stackoverflow.com/questions/7174781/boost-program-options-notifier-for-options-with-no-value
 * in the future to change default value of no value options to be bool
 *
 */
class Parameters {
    public: 
        Parameters() {};
        
        /** Add a parameter to the map without a default value */
        template <typename Ttype> 
           void add(string,string,string);

        /** Add a parameter to the map with a default value */
        template <typename Ttype> 
            void add(string,string,string, const Ttype);

        /** Set a parameter from a value */
        template<typename Ttype>
            void set(const string&, const Ttype);

        /** Set a parameter from an xml node*/
        template<typename Ttype>
            void set(const string&, const pt::ptree &);

        /** Insert options into the options_description data structure to be
         * read from the command line. */
        void setupCommandLine(boost::ptr_map<string,po::options_description>&);

        /** Update parameters from command line */
        void update(int,char*[],po::options_description&);

        /** Update parameters from an xml file */
        void update(const pt::ptree &);

        /** print out the parameter map */
        void print();

        /** Parameters() returns the full map */
        const po::variables_map& operator()() const {return params;}
        po::variables_map& operator()() { return params;}

        /** Parmaters(key) returns the existence of a parameter */
        const bool operator()(const string & key) const { return !(state.at(key) == UNSET); }

        /** Parmaters[key] returns the variable value at key */
        const po::variable_value & operator[](const string & key) const {return params[key];}
        po::variable_value & operator[](const string & key) {return params.at(key);}

    private:

        using Extractor = map<string, void(*)(const po::variable_value &)>;

        po::variables_map params;                 ///< Parameter map
        map<string, const type_info&> type;       ///< Parameter types
        map<string,string> pClass;                ///< Class of the parameters
        map<string,ParamState> state;             ///< State of a parameter
        map<string,string> shortName;             ///< Short name of a parameter (optional)
        map<string,string> helpMessage;           ///< Help message for a parameter
        Extractor extract;                        ///< Stores how to print each element

        /** Split a string at a delimeter */
        vector<string> split(const string &, char);

        template <typename Ttype> 
            po::typed_value<Ttype, char>* initValue(const string &);

        /** Get a node from an xml file */
        bool getNode(const pt::ptree &xmlParams, pt::ptree &node, const string key) {
            boost::optional<const pt::ptree&> child = xmlParams.get_child_optional(key);
            if (!child)
                return false;
            else
                node = *child; 
            return true;
        }
};

/**************************************************************************//**
 * Initialize a parameter in the map
 *
 * @param _label longName,shortName where shortName is optional
 * @param _helpMessage a command line help message
 * @param _pClass the class of parameter
******************************************************************************/
template <typename Ttype> 
void Parameters::add(string _label,string _helpMessage,string _pClass) {

    /* We break the label up at a possible comma and store the short-name in a
     * map*/
    vector<string> label = split(_label,',');
    string key = label[0];
    if (label.size() > 1) 
        shortName.emplace(key,label[1]);
    else 
        shortName.emplace(key,"");

    /* Add the help message to a map */
    helpMessage.emplace(key,_helpMessage);

    /* Add the parameter class to a map */
    pClass.emplace(key,_pClass);

    /* Add the type to the appropriate map */
    type.emplace(pair<string,const type_info&>(key,typeid(Ttype)));

    /* Stores the function signature to print an element */
    extract.emplace(pair<string,void(*)(const po::variable_value &)>
            (key,[](const po::variable_value& v) {cout << v.as<Ttype>();}));

    /* Initialize the parameter state */
    state.emplace(key,UNSET);

    /* Insert an empty parameter */
    params.insert(make_pair(key, po::variable_value())); 

    /* update the parameters map */
    po::notify(params);
}

/**************************************************************************//**
 * Initialize a parameter in the map with a default value
 *
 * @param _label longName,shortName where shortName is optional
 * @param _helpMessage a command line help message
 * @param _pClass the class of parameter
 * @param _defaultValue the default value of the parameter
******************************************************************************/
template <typename Ttype> 
void Parameters::add(string _label,string _helpMessage,string _pClass, 
        const Ttype _defaultValue) {

    /* We perform a standard initialization */
    add<Ttype>(_label,_helpMessage,_pClass);

    /* Get the long name */
    vector<string> label = split(_label,',');
    string key = label[0];

    /* Insert the default value into the parameter map */
    set<Ttype>(key,_defaultValue);

    /* Set the parameter state */
    state.at(key) = DEFAULTED;
}

/**************************************************************************//**
 * Set the value of a parameter in the map from a value.
 *
 * @param key name of the parameter
 * @param val value of the parameter
******************************************************************************/
template<typename Ttype>
void Parameters::set(const string& key, const Ttype val) {
    if (params.count(key)) {
        po::variables_map::iterator it(params.find(key));
        po::variable_value & v(it->second);
        v.value() = val;
    }
    else {
        params.insert(std::make_pair(key, po::variable_value(val, false)));
        po::notify(params);

        /* insert the type */
        type.emplace(pair<string,const type_info&>(key,typeid(Ttype)));

        /* Stores the function signature to print an element */
        extract.emplace(pair<string,void(*)(const po::variable_value &)>
                (key,[](const po::variable_value& v) {cout << v.as<Ttype>();}));
    }

    state[key] = SET;
}

/**************************************************************************//**
 * Set the value of a parameter in the map from an xml node
 *
 * @param key name of the parameter
 * @param xml node
******************************************************************************/
template<typename Ttype>
void Parameters::set(const string& key, const pt::ptree &xml) {
    if (!xml.count(key))
        return;

    /* command line overrides xml file */
    if (state[key] == UNSET || state[key] == DEFAULTED) {
        set<Ttype>(key,xml.get<Ttype>(key));
        state[key] = SET;
    }
}

/**************************************************************************//**
 * Get a value string to initialize the command line options.
 *
******************************************************************************/
template <typename Ttype> 
po::typed_value<Ttype, char>* Parameters::initValue(const string& key) {
    if (state[key] == DEFAULTED)
        return po::value<Ttype>()->default_value(params[key].as<Ttype>());
    else
        return po::value<Ttype>();
}

// ========================================================================  
// Setup Class
// ========================================================================  
/** 
 * Setup the simulation.
 *
 * This class uses boost::program_options to parse command line input 
 * specifying all possible simulation options.  In the future I would like
 * to implement the use of configuration files for static options as well.
 * We then parse the options, checking for correctness, and define all
 * simulation constants.  We setup the container and potential pointers
 * and finally provide a method to output all the parameters to a log
 * file on disk.
 * 
 * @see http://www.boost.org/doc/libs/release/doc/html/program_options.html
 */
class Setup {
    public:
        Setup();

        /* Get the options from the command line */
        void getOptions(int, char*[]);
        /* Parse the options and check for errors (needs to be more complete) */
        bool parseOptions();

        /* Setup the worldlines */
        bool worldlines();

        /* Setup the physical simulation cell */
        Container *cell();

        /* Setup the simulation constants */
        void setConstants();
        
        /* Setup the communicator */
        void communicator();

        /* Define the random seed */
        uint32 seed(const uint32);
        
        /* Output all options to disk */
        void outputOptions(int, char*[], const uint32, const Container*, const iVec&);

        /* Setup the interaction potential */
        PotentialBase * interactionPotential(const Container*);

        /* Setup the external potential */
        PotentialBase * externalPotential(const Container*);
    
        /* Setup the trial wave function */
        WaveFunctionBase * waveFunction(const Path &, LookupTable &);

        /* Setup the action */
        ActionBase * action(const Path &, LookupTable &, PotentialBase*, PotentialBase *, WaveFunctionBase *);

        /* Setup the move array */
        boost::ptr_vector<MoveBase> * moves(Path &, ActionBase *, MTRand &);

        /* Setup the estimator array */
        boost::ptr_vector<EstimatorBase> * estimators(Path &, ActionBase *, MTRand &);
        boost::ptr_vector<EstimatorBase> * estimators(boost::ptr_vector<Path> &,
                boost::ptr_vector<ActionBase> &, MTRand&);

        Parameters params;                          ///< All simulation parameters

    private:
        vector<string> interactionPotentialName;    ///< The allowed interaction potential names
        vector<string> externalPotentialName;       ///< The allowed external potential names
        vector<string> waveFunctionName;            ///< The allowed trial wave function names
        vector<string> randomGeneratorName;         ///< The allowed random number generator names
        vector<string> actionName;                  ///< The allowed action names
        vector<string> estimatorName;               ///< The allowed estimator names
        vector<string> moveName;                    ///< The allowed move names
        vector<string> optionClassNames;            ///< The allowed option class names
        vector<string> wavevectorTypeName;          ///< The allowed wavevector type names

        string interactionNames;                    ///< The interaction output list
        string externalNames;                       ///< The external output list
        string waveFunctionNames;                   ///< The wavefunction output list
        string randomGeneratorNames;                ///< The random number generator output list
        string actionNames;                         ///< The action output list
        string estimatorNames;                      ///< The estimator list
        string moveNames;                           ///< The move list
        string wavevectorTypeNames;                 ///< The wavevector types list

        /* The factories needed to instantiate objects */
        MoveFactory moveFactory;                    
        EstimatorFactory estimatorFactory;
        MultiEstimatorFactory multiEstimatorFactory;

        bool definedCell;                           ///< The user has physically set the sim. cell

        boost::ptr_map<string,po::options_description> optionClasses; ///< A map of different option types
        po::options_description cmdLineOptions;     ///< All options combined

        /** process and clean up the command line options */
        void cleanCommandLineOptions(int, char*[], vector<string> &, vector<string> &,
                vector<string>&);
        void update(int,char*[],po::options_description&);

        /** Initialize all possible parameters*/
        void initParameters();

        /* Get a formatted list of xml options */
        string getXMLOptionList(const vector<string> &, const string);  
};

#endif
