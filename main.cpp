#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cstring>
#include <assert.h>
#include <stdexcept>
#include <cstdio>
#include <map>

#define LOG_PARSE

#ifdef LOG_PARSE
#define LOG printf
#else
#define LOG
#endif


using std::string;

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
    string* str = NULL;
    };
    Variable(Type type=NONE) :type(type)
    {
        str = NULL;
        if(type==STR) {
            str = new string;
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
            str = new string(*var.str);
            type = STR;
        }
        else {
            integer = var.integer;
            type = INT;
        }
    }
};

constexpr unsigned int string_hash(const char* str)
{
    unsigned h = 37;
    while(*str) 
    {
        h=(h*54059) ^ (str[0]*76963);
        str++;
    }
    return h%86969;
}
std::unordered_map<string, Variable> shared_variables;

Variable& get_variable(string name)
{
    auto found = shared_variables.find(name);
    if(found != shared_variables.end()) {
        return found->second;
    }
    else {
        // Default all variables to 0
        // Bad design? maybe
        Variable v; v.type = Variable::INT; v.integer = 0;
        shared_variables[name] = v;
    }
    return shared_variables[name];
}
void init_variable(string name, Variable::Type type)
{
    auto found = shared_variables.find(name);
    if(found != shared_variables.end())
        return;
    
    Variable v(type);
    shared_variables.insert({name,v});
}
void set_variable(string name, int new_val)
{
    assert(shared_variables[name].type == Variable::INT);
    shared_variables[name].integer = new_val;
}
void set_variable(string name, string new_val)
{
    assert(shared_variables[name].type == Variable::STR);
    *shared_variables[name].str = new_val;
}

const char* keyword_str[] = {
    "if", "else", "elif", "var"
};

const char* op_strings[] = {
    "F","(", ")", "!", "*", "/", "%", "+", "-", 
    "<", "<=", ">", ">=", "==", "!=", "&&", "||", ":="};
struct Token
{
    enum Specific
    {
        FUNCTION,

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
        G_VAR, //$variable
        L_VAR, //@variable
        
        //KEYWORDS
        IF,
        ELSE,
        ELIF,
        DECLERATION,

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
    ResultVal(const Token& token)
    {
        assert(token.g_type ==Token::VALUE);
        if(token.s_type==Token::INT) {
            type = INT;
            integer = token.integer;
        }
        else if(token.s_type==Token::STR){
            type = STR;
            str = token.string_var;
        }
        else if(token.s_type==Token::G_VAR) {
            Variable& v = get_variable(token.string_var);
            if(v.type==Variable::INT) {
                integer = v.integer;
                type = INT;
            }
            else if(v.type==Variable::STR) {
                str = *v.str;
                type = STR;
            }
        }
        else {
            std::runtime_error("Unknown assignment type for ResultVal\n");
        }
    }
    int integer=0;
    string str;
};
struct Statement
{
    int token_start, token_end;
};
struct Enviornment
{
    // read from input, don't get modified for life of program
    std::vector<Token> t_arr; 
    // values either literals from tokens or resulting vals from operations
    std::vector<Value> val_stack; 
    // resulting values, indexed into through val stack
    std::vector<ResultVal> r_arr;
    // index into token array for operators
    std::vector<int> op_stack;
};
struct Block
{
    Block* parent=NULL;
    std::vector<Block*> children;
    std::vector<Statement> statements;
    // Blocks and states are interweaved but are different types
    // indices are how many statements come before the blocks
    std::vector<int> statement_indices;
    // Local variables
    std::map<string, Variable> vars;
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
        for(auto block : children) {
            delete block;
        }
    }
    virtual void execute(Enviornment& env);
    void execute_blocks_and_statements(Enviornment& env);
};
struct IfBlock : public Block
{
    ~IfBlock()
    {
        delete else_block;
    }
    bool has_statement = true;
    Statement cond_statement;
    // Can be another ifblock or statement block
    Block* else_block;
    virtual void execute(Enviornment& env) override;
};
// Does not pop, just peeks
int get_integer_from_value(Enviornment& env)
{
    if(env.val_stack.back().type == Value::TOKEN) {
        assert(env.t_arr.at(env.val_stack.back().index).s_type==Token::INT);
        return env.t_arr.at(env.val_stack.back().index).integer;
    }
    assert(env.r_arr.at(env.val_stack.back().index).type==ResultVal::INT);
    return env.r_arr.at(env.val_stack.back().index).integer;
}
ResultVal get_resultval_from_top(Enviornment& env)
{
    ResultVal rv;
    assert(env.val_stack.size()>0);
    if(env.val_stack.back().type==Value::RESULT) {
        rv = env.r_arr.at(env.val_stack.back().index);
    }
    else {
        rv = env.t_arr.at(env.val_stack.back().index);
    }
    return rv;
}
#define INTERPFUNC(name) ResultVal name(Enviornment& env)
typedef ResultVal(*InterpFuncPtr)(Enviornment& env);

