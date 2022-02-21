#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cstring>
#include <assert.h>
#include <stdexcept>
#include <cstdio>
#include "lang.h"

//#define LOG_PARSE

#ifdef LOG_PARSE
#define LOG printf
#else
#define LOG
#endif


using std::string;

std::unordered_map<string, Variable> Interpreter::variables;
std::unordered_map<string, Macro> Interpreter::macros;

std::deque<string> Interpreter::events;

constexpr unsigned int string_hash(const char* str)
{
    unsigned h = 37;
    while (*str)
    {
        h = (h * 54059) ^ (str[0] * 76963);
        str++;
    }
    return h % 86969;
}

const char* keyword_str[] = {
    "if", "else", "elif", "var","def"
};

const char* op_strings[] = {
    "F","(", ")", "!", "*", "/", "%", "+", "-",
    "<", "<=", ">", ">=", "==", "!=", "&&", "||", ":=" };
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
        DECLERATION,
        FUNCTION_DEF,

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
    string string_var;

    // Data type value, later in union
    int integer;
};
struct Value
{
    enum Type
    {
        TOKEN,
        RESULT
    }type;
    int index;
};

ResultVal::ResultVal(const Token& token)
{
    assert(token.g_type == Token::VALUE);
    if (token.s_type == Token::INT) {
        type = INT;
        integer = token.integer;
    }
    else if (token.s_type == Token::STR) {
        type = STR;
        str = token.string_var;
    }
    else if (token.s_type == Token::G_VAR) {
        Variable& v = Interpreter::get_variable(token.string_var);
        if (v.type == Variable::INT) {
            integer = v.integer;
            type = INT;
        }
        else if (v.type == Variable::STR) {
            str = *v.str;
            type = STR;
        }
    }
    else if (token.s_type == Token::L_VAR) {
        throw std::runtime_error("Assign local variables through method assign_local_variable\n");
    }
    else {
        throw std::runtime_error("Unknown assignment type for ResultVal\n");
    }
}
struct Statement
{
    int token_start, token_end;
};
struct Enviornment
{
    Enviornment(Interpreter& inter) : inter(inter) {}
    // read from input, don't get modified for life of program
    std::vector<Token> t_arr;
    // values either literals from tokens or resulting vals from operations
    std::vector<Value> val_stack;
    // resulting values, indexed into through val stack
    std::vector<ResultVal> r_arr;
    // index into token array for operators
    std::vector<int> op_stack;

    // variables
    std::unordered_map<string, Variable> vars;
    std::vector<string> variable_stack;
    std::vector<unsigned int> allocated_vars_in_block;
    

    Interpreter& inter;
};
struct Block
{
    Block* parent = NULL;
    std::vector<Block*> children;
    std::vector<Statement> statements;
    // Blocks and states are interweaved but are different types
    // indices are how many statements come before the blocks
    std::vector<int> statement_indices;
    // Local variables
    //std::unordered_map<string, Variable> vars;

    enum Type
    {
        ROOT,
        STATEMENT,
        IF,
        ELIF,
        ELSE,
        FUNCTION,
        NONE
    };
    Type type = ROOT;
    ~Block()
    {
        for (auto block : children) {
            delete block;
        }
    }
    virtual void execute(Enviornment& env);
    void execute_blocks_and_statements(Enviornment& env);
};
void ResultVal::assign_local_variable(const Token& tok, const Enviornment& env)
{
    auto find = env.vars.find(tok.string_var);
    if (find != env.vars.end()) {
        if (find->second.type == Variable::INT) {
            integer = find->second.integer;
            type = INT;
        }
        else if (find->second.type == Variable::STR) {
            str = *find->second.str;
            type = STR;
        }
    }
    else {
        throw std::runtime_error("Could not locate local variable for assignment: " + tok.string_var);
    }
}

