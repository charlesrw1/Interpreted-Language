#include <unordered_map>
#include <deque>
#include <string>
#include <vector>
#include <assert.h>

struct Enviornment;
struct Token
{
    enum Specific
    {
        FUNCTION,   // user defined functions in language
        MACRO,      // functions in C++ accessible in language

        //OPERATORS
        LPAREN,
        RPAREN,
        NEGATE,
        MULT,
        DIV,
        MOD,
        PLUS,
        MINUS,
        LESS,
        LESSEQ,
        GREAT,
        GREATEQ,
        EQUALS,
        NOTEQUALS,
        LOGAND,
        LOGOR,
        ASSIGNMENT,

        //VALUE TYPES
        INT,
        STR,
        FLOAT,
        G_VAR, // $variable
        L_VAR, // variable

        //KEYWORDS
        IF,
        ELSE,
        ELIF,
        WHILE,
        DECLERATION,
        FUNCTION_DEF,
        LIST_DEF,
        PTR_DEF,

        COMMA,

        NONE
    }s_type;
    enum General
    {
        VALUE,
        OPERATOR,
        KEYWORD,
        SEPERATOR,
    }g_type;

    Token(General type) : g_type(type) {}
    // Used for: function name, variable name, or string literal
    std::string string_var;

    // Data type value, later in union
    int integer;
};
struct Variable;
struct ResultVal
{   
    enum Type
    {
        INT,
        STR,

        // Ie don't push on to the stack
        VOID
    }type;
    ResultVal() {}
    ResultVal(ResultVal::Type type) : type(type) {}
    ResultVal(const Token& token);
    ResultVal(const Variable& var);
    void assign_local_variable(const Token& tok, const Enviornment& env);
    int integer=0;
    std::string str;
};
struct Variable
{
    enum Type
    {
        INT,
        STR,

        PTR,
        LIST,

        NONE
    }type = NONE;
    union {
    int integer;
    std::string* str = NULL;
    std::vector<Variable>* list;
    };
    Variable(Type type=NONE) :type(type)
    {
        str = NULL;
        if(type==STR) {
            str = new std::string;
        }
        else if (type == LIST) {
            list = new std::vector<Variable>;
        }
    }
    ~Variable()
    {
        if(type==STR) {
            delete str;
        }
        else if (type == LIST) {
            delete list;
        }
    }
    void assign_variable_type(Enviornment& env, const Token& tok);
    Variable(const Variable& var);
    Variable& operator=(const Variable& var);
    Variable& operator=(const Token& tok);
    Variable& operator=(const ResultVal& rv);
    Variable(const ResultVal& rv);
};
struct Block;
struct FuncBlock;
typedef ResultVal(*MacroPtr)(Enviornment& env);
struct Macro
{
    MacroPtr ptr;
    int num_arguments;
};
typedef std::unordered_map<std::string, FuncBlock*> FunctionTable;
class Interpreter
{
public:
    ~Interpreter()
    {
        delete env;
        delete program_root;
    }
    static void init_macros();
    void interpret(std::string code);
    std::istream& interpret(std::istream& stream);
    static std::unordered_map<std::string, Variable> variables;
    static std::unordered_map<std::string, Macro> macros;
    static std::deque<std::string> events;
    static Variable& get_variable(std::string name)
    {
        auto found = variables.find(name);
        if(found != variables.end()) {
            return found->second;
        }
        else {
            Variable v; v.type = Variable::NONE;
            variables[name] = v;
        }
        return variables[name];
    }
    static void init_variable(std::string name, Variable::Type type)
    {
        auto found = variables.find(name);
        if(found != variables.end())
            return;
        
        Variable v(type);
        variables.insert({name,v});
    }
    static void set_variable(std::string name, int new_val)
    {
        assert(variables[name].type == Variable::INT);
        variables[name].integer = new_val;
    }
    static void set_variable(std::string name, std::string new_val)
    {
        assert(variables[name].type == Variable::STR);
        *variables[name].str = new_val;
    }
    ResultVal get_return_value();
    // Reset interpreter state
    void wipe_functions_and_tokens()
    {
        functions.clear();
        env_tokens.clear();
    }
    // these shouldn't be here, but they need access across all enviornments
    FunctionTable functions;
    std::vector<Token> env_tokens;
private:
    unsigned index = 0;
    Block* program_root = nullptr;
    Enviornment* env = nullptr;

};