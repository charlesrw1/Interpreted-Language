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
    int integer=0;
    std::string str;
};
typedef ResultVal(*InterpFuncPtr)(Enviornment& env);
struct Function
{
    InterpFuncPtr ptr;
    // For debugging mainly, -1 for multiple (ie takes all arguments from value stack), used for printing
    int num_arguments;
};
class Interpreter
{
public:
    ~Interpreter()
    {
        delete env;
        delete program_root;
    }
    static void init_functions();
    void interpret(std::string code);
    static std::unordered_map<std::string, Variable> variables;
    static std::unordered_map<std::string, Function> functions;
    static std::deque<std::string> events;
    static Variable& get_variable(std::string name)
    {
        auto found = variables.find(name);
        if(found != variables.end()) {
            return found->second;
        }
        else {
            // Default all variables to 0
            // Bad design? maybe
            Variable v; v.type = Variable::INT; v.integer = 0;
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
private:
    Block* program_root = nullptr;
    Enviornment* env = nullptr;

};