struct IfBlock : public Block
{
    ~IfBlock()
    {
        delete else_block;
    }
    bool has_statement = true;
    Statement cond_statement;
    // Can be another ifblock or statement block
    Block* else_block=nullptr;
    virtual void execute(Enviornment& env) override;
};
struct FuncBlock : public Block
{
    std::vector<string> parameters;
    string name;
};
// Does not pop, just peeks, UNSAFE!
int get_integer_from_value(Enviornment& env)
{
    if (env.val_stack.back().type == Value::TOKEN) {
        assert(env.t_arr.at(env.val_stack.back().index).s_type == Token::INT);
        return env.t_arr.at(env.val_stack.back().index).integer;
    }
    assert(env.r_arr.at(env.val_stack.back().index).type == ResultVal::INT);
    return env.r_arr.at(env.val_stack.back().index).integer;
}
// SAFER
ResultVal get_resultval_from_top(Enviornment& env, Block& block)
{
    ResultVal rv;
    assert(env.val_stack.size() > 0);
    if (env.val_stack.back().type == Value::RESULT) {
        rv = env.r_arr.at(env.val_stack.back().index);
    }
    else if (env.t_arr.at(env.val_stack.back().index).s_type == Token::L_VAR) {
        rv.assign_local_variable(env.t_arr.at(env.val_stack.back().index), env);
    }
    else {
        rv = env.t_arr.at(env.val_stack.back().index);
    }
    return rv;
}
#define DEF_LANGMACRO(name) ResultVal name(Enviornment& env, Block& block)
typedef ResultVal(*MacroPtr)(Enviornment& env, Block& block);

#include <random>
DEF_LANGMACRO(my_rand)
{
    assert(env.val_stack.size() >= 2);
    int val2 = get_integer_from_value(env);
    env.val_stack.pop_back();
    int val1 = get_integer_from_value(env);
    env.val_stack.pop_back();

    ResultVal rv;
    rv.type = ResultVal::INT;
    rv.integer = val1 + (rand() % (val2 - val1));

    return rv;
}
DEF_LANGMACRO(print)
{
    // reads removes all vals from stack
    while (!env.val_stack.empty()) {
        ResultVal rv = get_resultval_from_top(env, block);
        if (rv.type == ResultVal::INT) {
            std::cout << rv.integer << " ";
        }
        else if (rv.type == ResultVal::STR) {
            std::cout << rv.str;
        }
        env.val_stack.pop_back();
    }
    ResultVal result(ResultVal::VOID);
    std::cout << "\n";

    return result;
}

// Calls a built-in macro
void call_macro(const Token& token, Enviornment& env, Block& block)
{
    assert(token.s_type == Token::MACRO);
    auto found = Interpreter::macros.find(token.string_var);
    if (found == Interpreter::macros.end()) {
        throw std::runtime_error("Couldn't find macro: " + token.string_var);
    }
    ResultVal result = found->second.ptr(env, block);
    if (result.type != ResultVal::VOID) {
        env.r_arr.push_back(result);
        int index = env.r_arr.size() - 1;
        env.val_stack.push_back({ Value::RESULT, index });
    }
};