#include <random>
INTERPFUNC(my_rand)
{
    assert(env.val_stack.size()>=2);
    int val2 = get_integer_from_value(env);
    env.val_stack.pop_back();
    int val1 = get_integer_from_value(env);
    env.val_stack.pop_back();

    ResultVal rv;
    rv.type = ResultVal::INT;
    rv.integer = val1 + (rand() % (val2-val1));

    return rv;
}
INTERPFUNC(print)
{
    for(int i = 0; i < env.val_stack.size();i++) {
        ResultVal rv = get_resultval_from_top(env);
        if(rv.type==ResultVal::INT) {
            std::cout << rv.integer << " ";
        }
        else if (rv.type==ResultVal::STR) {
            std::cout << rv.str;
        }
    }
    // Printing removes all vals from the stack
    ResultVal result(ResultVal::VOID);
    std::cout << "\n";

    return result;
}

struct Function
{
    InterpFuncPtr ptr;
    // For debugging mainly, -1 for multiple (ie takes all arguments from value stack), used for printing
    int num_arguments;
};
std::unordered_map<std::string, Function> function_lookup;
// Calls a built-in or user-defined function
void call_function(const Token& token, Enviornment& env)
{
    assert(token.s_type==Token::FUNCTION);
    auto found = function_lookup.find(token.string_var);
    if(found == function_lookup.end()) {
        throw std::runtime_error("Couldn't find function: " + token.string_var);
    }
    ResultVal result = found->second.ptr(env);
    if(result.type!=ResultVal::VOID) {
        env.r_arr.push_back(result);
        int index = env.r_arr.size()-1;
        env.val_stack.push_back({Value::RESULT, index});
    }
};
// Returns 0 if not operator, returns 1 if 1 length op, 2 if 2 length op (==, <=,..)
// Adds to output
int check_for_operator(const std::string& line, const int index, std::vector<Token>& output)
{
    Token tok(Token::OPERATOR);
    bool is_op = false;
    int i;
    int op_index;
    bool double_length = false;
    for(i = 1; i < 18; i++) {
        if(line.at(index)==op_strings[i][0]) {
            is_op = true;
            op_index = i;
            char ch = op_strings[i][0];
            if(line.size() <= index+1) break;
            char n_ch = line.at(index+1);
            if(!(n_ch == '&' || n_ch == '|' || n_ch == '=')) break;
            for(int j = i; j < 18; j++) {
                if(op_strings[j][0] != ch || strlen(op_strings[j])<=1) {
                    continue;
                }
                if(op_strings[j][1] == n_ch) {
                    op_index = j;
                    double_length = true;
                    break;
                }
            }
            break;
        }
    }
    if(!is_op) return 0;

    tok.s_type = (Token::Specific)op_index;
    output.push_back(tok);

    return 1 + double_length;
}
int check_for_keyword(const string& str)
{
    for(int i = 0; i < 4; i++) {
        if(str== keyword_str[i])
            return Token::IF + i;
    }
    return -1;
}
int check_for_symbol(const std::string& line, const int index, std::vector<Token>& output)
{
    
    char ch = line.at(index);
    if(isdigit(ch)) return -1;
    // variable sign
    bool var = false;
    bool local = false;
    if(ch == '$') {
        var = true;
        ch = line.at(index+1);
    }
    else if(ch=='@') {
        var = true;
        local = true;
        ch = line.at(index+1);
    }

    int offset = 0;
    while(isalpha(ch) || isdigit(ch) || ch == '_') {
        ch = line.at(index + var + ++offset);
    }
    if(var && offset == 0) {
        throw std::runtime_error("Unexpected $ symbol");
    }
    std::string word = line.substr(index+var, offset);

    Token tok(Token::OPERATOR);
    if(var) {
        LOG("Variable parsed: %s\n", word.c_str());
        tok.g_type = Token::VALUE;
        if(local) {
            tok.s_type = Token::L_VAR;
        }
        else {
            tok.s_type = Token::G_VAR;
        }
        tok.string_var = word;
    }
    else {
        int key_check = check_for_keyword(word);
        if(key_check==-1) {
        LOG("Function parsed: %s\n", word.c_str());
        tok.s_type = Token::FUNCTION;
        tok.string_var = word;
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
    if(!isdigit(ch) || line.size()<index+2) return -1;
    // ERROR! check indicies
    int offset = 1;
    ch = line.at(index+offset);
    while(isdigit(ch) && line.size() > index+offset+1) {
        ch = line.at(index + ++offset);
    }
    string str_num = line.substr(index,offset).c_str();
    LOG("Integer found: %s\n", str_num.c_str());
    //printf("Integer found: %s\n", str_num.c_str());
    int res = atoi(str_num.c_str());

    Token tok(Token::VALUE);
    tok.s_type = Token::INT;
    tok.integer = res;

    output.push_back(tok);
    
    return offset-1;
}
int check_for_str_literal(const std::string& line, const int index, std::vector<Token>& output)
{
    char ch = line.at(index);
    if(ch!='\"') return -1;
    assert(index+1<line.size() && "Can't find matching pair of \"");
    ch = line.at(index+1);
    int offset = 1;
    while(ch!='\"')
    {
        assert(index+offset+1 < line.size());
        ch=line.at(index+ ++offset);
    }
    string literal = line.substr(index+1, offset-1);
    LOG("String literal found: %s\n",literal.c_str());
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
    for(i = 0; i < str.size(); i++) {
        if(str.at(i)!=' '&&str.at(i)!='\t') break;
    }
    str.erase(0,i);
}
bool test_open_file(std::string* result, const char* file_name) 
{
    std::ifstream infile(file_name);
    if(!infile) {
        printf("Couldn't open file %s\n", file_name);
        return false;
    }
    std::stringstream ss;
    ss << infile.rdbuf();
    (*result) = ss.str();
    return true; 
}
void assignment_operation(Enviornment& env)
{
    assert(env.val_stack.size()>=2);
    assert(env.val_stack.at(env.val_stack.size()-2).type == Value::TOKEN);
    const Token& var = env.t_arr.at(env.val_stack.at(env.val_stack.size()-2).index);
    assert(var.s_type == Token::G_VAR);
    Variable& stored_var = get_variable(var.string_var);
    ResultVal assigned_value = get_resultval_from_top(env);
    assert(assigned_value.type == stored_var.type || stored_var.type == Variable::NONE);
    switch(assigned_value.type)
    {
    case ResultVal::INT:
        stored_var.integer = assigned_value.integer;
        stored_var.type = Variable::INT;
        break;
    case ResultVal::STR:
        if(stored_var.type == Variable::NONE) {
            stored_var.str = new string(assigned_value.str);
        }
        else {
            *stored_var.str = assigned_value.str;
        }
        stored_var.type = Variable::STR;
    default:
        std::runtime_error("Unknown type");
    }
    env.val_stack.pop_back();
    env.val_stack.pop_back();
}
// Evaluates operators from val stack, POPS THEM TOO!
void evaluate_operator(Token::Specific op_type, Enviornment& env)
{   
    int val1,val2;

    if(op_type == Token::ASSIGNMENT) {
        return assignment_operation(env);
    }

    val2 = get_resultval_from_top(env).integer;//get_integer_from_value(env);
    env.val_stack.pop_back();
    if(op_type!=Token::NEGATE) {
        val1 = get_resultval_from_top(env).integer;
        env.val_stack.pop_back();
    }
    int result = 0;
    switch(op_type)
    {
    case Token::NEGATE:
        result = !val2;
        break;
    case Token::MULT:
        result = val1*val2;
        break;
    case Token::DIV:
        result = val1*val2;
        break;
    case Token::MOD:
        result = val1%val2;
        break;
    case Token::PLUS:
        result = val1+val2;
        break;
    case Token::MINUS:
        result = val1-val2;
        break;
    case Token::LESS:
        result = val1<val2;
        break;
    case Token::LESSEQ:
        result = val1<=val2;
        break;
    case Token::GREAT:
        result = val1 >val2;
        break;
    case Token::GREATEQ:
        result = val1>=val2;
        break;
    case Token::EQUALS:
        result = val1==val2;
        break;
    case Token::NOTEQUALS:
        result = val1!=val2;
        break;
    case Token::LOGAND:
        result = val1&&val2;
        break;
    case Token::LOGOR:
        result=val2||val2;
        break;
    default:
        throw std::runtime_error("unknown symbol: " + op_type);
        break;
    }
    ResultVal rv;
    rv.type = ResultVal::INT;
    rv.integer = result;
    env.r_arr.push_back(rv);
    int index = env.r_arr.size()-1;
    env.val_stack.push_back({Value::RESULT, index});
}
void push_operator(Enviornment& env, int op_index)
{
    const Token& pushed = env.t_arr.at(op_index);
    assert(pushed.s_type >= Token::FUNCTION && pushed.s_type <= Token::ASSIGNMENT);
    while(env.op_stack.size()>0)
    {
        const Token& top = env.t_arr.at(env.op_stack.back());
        assert(top.s_type >= Token::FUNCTION && top.s_type <= Token::ASSIGNMENT);
        // Reached left parentheses, stop
        if(top.s_type==Token::LPAREN)
            break;
        
        // Higher precedence have lower values
        if(top.s_type < pushed.s_type)
        {
            assert(env.val_stack.size()>=2);
            if(top.s_type==Token::FUNCTION) {
                call_function(top, env);
            }
            else {
                evaluate_operator(top.s_type,env);
            }
            env.op_stack.pop_back();
        }
        else {
            break;
        }
    }
    env.op_stack.push_back(op_index);
}
void evaluate_till_lparen(Enviornment& env)
{
    while(env.op_stack.size() > 0 && env.t_arr.at(env.op_stack.back()).s_type != Token::LPAREN)
    {
        const Token& op = env.t_arr.at(env.op_stack.back());
        if(op.s_type==Token::FUNCTION) {
            call_function(op, env);
        }
        else {
            evaluate_operator(op.s_type,env);
        }
        env.op_stack.pop_back();
    }
    assert(env.op_stack.size()>0 && env.t_arr.at(env.op_stack.back()).s_type == Token::LPAREN);
    // Remove l parentheses
    env.op_stack.pop_back();
}
// My lord... is that legal?
void build_base_tree(Block* root, const std::vector<Token>& tokens, int start, int end);

void build_conditional(Block* root, const std::vector<Token>& tokens, int start, int end)
{
    IfBlock* temp = new IfBlock;
    int paren = 0;
    int index = start;
    do
    {
        if(tokens.at(index).s_type==Token::LPAREN)
            paren++;
        if(tokens.at(index).s_type==Token::RPAREN)
            paren--;
        index++;
    } while (paren!=0 && index < end);
    assert(paren==0);
    temp->cond_statement.token_start = start;
    temp->cond_statement.token_end = index-1;
    temp->parent = root;
    temp->type = Block::IF;
    root->children.push_back(temp);
    root->statement_indices.push_back(root->statements.size());

    build_base_tree(temp, tokens, index, end);
}
void build_if_else(Block* root, const std::vector<Token>& tokens, int start, int end)
{
    IfBlock* previous = dynamic_cast<IfBlock*>(root->children.back());
    assert(previous!=NULL);


    IfBlock* temp = new IfBlock;
    previous->else_block = temp;

    int paren = 0;
    int index = start;
    do
    {
        if(tokens.at(index).s_type==Token::LPAREN)
            paren++;
        if(tokens.at(index).s_type==Token::RPAREN)
            paren--;
        index++;
    } while (paren!=0 && index < end);
    assert(paren==0);
    temp->cond_statement.token_start = start;
    temp->cond_statement.token_end = index-1;
    temp->parent = previous;
    temp->type = Block::ELIF;

    build_base_tree(temp, tokens, index, end);

}
void build_else(Block* root, const std::vector<Token>& tokens, int start, int end)
{
    IfBlock* previous = dynamic_cast<IfBlock*>(root->children.back());
    assert(previous!=NULL);

    IfBlock* temp = new IfBlock;
    temp->has_statement = false;
    previous->else_block = temp;
    temp->parent = previous;
    temp->type = Block::ELSE;
    
    build_base_tree(temp, tokens, start, end);

}
void build_general_statement(Block* root, const std::vector<Token>& tokens, int start, int end)
{
    Statement temp;
    temp.token_start = start;
    temp.token_end = end;
    root->statements.push_back(temp);
}
void analyze_statement(Block* root, const std::vector<Token>& tokens, int start, int end, Block::Type& previous)
{
    Token::Specific first_statement=Token::NONE;
    // Find first statement
    for(int i = start; i < end && i < tokens.size(); i++) {
        if(tokens.at(i).s_type==Token::LPAREN) continue;
        first_statement=tokens.at(i).s_type; break;
    }
    assert(first_statement!=Token::NONE);
    switch(first_statement)
    {
    case Token::ELIF:
        assert(previous==Block::IF || previous==Block::ELIF);
        build_if_else(root, tokens, start+2, end);
        previous = Block::ELIF;
        break;
    case Token::ELSE:
        assert(previous==Block::IF || previous==Block::ELIF);
        build_else(root, tokens, start+2, end);
        previous = Block::ELSE;
        break;
    case Token::IF:
        build_conditional(root, tokens, start+2, end);
        previous = Block::IF;
        break;
    default:
        build_general_statement(root, tokens, start, end);
        previous = Block::STATEMENT;
        break;
    }
}
void build_base_tree(Block* root, const std::vector<Token>& tokens, int start, int end)
{
    int paren_level = 0;
    int statement_start = start;
    Block::Type previous = Block::NONE;
    for(int i = start; i < end; i++) {
        if(tokens.at(i).s_type==Token::LPAREN)
            paren_level++;
        if(tokens.at(i).s_type==Token::RPAREN)
            paren_level--;
        if(paren_level==0) {
            analyze_statement(root, tokens, statement_start, i, previous);
            statement_start = i+1;
        }
    }

}
void init()
{
    Function f;
    f.ptr =  &print;
    f.num_arguments = -1;
    function_lookup.insert({"print", f});
    f.ptr = &my_rand;
    f.num_arguments = 2;
    function_lookup.insert({"rand", f});
};
void evaluate_statement(Enviornment& env, int start, int end)
{
    env.val_stack.clear();
    env.r_arr.clear();
    env.op_stack.clear();
    int index = start;
    while(index < env.t_arr.size() && index <= end) 
    {
        const Token& tok = env.t_arr.at(index);
        switch (tok.g_type)
        {
        case Token::General::VALUE:
            env.val_stack.push_back({Value::TOKEN, index});
            break;
        case Token::General::OPERATOR:
            switch(tok.s_type)
            {
            case Token::Specific::LPAREN:
                env.op_stack.push_back(index);
                break;
            case Token::Specific::RPAREN:
                assert(!env.op_stack.empty() && "Mismatched parentheses");
                evaluate_till_lparen(env);
                break;
            
            // Operators
            default:
                assert(tok.s_type <= Token::ASSIGNMENT && "Invalid operator");
                push_operator(env, index);
                break;
            }
            break;
        default:
            break;
        }
        ++index;
    }
    while(!env.op_stack.empty())
    {
        const Token& tok = env.t_arr.at(env.op_stack.back());
        assert(tok.s_type != Token::LPAREN);
        if(tok.s_type==Token::FUNCTION) {
            call_function(tok, env);
        }
        else {
            evaluate_operator(tok.s_type,env);
        }
        env.op_stack.pop_back();
    }

}
void Block::execute_blocks_and_statements(Enviornment& env)
{
    int cur_stmt=0;
    int stmt_till_block;
    int stmt_indices_index = 0;
    if(statement_indices.empty()) {
        stmt_till_block = INT32_MAX;
    }
    else {
        stmt_till_block = statement_indices[0];
    }
    while(1)
    {
        if(cur_stmt<stmt_till_block && cur_stmt < statements.size()) 
        {
            evaluate_statement(env,statements.at(cur_stmt).token_start, statements.at(cur_stmt).token_end);

            ++cur_stmt;
        }
        else if(stmt_indices_index<statement_indices.size())
        {
            if(stmt_indices_index>=statement_indices.size()) {
                stmt_till_block = INT32_MAX;
            }
            else {
                stmt_till_block = statement_indices[stmt_indices_index];
            }
            children[stmt_indices_index]->execute(env);
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
    if(has_statement) {
        evaluate_statement(env,cond_statement.token_start,cond_statement.token_end);
        bool cond = get_integer_from_value(env);
        if(cond) {
            return execute_blocks_and_statements(env);
        }
        else if(else_block){
            return else_block->execute(env);
        }
    }
    else {
       return execute_blocks_and_statements(env); 
    }
}
#include <chrono>
int read_line(Enviornment& env, std::stringstream& helper)
{
    std::string line;
    while(std::getline(helper,line))
    {
        remove_white_space(line);
        if(line.empty() || line.at(0)=='#') {
            continue;
        }
        bool in_word = false;
        bool in_var = false;
        bool in_num = false;
        int word_start = 0;
        for(int i = 0; i < line.size(); i++) {
            if(line.at(i)==' '||line.at(i)=='\t') continue;

            int op_res = check_for_operator(line, i, env.t_arr);
            // CHECK FOR CURRENT WORDS/NUMS
            if(op_res==1) continue;
            else if(op_res==2) {
                i++; continue;
            }
            int str_res = check_for_str_literal(line,i,env.t_arr);
            if(str_res>=0) {
                i+= str_res;
                continue;
            }
            int word_res = check_for_symbol(line, i, env.t_arr);
            if(word_res >= 0)  {
                i+=word_res;
                continue;
            }

            int num_res = check_for_num_literal(line, i, env.t_arr);
            if(num_res>=0) {
                i+=num_res;
                continue;
            }

            // failed to parse
            printf("UNKNOWN SYMBOL @ index %d, line=%s", i, line.c_str());
            return 1;

        }
    }
    return 0;
}
int main()
{
    init();

    std::string source_code;
    
    if(!test_open_file(&source_code, "test1.txt")) {
        return 1;
    }
    // Current status
    unsigned int current_id;
    //Type current_type;
    std::stringstream helper;
    helper << source_code;
    std::string line;

    init_variable("hello_world", Variable::INT);
    set_variable("hello_world", 12345);
    init_variable("str_hw", Variable::STR);
    set_variable("str_hw", "hello world from C++!");
    //std::vector<Token> token_array;

    //Tokenize
    //while(1) {
    //std::stringstream help2;
    //std::cout << ">>>";
    //std::getline(std::cin, line);
    //help2 << line;
    Block program_root;
    Enviornment env;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    read_line(env, helper);
    std::chrono::steady_clock::time_point token = std::chrono::steady_clock::now();
    // Build control flow tree
    build_base_tree(&program_root, env.t_arr, 0, env.t_arr.size());
    std::chrono::steady_clock::time_point tree = std::chrono::steady_clock::now();


    // Evaluate with shunting yard algorithm
    // stacks with index into token vector (don't copy around strings)
    int index = 0;
    // Stores results of token operations ie: tok: 5 + tok: 3 = ReV: 8
    //std::vector<ResultVal> result_array;
    //std::vector<int> op_stack;
    //std::vector<Value> val_stack; 
    //evaluate_statement(token_array, val_stack, result_array, op_stack, 0, token_array.size()-1);
    program_root.execute(env);
    std::chrono::steady_clock::time_point execute = std::chrono::steady_clock::now();
    std::cout << "Tokenize=  " << std::chrono::duration_cast<std::chrono::microseconds>(token - start).count() << "[µs]" << std::endl;
    std::cout << "Tree=  " << std::chrono::duration_cast<std::chrono::microseconds>(tree - token).count() << "[µs]" << std::endl;
    std::cout << "Execute=  " << std::chrono::duration_cast<std::chrono::microseconds>(execute - tree).count() << "[µs]" << std::endl;

    //std::cout << "Result: " << get_integer_from_value(val_stack, result_array, token_array) << "\n";

    return 0;
}