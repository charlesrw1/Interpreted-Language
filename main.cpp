#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cstring>
#include <assert.h>
#include <stdexcept>
#include <cstdio>
#include <map>

using std::string;

struct Variable
{

};
// base block, execute method just calls the body statement code


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
std::unordered_map<string, int> shared_variables;
int get_variable(string name)
{
    auto found = shared_variables.find(name);
    if(found != shared_variables.end()) {
        return found->second;
    }
    else {
        // Default all variables to 0
        // Bad design? maybe
        shared_variables[name] = 0;
    }
    return 0;
}
void set_variable(string name, int new_val)
{
    shared_variables[name] = new_val;
}

enum class keywords{
    IF,
    ELSE,
    ELIF,
    EQUALS,
    NOTEQUALS,
    LESS,
    GREATER,

    SYMBOLSIZE,

    FUNCTION
};
const char* keyword_str[] = {
    "if", "else", "elif", "var"
};
#include <functional>

const char* op_strings[] = {
    "F","(", ")", "!", "*", "/", "%", "+", "-", 
    "<", "<=", ">", ">=", "==", "!=", "&&", "||"};
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
        VAR,
        
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
    int integer;
    string str;
};
struct Statement
{
    int token_start, token_end;
};
struct Block
{
    Block* parent=NULL;
    std::vector<Block*> children;
    std::map<string, Variable> vars;
    enum Type
    {
        ROOT,
        STATEMENT,
        IF,
        ELIF,
        ELSE,
        NONE
    };
    Type type = ROOT;
    virtual void execute()
    {
        // run through body and execute
        return;
    }
};
struct StatementBlock : public Block
{
    Statement statement;

};
struct IfBlock : public Block
{
    bool has_statement = true;
    Statement cond_statement;
    // Can be another ifblock or statement block
    Block* else_block;
};
// Does not pop, just peeks
int get_integer_from_value(const std::vector<Value>& val_stack, 
const std::vector<ResultVal>& results_arr, const std::vector<Token>& token_arr)
{
    if(val_stack.back().type == Value::TOKEN) {
        assert(token_arr.at(val_stack.back().index).s_type==Token::INT);
        return token_arr.at(val_stack.back().index).integer;
    }
    assert(results_arr.at(val_stack.back().index).type==ResultVal::INT);
    return results_arr.at(val_stack.back().index).integer;
}
#define INTERPFUNC(name) ResultVal name(std::vector<Value>& vals, std::vector<ResultVal>& rvs, const std::vector<Token>& tokens)
typedef ResultVal(*InterpFuncPtr)(std::vector<Value>&, std::vector<ResultVal>&, const std::vector<Token>&);