void call_function(const Token& token, Enviornment& env, Block& block)
{
    assert(token.s_type == Token::FUNCTION);
    auto found = env.inter.functions.find(token.string_var);
    if (found == env.inter.functions.end()) {
        throw std::runtime_error("Couldn't find function: " + token.string_var);
    }
    // Create new enviornment to call function
    Enviornment* new_env = new Enviornment(env.inter);
    // bad, change later
    new_env->t_arr = env.t_arr;
    FuncBlock* fu = found->second;
    // Load parameter variables into function locals
    assert(env.val_stack.size() >= fu->parameters.size());
    for (int i = fu->parameters.size()-1; i >= 0; i--) {
        ResultVal rv = get_resultval_from_top(env, block);
        if (rv.type == ResultVal::INT) {
            new_env->vars[fu->parameters.at(i)].type = Variable::INT;
            new_env->vars[fu->parameters.at(i)].integer = rv.integer;
        }
        else {
            new_env->vars[fu->parameters.at(i)].type = Variable::STR;
            new_env->vars[fu->parameters.at(i)].str = new string;
            *new_env->vars[fu->parameters.at(i)].str = rv.str;
        }
        env.val_stack.pop_back();
        new_env->variable_stack.push_back(fu->parameters.at(i));
    }
    new_env->allocated_vars_in_block.push_back(fu->parameters.size());
    // Call function 
    fu->execute(*new_env);
    // load result into previous stack
    if (new_env->val_stack.size() > 0) {
        ResultVal rv = get_resultval_from_top(*new_env, block);
        env.r_arr.push_back(rv);
        int index = env.r_arr.size() - 1;
        env.val_stack.push_back({ Value::RESULT, index });
    }
    // clean up
    delete new_env;
}
// Returns 0 if not operator, returns 1 if 1 length op, 2 if 2 length op (==, <=,..)
// Adds to output
int check_for_operator(const std::string& line, const int index, std::vector<Token>& output)
{
    Token tok(Token::OPERATOR);
    bool is_op = false;
    int i;
    char ch = line.at(index);
    char n_ch;
    bool double_wide = false;
    if (index + 1 < line.size()) {
        n_ch = line.at(index + 1);
        if ((ch=='&'&&n_ch == '&') || n_ch == '|' || n_ch == '=')
            double_wide = true;
    }
    for (i = 1; i < 18; i++) {
        if (ch == op_strings[i][0]) {
            if (double_wide) {
                if (strlen(op_strings[i]) >= 2 && n_ch == op_strings[i][1]) {
                    is_op = true;
                    break;
                }
            }
            else if(strlen(op_strings[i]) == 1){
                is_op = true;
                break;
            }
        }
    }
    if (!is_op) return 0;

    tok.s_type = (Token::Specific)(i+Token::MACRO);
    output.push_back(tok);

    return 1 + double_wide;
}
int check_for_keyword(const string& str)
{
    for (int i = 0; i < 5; i++) {
        if (str == keyword_str[i])
            return Token::IF + i;
    }
    return -1;
}
int check_for_symbol(const std::string& line, const int index, std::vector<Token>& output)
{
    char ch = line.at(index);
    if (isdigit(ch)) return -1;
    // variable sign
    bool var = false;
    bool global_var = false;
    bool local_var = false;
    bool function = false;
    if (ch == '$') {
        global_var = true;
        ch = line.at(index + 1);
    }
    else if (ch == '&') {
        function = true;
        ch = line.at(index + 1);
    }

    int offset = 0;
    while (isalpha(ch) || isdigit(ch) || ch == '_') {
        ch = line.at(index + var + ++offset);
    }
    if (var && offset == 0) {
        throw std::runtime_error("Unexpected $ symbol");
    }
    std::string word = line.substr(index + var, offset);

    Token tok(Token::OPERATOR);
    if (global_var) {
        LOG("Variable parsed: %s\n", word.c_str());
        tok.g_type = Token::VALUE;
        tok.s_type = Token::G_VAR;
        tok.string_var = word;
    }
    else if (function) {
        LOG("Function parsed: %s\n", word.c_str());
        tok.s_type = Token::FUNCTION;
        tok.string_var = word;
    }
    else {
        int key_check = check_for_keyword(word);
        if (key_check == -1) {
            if (Interpreter::macros.find(word) != Interpreter::macros.end()) {
                LOG("Macro parsed: %s\n", word.c_str());
                tok.s_type = Token::MACRO;
                tok.string_var = word;
            }
            else {
                LOG("Local variable parsed: %s\n", word.c_str());
                tok.s_type = Token::L_VAR;
                tok.g_type = Token::VALUE;
                tok.string_var = word;
            }
        }
        else {
            LOG("Keyword parsed: %s\n", word.c_str());
            tok.g_type = Token::General::KEYWORD;
            tok.s_type = (Token::Specific)key_check;
        }
    }

    output.push_back(tok);

    return offset + var - 1;
}
// only integers for now
int check_for_num_literal(const std::string& line, const int index, std::vector<Token>& output)
{
    char ch = line.at(index);
    if (!isdigit(ch) || line.size() < index + 2) return -1;
    // ERROR! check indicies
    int offset = 1;
    ch = line.at(index + offset);
    while (isdigit(ch) && line.size() > index + offset + 1) {
        ch = line.at(index + ++offset);
    }
    string str_num = line.substr(index, offset).c_str();
    LOG("Integer found: %s\n", str_num.c_str());
    //printf("Integer found: %s\n", str_num.c_str());
    int res = atoi(str_num.c_str());

    Token tok(Token::VALUE);
    tok.s_type = Token::INT;
    tok.integer = res;

    output.push_back(tok);

    return offset - 1;
}
int check_for_str_literal(const std::string& line, const int index, std::vector<Token>& output)
{
    char ch = line.at(index);
    if (ch != '\"') return -1;
    assert(index + 1 < line.size() && "Can't find matching pair of \"");
    ch = line.at(index + 1);
    int offset = 1;
    while (ch != '\"')
    {
        assert(index + offset + 1 < line.size());
        ch = line.at(index + ++offset);
    }
    string literal = line.substr(index + 1, offset - 1);
    LOG("String literal found: %s\n", literal.c_str());
    //printf("String literal found: %s\n",literal.c_str());

    Token tok(Token::VALUE);
    tok.s_type = Token::STR;
    tok.string_var = literal;

    output.push_back(tok);

    return offset;
}

