#include <unordered_map>
#include <deque>
#include <string>
#include <assert.h>
struct Variable
{
    
    enum Type
    {
        INT,
        STR,
        NONE
    }type = NONE;
    union {
    int integer;
    std::string* str = NULL;
    };
    Variable(Type type=NONE) :type(type)
    {
        str = NULL;
        if(type==STR) {
            str = new std::string;
        }
    }
    ~Variable()
    {
        if(type==STR) {
            delete str;
        }
    }
    Variable(const Variable& var)
    {
        if(var.type==STR) {
            str = new std::string(*var.str);
            type = STR;
        }
        else {
            integer = var.integer;
            type = INT;
        }
    }
};
struct Enviornment;
struct Block;
struct Token;
struct FuncBlock;
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
    void assign_local_variable(const Token& tok, const Enviornment& env);
    int integer=0;
    std::string str;
};
typedef ResultVal(*MacroPtr)(Enviornment& env, Block& block);
struct Macro
{
    MacroPtr ptr;
    // For debugging mainly, -1 for multiple (ie takes all arguments from value stack), used for printing
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
    FunctionTable functions;
private:
    unsigned index = 0;

    Block* program_root = nullptr;
    Enviornment* env = nullptr;

};