#include <random>
INTERPFUNC(my_rand)
{
    assert(vals.size()>=2);
    int val2 = get_integer_from_value(vals, rvs, tokens);
    vals.pop_back();
    int val1 = get_integer_from_value(vals,rvs,tokens);
    vals.pop_back();

    ResultVal rv;
    rv.type = ResultVal::INT;
    rv.integer = val1 + (rand() % (val2-val1));

    return rv;
}
INTERPFUNC(print)
{
    for(const auto val : vals) {
        switch(val.type)
        {
        case Value::RESULT:
        {
            const ResultVal& rv = rvs.at(val.index);
            if(rv.type==ResultVal::STR) {
                std::cout << rv.str;
            }
            else {
                std::cout << rv.integer << " ";
            }
            break;
        }
        case Value::TOKEN:
        {
            const Token& tok = tokens.at(val.index);
            if(tok.s_type==Token::STR) {
                std::cout << tok.string_var;
            }
            else {
                std::cout << tok.integer << " ";
            }
            break;
        }
        }
    }
    // Printing removes all vals from the stack
    vals.resize(0);
    ResultVal result = {ResultVal::VOID};
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
void call_function(const Token& token, 
std::vector<Value>& vals, std::vector<ResultVal>& rvs, const std::vector<Token>& tokens)
{
    assert(token.s_type==Token::FUNCTION);
    auto found = function_lookup.find(token.string_var);
    if(found == function_lookup.end()) {
        throw std::runtime_error("Couldn't find function: " + token.string_var);
    }
    ResultVal result = found->second.ptr(vals,rvs,tokens);
    if(result.type!=ResultVal::VOID) {
        rvs.push_back(result);
        int index = rvs.size()-1;
        vals.push_back({Value::RESULT, index});
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
    for(i = 1; i < 17; i++) {
        if(line.at(index)==op_strings[i][0]) {
            is_op = true;
            op_index = i;
            char ch = op_strings[i][0];
            if(line.size() <= index+1) break;
            char n_ch = line.at(index+1);
            if(!(n_ch == '&' || n_ch == '|' || n_ch == '=')) break;
            for(int j = i; j < 17; j++) {
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
int func(int& a)
{
    a = 10;
    return 10;
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
    if(ch == '$') {
        var = true;
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
        printf("Variable parsed: %s\n", word.c_str());
        tok.g_type = Token::VALUE;
        tok.s_type = Token::VAR;
        tok.string_var = word;
    }
    else {
        int key_check = check_for_keyword(word);
        if(key_check==-1) {
        printf("Function parsed: %s\n", word.c_str());
        tok.s_type = Token::FUNCTION;
        tok.string_var = word;
        }
        else {
        printf("Keyword parsed: %s\n", word.c_str());
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
    printf("Integer found: %s\n", str_num.c_str());
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
    printf("String literal found: %s\n",literal.c_str());

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

// Evaluates operators from val stack, POPS THEM TOO!
void evaluate_operator(Token::Specific op_type, std::vector<Value>& val_stack, 
const std::vector<Token>& tokens, std::vector<ResultVal>& result_array)
{   
    int val1,val2;
    val2 = get_integer_from_value(val_stack, result_array, tokens);
    val_stack.pop_back();
    if(op_type!=Token::NEGATE) {
        val1 = get_integer_from_value(val_stack, result_array, tokens);
        val_stack.pop_back();
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
    result_array.push_back(rv);
    int index = result_array.size()-1;
    val_stack.push_back({Value::RESULT, index});
}
void push_operator(std::vector<int>& op_stack, std::vector<Value>& val_stack, 
const std::vector<Token>& tokens, std::vector<ResultVal>& result_arr, int op_index)
{
    const Token& pushed = tokens.at(op_index);
    assert(pushed.s_type >= Token::FUNCTION && pushed.s_type <= Token::LOGOR);
    while(op_stack.size()>0)
    {
        const Token& top = tokens.at(op_stack.back());
        assert(top.s_type >= Token::FUNCTION && top.s_type <= Token::LOGOR);
        // Reached left parentheses, stop
        if(top.s_type==Token::LPAREN)
            break;
        
        // Higher precedence have lower values
        if(top.s_type < pushed.s_type)
        {
            assert(val_stack.size()>=2);
            if(top.s_type==Token::FUNCTION) {
                call_function(top, val_stack, result_arr, tokens);
            }
            else {
                evaluate_operator(top.s_type,val_stack,tokens, result_arr);
            }
            op_stack.pop_back();
        }
        else {
            break;
        }
    }
    op_stack.push_back(op_index);
}
void evaluate_till_lparen(std::vector<int>& op_stack, std::vector<Value>& val_stack, 
        const std::vector<Token>& tokens, std::vector<ResultVal>& result_arr)
{
    while(op_stack.size() > 0 && tokens.at(op_stack.back()).s_type != Token::LPAREN)
    {
        const Token& op = tokens.at(op_stack.back());
        if(op.s_type==Token::FUNCTION) {
            call_function(op, val_stack, result_arr, tokens);
        }
        else {
            evaluate_operator(op.s_type,val_stack,tokens, result_arr);
        }
        op_stack.pop_back();
    }
    assert(op_stack.size()>0 && tokens.at(op_stack.back()).s_type == Token::LPAREN);
    // Remove l parentheses
    op_stack.pop_back();
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
    StatementBlock* temp = new StatementBlock;
    temp->type = Block::STATEMENT;
    temp->parent = root;
    temp->statement.token_start = start;
    temp->statement.token_end = end;
    root->children.push_back(temp);
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

    std::vector<Token> token_array;
    Block program_root;

    //Tokenize
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

            int op_res = check_for_operator(line, i, token_array);
            // CHECK FOR CURRENT WORDS/NUMS
            if(op_res==1) continue;
            else if(op_res==2) {
                i++; continue;
            }
            int str_res = check_for_str_literal(line,i,token_array);
            if(str_res>=0) {
                i+= str_res;
                continue;
            }
            int word_res = check_for_symbol(line, i, token_array);
            if(word_res >= 0)  {
                i+=word_res;
                continue;
            }

            int num_res = check_for_num_literal(line, i, token_array);
            if(num_res>=0) {
                i+=num_res;
                continue;
            }

            // failed to parse
            printf("UNKNOWN SYMBOL @ index %d, line=%s", i, line.c_str());
            return 1;

        }
    }
    // Build control flow tree
    build_base_tree(&program_root, token_array, 0, token_array.size());

    // Evaluate with shunting yard algorithm
    // stacks with index into token vector (don't copy around strings)
    int index = 0;
    // Stores results of token operations ie: tok: 5 + tok: 3 = ReV: 8
    std::vector<ResultVal> result_array;
    std::vector<int> op_stack;
    std::vector<Value> val_stack; 
    while(index < token_array.size()) 
    {
        const Token& tok = token_array.at(index);
        switch (tok.g_type)
        {
        case Token::General::VALUE:
            val_stack.push_back({Value::TOKEN, index});
            break;
        case Token::General::OPERATOR:
            switch(tok.s_type)
            {
            case Token::Specific::LPAREN:
                op_stack.push_back(index);
                break;
            case Token::Specific::RPAREN:
                assert(!op_stack.empty() && "Mismatched parentheses");
                evaluate_till_lparen(op_stack, val_stack, token_array, result_array);
                break;
            
            // Operators
            default:
                assert(tok.s_type <= Token::LOGOR && "Invalid operator");
                push_operator(op_stack, val_stack, token_array, result_array, index);
                break;
            }
            break;
        default:
            break;
        }
        ++index;
    }
    while(!op_stack.empty())
    {
        const Token& tok = token_array.at(op_stack.back());
        assert(tok.s_type != Token::LPAREN);
        if(tok.s_type==Token::FUNCTION) {
            call_function(tok, val_stack, result_array, token_array);
        }
        else {
            evaluate_operator(tok.s_type,val_stack,token_array,result_array);
        }
        op_stack.pop_back();
    }

    std::cout << "Result: " << get_integer_from_value(val_stack, result_array, token_array) << "\n";

    return 0;
}