#include <fstream>
#include <sstream>
void remove_white_space(string& str)
{
    int i;
    for (i = 0; i < str.size(); i++) {
        if (str.at(i) != ' ' && str.at(i) != '\t') break;
    }
    str.erase(0, i);
}
bool test_open_file(std::string* result, const char* file_name)
{
    std::ifstream infile(file_name);
    if (!infile) {
        printf("Couldn't open file %s\n", file_name);
        return false;
    }
    std::stringstream ss;
    ss << infile.rdbuf();
    (*result) = ss.str();
    return true;
}
Variable& get_local_or_global_var(Enviornment& env, const Token& var)
{
    assert(var.s_type == Token::G_VAR || var.s_type == Token::L_VAR);

    if (var.s_type == Token::G_VAR) {
        return Interpreter::get_variable(var.string_var);
    }

    auto find = env.vars.find(var.string_var);
    if (find != env.vars.end()) {
        return find->second;
    }
    Variable v;
    v.type = Variable::NONE;
    env.allocated_vars_in_block.back()++;
    env.variable_stack.push_back(var.string_var);
    env.vars[var.string_var] = v;
    return env.vars[var.string_var];
}
void assignment_operation(Enviornment& env, Block& block)
{
    assert(env.val_stack.size() >= 2);
    assert(env.val_stack.at(env.val_stack.size() - 2).type == Value::TOKEN);
    const Token& var = env.t_arr.at(env.val_stack.at(env.val_stack.size() - 2).index);
    assert(var.s_type == Token::G_VAR || var.s_type == Token::L_VAR);
// FIX ME, LOCAL VARIABLES
    Variable& stored_var = get_local_or_global_var(env, var);
    ResultVal assigned_value = get_resultval_from_top(env, block);
    assert(assigned_value.type == stored_var.type || stored_var.type == Variable::NONE);
    switch (assigned_value.type)
    {
    case ResultVal::INT:
        stored_var.integer = assigned_value.integer;
        stored_var.type = Variable::INT;
        break;
    case ResultVal::STR:
        if (stored_var.type == Variable::NONE) {
            stored_var.str = new string(assigned_value.str);
        }
        else {
            *stored_var.str = assigned_value.str;
        }
        stored_var.type = Variable::STR;
        break;
    default:
        throw std::runtime_error("Unknown type");
        break;
    }
    env.val_stack.pop_back();
    env.val_stack.pop_back();
}
// Evaluates operators from val stack, POPS THEM TOO!
void evaluate_operator(Token::Specific op_type, Enviornment& env, Block& block)
{
    int val1, val2;

    if (op_type == Token::ASSIGNMENT) {
        return assignment_operation(env, block);
    }

    val2 = get_resultval_from_top(env, block).integer;//get_integer_from_value(env);
    env.val_stack.pop_back();
    if (op_type != Token::NEGATE) {
        val1 = get_resultval_from_top(env, block).integer;
        env.val_stack.pop_back();
    }
    int result = 0;
    switch (op_type)
    {
    case Token::NEGATE:
        result = !val2;
        break;
    case Token::MULT:
        result = val1 * val2;
        break;
    case Token::DIV:
        result = val1 * val2;
        break;
    case Token::MOD:
        result = val1 % val2;
        break;
    case Token::PLUS:
        result = val1 + val2;
        break;
    case Token::MINUS:
        result = val1 - val2;
        break;
    case Token::LESS:
        result = val1 < val2;
        break;
    case Token::LESSEQ:
        result = val1 <= val2;
        break;
    case Token::GREAT:
        result = val1 > val2;
        break;
    case Token::GREATEQ:
        result = val1 >= val2;
        break;
    case Token::EQUALS:
        result = val1 == val2;
        break;
    case Token::NOTEQUALS:
        result = val1 != val2;
        break;
    case Token::LOGAND:
        result = val1 && val2;
        break;
    case Token::LOGOR:
        result = val1 || val2;
        break;
    default:
        throw std::runtime_error("unknown symbol: " + op_type);
        break;
    }
    ResultVal rv;
    rv.type = ResultVal::INT;
    rv.integer = result;
    env.r_arr.push_back(rv);
    int index = env.r_arr.size() - 1;
    env.val_stack.push_back({ Value::RESULT, index });
}
void push_operator(Enviornment& env, int op_index, Block& block)
{
    const Token& pushed = env.t_arr.at(op_index);
    assert(pushed.s_type >= Token::FUNCTION && pushed.s_type <= Token::ASSIGNMENT);
    while (env.op_stack.size() > 0)
    {
        const Token& top = env.t_arr.at(env.op_stack.back());
        assert(top.s_type >= Token::FUNCTION && top.s_type <= Token::ASSIGNMENT);
        // Reached left parentheses, stop
        if (top.s_type == Token::LPAREN)
            break;

        // Higher precedence have lower values
        if (top.s_type < pushed.s_type)
        {
            assert(env.val_stack.size() >= 2);
            if (top.s_type == Token::MACRO) {
                call_macro(top, env, block);
            }
            else {
                evaluate_operator(top.s_type, env, block);
            }
            env.op_stack.pop_back();
        }
        else {
            break;
        }
    }
    env.op_stack.push_back(op_index);
}
void evaluate_till_lparen(Enviornment& env, Block& block)
{
    while (env.op_stack.size() > 0 && env.t_arr.at(env.op_stack.back()).s_type != Token::LPAREN)
    {
        const Token& op = env.t_arr.at(env.op_stack.back());
        if (op.s_type == Token::MACRO) {
            call_macro(op, env, block);
        }
        else if (op.s_type == Token::FUNCTION) {
            call_function(op, env, block);
        }
        else {
            evaluate_operator(op.s_type, env, block);
        }
        env.op_stack.pop_back();
    }
    assert(env.op_stack.size() > 0 && env.t_arr.at(env.op_stack.back()).s_type == Token::LPAREN);
    // Remove l parentheses
    env.op_stack.pop_back();
}


