#include <string>
#include <unordered_map>
#include <vector>
using std::string;

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

enum Comparison{
    EQUALS,
    NOTEQUALS,
    GREATER,
    LESSER
};
struct Condition
{
    string variable;
    Comparison type;
    int val;
};
bool if_cond(const Condition& cond)
{
    int variable = get_variable(cond.variable);
    switch(cond.type) {
    case EQUALS:
        return variable == cond.val;
    case NOTEQUALS:
        return variable != cond.val;
    }
    return false;
}

enum Type
{
    REPLY,
    MESSAGE,
    LEAVE,
    OTHER,
};
using Logic = string;
struct Message
{
    string person;
    string message;
    
    // Default exit
    Type exit_type;
    unsigned int exit_id;

    // Message condition, skips if false
    Condition message_cond;
};
#define MAX_REPLIES 6
struct ReplySet
{
    unsigned int reply_ids[6];
    unsigned char num_replies;   
};
struct Reply
{
    string message;
    Condition reply_cond;
    bool display_if_false;
    bool seen;
    bool hide_if_choosen;
};
void func()
{
    return;
}
void parse_logic(string logic)
{
    func();
    int func;
}

#include <vector>
#include <cstdio>

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
const char* keyword_list[] = {
    "if", "else", "elif", "==", "!=", "<", ">"
};
#include <functional>
using Variable = std::string;
using Function = std::string;
/*
struct KeyOp
{
    enum Type
    {
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
        
        //KEYWORDS
        KEYWORDBEGIN,
        IF,
        ELSE,
        ELIF,
        ENDKEYWORD,

        COMMA,

        FUNCTION
    }type;
    KeyOp(KeyOp::Type type) : type(type) {}
    ~KeyOp() {}

    // Only used if function is type
    std::string function_name;
};


struct Value
{
    enum Type
    {
        INT,
        STR,
        VAR
    }type;
    union
    {
        int i;
        string* str;
    }u;
    Value(Value::Type type) : type(type) 
    {
        if(type == STR || type == VAR) {
            u.str = new string;
        }
    }
    ~Value() 
    {
        if(type == STR || type == VAR) {
            delete u.str;
        }
    }
};
*/
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

        //VALUE TYPES
        INT,
        STR,
        VAR,
        
        //KEYWORDS
        IF,
        ELSE,
        ELIF,

        COMMA
    }s_type;
    enum General
    {
        VALUE,
        OPERATOR,
        SEPERATOR,
    }g_type;

    Token(General type) : g_type(type) {}
    // Used for: function name, variable name, or string literal
    string string_var;

    // Data type value, later in union
    int integer;
};
#include <assert.h>
#include <stdexcept>

// Returns 0 if not operator, returns 1 if 1 length op, 2 if 2 length op (==, <=,..)
// Adds to output
const char op_symbols []= {'F','(',')','!','*','/','%','+','-','<','>','=','&','|'};
int check_for_operator(const std::string& line, const int index, std::vector<Token>& output)
{
    Token tok(Token::OPERATOR);
    bool is_op = false;
    int i;
    for(i = 1; i < 14; i++) {
        if(line.at(index)==op_symbols[i]) {
            is_op = true;
            break;
        }
    }
    if(!is_op) return 0;

    if(i >= 9 || i==3) {
        if(line.size() > index+1) {
            char next = line.at(index + 1);
            switch(i)
            {
            case 3:
                if(next == '=') 
                    i=Token::NOTEQUALS;
            // <
            case 9:
                if(next == '=')
                    i=Token::LESSEQ;
                break;
            // >
            case 10:
                i = Token::GREAT;
                if(next == '=')
                    i+=1;
                break;
            // =
            case 11:
                if(next == '=')
                    i = Token::EQUALS;
                else {
                    throw std::runtime_error("EQUALS (=) not supported, use SET function");
                }
            case 12:
                if(next == '&')
                    i = Token::LOGAND;
                else {
                    throw std::runtime_error("& not supported");
                }
            case 13:
                if(next =='|')
                    i = Token::LOGOR;
                else {
                    throw std::runtime_error("| not supported");
                }
            }
        }
    }
    tok.s_type = (Token::Specific)i;
    output.push_back(tok);

    return 1 + (i==10||i==12||i==13||i==14||i==15||i==16);
}
int func(int& a)
{
    a = 10;
    return 10;
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
        printf("Function parsed: %s\n", word.c_str());
        tok.s_type = Token::FUNCTION;
        tok.string_var = word;
    }

    output.push_back(tok);

    return offset + var - 1;
}
// only integers for now
int check_for_literals(const std::string& line, const int index, std::vector<Token>& output)
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
#include <iostream>
int main()
{
    int a = 1;
    int b = a+func(a);
    std::cout << b << "\n";

    std::string source_code;
    
    if(!test_open_file(&source_code, "test1.txt")) {
        return 1;
    }
    // Current status
    unsigned int current_id;
    Type current_type;
    std::stringstream helper;
    helper << source_code;
    std::string line;

    std::vector<Token> tokenized_input;

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

            int op_res = check_for_operator(line, i, tokenized_input);
            // CHECK FOR CURRENT WORDS/NUMS
            if(op_res==1) continue;
            else if(op_res==2) {
                i++; continue;
            }
            int word_res = check_for_symbol(line, i, tokenized_input);
            if(word_res >= 0)  {
                i+=word_res;
                continue;
            }

            int num_res = check_for_literals(line, i, tokenized_input);
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