void build_base_tree(Block* root, const std::vector<Token>& tokens, int start, int end, FunctionTable& ft);

void build_conditional(Block* root, const std::vector<Token>& tokens, int start, int end, FunctionTable& ft)
{
    IfBlock* temp = new IfBlock;
    int paren = 0;
    int index = start;
    do
    {
        if (tokens.at(index).s_type == Token::LPAREN)
            paren++;
        if (tokens.at(index).s_type == Token::RPAREN)
            paren--;
        index++;
    } while (paren != 0 && index < end);
    assert(paren == 0);
    temp->cond_statement.token_start = start;
    temp->cond_statement.token_end = index - 1;
    temp->parent = root;
    temp->type = Block::IF;
    root->children.push_back(temp);
    root->statement_indices.push_back(root->statements.size());

    build_base_tree(temp, tokens, index, end, ft);
}
void build_if_else(Block* root, const std::vector<Token>& tokens, int start, int end, FunctionTable& ft)
{
    IfBlock* previous = dynamic_cast<IfBlock*>(root->children.back());
    assert(previous != NULL);


    IfBlock* temp = new IfBlock;
    previous->else_block = temp;

    int paren = 0;
    int index = start;
    do
    {
        if (tokens.at(index).s_type == Token::LPAREN)
            paren++;
        if (tokens.at(index).s_type == Token::RPAREN)
            paren--;
        index++;
    } while (paren != 0 && index < end);
    assert(paren == 0);
    temp->cond_statement.token_start = start;
    temp->cond_statement.token_end = index - 1;
    temp->parent = previous;
    temp->type = Block::ELIF;

    build_base_tree(temp, tokens, index, end, ft);

}
void build_else(Block* root, const std::vector<Token>& tokens, int start, int end, FunctionTable& ft)
{
    IfBlock* previous = dynamic_cast<IfBlock*>(root->children.back());
    assert(previous != NULL);

    IfBlock* temp = new IfBlock;
    temp->has_statement = false;
    previous->else_block = temp;
    temp->parent = previous;
    temp->type = Block::ELSE;

    build_base_tree(temp, tokens, start, end, ft);

}
void build_general_statement(Block* root, const std::vector<Token>& tokens, int start, int end)
{
    Statement temp;
    temp.token_start = start;
    temp.token_end = end;
    root->statements.push_back(temp);
}
// format: [FUNC_NAME(...)()...())]
void build_function(Block* root, const std::vector<Token>& tokens, int start, int end, FunctionTable& ft)
{
    FuncBlock* temp = new FuncBlock;
    int paren = 0;
    int index = start+1;
    assert(tokens.at(start).s_type == Token::FUNCTION);
    do
    {
        if (tokens.at(index).s_type == Token::LPAREN)
            paren++;
        if (tokens.at(index).s_type == Token::RPAREN)
            paren--;
        index++;
    } while (paren != 0 && index < end);
    assert(paren == 0);
    for (int i = start + 2; i < index-1; i++) {
        assert(tokens.at(i).s_type == Token::L_VAR);
        temp->parameters.push_back(tokens.at(i).string_var);
    }
    temp->type = Block::FUNCTION;
    ft.insert({ tokens.at(start).string_var, temp });
    build_base_tree(temp, tokens, index, end, ft);
};
void analyze_statement(Block* root, const std::vector<Token>& tokens, int start, int end, Block::Type& previous, FunctionTable& ft)
{
    Token::Specific first_statement = Token::NONE;
    // Find first statement
    int i;
    for (i = start; i < end && i < tokens.size(); i++) {
        if (tokens.at(i).s_type == Token::LPAREN) continue;
        first_statement = tokens.at(i).s_type; break;
    }
    //assert(first_statement!=Token::NONE);
    switch (first_statement)
    {
    case Token::ELIF:
        assert(previous == Block::IF || previous == Block::ELIF);
        build_if_else(root, tokens, i + 1, end, ft);
        previous = Block::ELIF;
        break;
    case Token::ELSE:
        assert(previous == Block::IF || previous == Block::ELIF);
        build_else(root, tokens, i + 1, end, ft);
        previous = Block::ELSE;
        break;
    case Token::IF:
        build_conditional(root, tokens, i + 1, end, ft);
        previous = Block::IF;
        break;
    case Token::FUNCTION_DEF:
        build_function(root, tokens, i + 1, end, ft);
        break;
    default:
        // hack that works?
        build_general_statement(root, tokens, i - 1, end - (i - start - 1));
        previous = Block::STATEMENT;
        break;
    }
}
void build_base_tree(Block* root, const std::vector<Token>& tokens, int start, int end, FunctionTable& ft)
{
    int paren_level = 0;
    int statement_start = start;
    Block::Type previous = Block::NONE;
    for (int i = start; i < end; i++) {
        if (tokens.at(i).s_type == Token::LPAREN)
            paren_level++;
        if (tokens.at(i).s_type == Token::RPAREN)
            paren_level--;
        if (paren_level == 0) {
            analyze_statement(root, tokens, statement_start, i, previous, ft);
            statement_start = i + 1;
        }
    }

}
void evaluate_statement(Enviornment& env, int start, int end, Block& block)
{
    env.val_stack.clear();
    env.r_arr.clear();
    env.op_stack.clear();
    int index = start;
    while (index < env.t_arr.size() && index <= end)
    {
        const Token& tok = env.t_arr.at(index);
        switch (tok.g_type)
        {
        case Token::General::VALUE:
            env.val_stack.push_back({ Value::TOKEN, index });
            break;
        case Token::General::OPERATOR:
            switch (tok.s_type)
            {
            case Token::Specific::LPAREN:
                env.op_stack.push_back(index);
                break;
            case Token::Specific::RPAREN:
                assert(!env.op_stack.empty() && "Mismatched parentheses");
                evaluate_till_lparen(env, block);
                break;

                // Operators
            default:
                assert(tok.s_type <= Token::ASSIGNMENT && "Invalid operator");
                push_operator(env, index, block);
                break;
            }
            break;
        default:
            break;
        }
        ++index;
    }
    while (!env.op_stack.empty())
    {
        const Token& tok = env.t_arr.at(env.op_stack.back());
        assert(tok.s_type != Token::LPAREN);
        if (tok.s_type == Token::MACRO) {
            call_macro(tok, env, block);
        }
        else if (tok.s_type == Token::FUNCTION) {
            call_function(tok, env, block);
        }
        else {
            evaluate_operator(tok.s_type, env, block);
        }
        env.op_stack.pop_back();
    }

}
void clean_up_variables(Enviornment& env)
{
    for (int i = 0; i < env.allocated_vars_in_block.back(); i++) {
        env.vars.erase(env.variable_stack.back());
        env.variable_stack.pop_back();
    }
    env.allocated_vars_in_block.pop_back();
}
void Block::execute_blocks_and_statements(Enviornment& env)
{
    int cur_stmt = 0;
    int stmt_till_block;
    int stmt_indices_index = 0;
    if (statement_indices.empty()) {
        stmt_till_block = INT32_MAX;
    }
    else {
        stmt_till_block = statement_indices[0];
    }
    while (1)
    {
        if (cur_stmt < stmt_till_block && cur_stmt < statements.size())
        {
            evaluate_statement(env, statements.at(cur_stmt).token_start, statements.at(cur_stmt).token_end, *this);

            ++cur_stmt;
        }
        else if (stmt_indices_index < statement_indices.size())
        {
            if (stmt_indices_index >= statement_indices.size()) {
                stmt_till_block = INT32_MAX;
            }
            else {
                stmt_till_block = statement_indices[stmt_indices_index];
            }
            env.allocated_vars_in_block.push_back(0);
            children[stmt_indices_index]->execute(env);
            clean_up_variables(env);
            stmt_indices_index++;
        }
        else {
            break;
        }
    }
}
void Block::execute(Enviornment& env)
{
    execute_blocks_and_statements(env);
}
void IfBlock::execute(Enviornment& env)
{
    if (has_statement) {
        evaluate_statement(env, cond_statement.token_start, cond_statement.token_end, *this);
        bool cond = get_integer_from_value(env);
        if (cond) {
            execute_blocks_and_statements(env);
        }
        else if (else_block) {
            env.allocated_vars_in_block.push_back(0);
            else_block->execute(env);
            clean_up_variables(env);
        }
    }
    else {
        execute_blocks_and_statements(env);
    }
}
#include <chrono>
int read_line(Enviornment& env, std::stringstream& helper)
{
    std::string line;
    while (std::getline(helper, line))
    {
        remove_white_space(line);
        if (line.empty() || line.at(0) == '#') {
            continue;
        }
        bool in_word = false;
        bool in_var = false;
        bool in_num = false;
        int word_start = 0;
        for (int i = 0; i < line.size(); i++) {
            if (line.at(i) == ' ' || line.at(i) == '\t') continue;

            int op_res = check_for_operator(line, i, env.t_arr);
            // CHECK FOR CURRENT WORDS/NUMS
            if (op_res == 1) continue;
            else if (op_res == 2) {
                i++; continue;
            }
            int str_res = check_for_str_literal(line, i, env.t_arr);
            if (str_res >= 0) {
                i += str_res;
                continue;
            }
            int word_res = check_for_symbol(line, i, env.t_arr);
            if (word_res >= 0) {
                i += word_res;
                continue;
            }

            int num_res = check_for_num_literal(line, i, env.t_arr);
            if (num_res >= 0) {
                i += num_res;
                continue;
            }

            // failed to parse
            printf("UNKNOWN SYMBOL @ index %d, line=%s", i, line.c_str());
            return 1;

        }
    }
    return 0;
}

ResultVal Interpreter::get_return_value()
{
    return get_resultval_from_top(*env, *program_root);
}
#include <chrono>
void Interpreter::interpret(std::string source)
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    std::stringstream helper;
    helper << source;
    //delete env;
    delete program_root;
    
    if (env == nullptr) {
        env = new Enviornment(*this);
        env->allocated_vars_in_block.push_back(0);
    }
    else {
        env->val_stack.clear();
        env->op_stack.clear();
        env->r_arr.clear();
    }
    
    program_root = new Block;
    read_line(*env, helper);
    std::chrono::steady_clock::time_point after_read = std::chrono::steady_clock::now();

    build_base_tree(program_root, env->t_arr, index, env->t_arr.size(), this->functions);
    std::chrono::steady_clock::time_point after_tree = std::chrono::steady_clock::now();

    program_root->execute(*env);
    std::chrono::steady_clock::time_point after_run = std::chrono::steady_clock::now();
    std::cout << "Lexing = " << std::chrono::duration_cast<std::chrono::microseconds>(after_read - start).count()/1000.0 << "ms" << std::endl;
    std::cout << "AST = " << std::chrono::duration_cast<std::chrono::microseconds>(after_tree - after_read).count()/1000.0 << "ms" << std::endl;
    std::cout << "Execution = " << std::chrono::duration_cast<std::chrono::microseconds>(after_run - after_tree).count() / 1000.0 << "ms" << std::endl;

    index = env->t_arr.size();
}
void Interpreter::init_macros()
{
    Macro f;
    f.ptr = &print;
    f.num_arguments = -1;
    macros.insert({ "print", f });
    f.ptr = &my_rand;
    f.num_arguments = 2;
    macros.insert({ "rand", f });
};
