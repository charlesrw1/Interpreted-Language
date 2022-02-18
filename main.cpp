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

        FUNCTION
    }type;
    KeyOp(KeyOp::Type type) : type(type) {}

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
    Value(Value::Type type) : type(type) {}
    ~Value() {}
    union
    {
        int i;
        std::string str;
        Variable var;
    };
};
struct Token
{
    enum Type
    {
        VALUE,
        OPERATOR
    }type;
    Token(Type type) : type(type) {}
    union
    {
        Value val;
        KeyOp keyop;
    };
};


#include <fstream>
#include <sstream>
void remove_white_space(string& str)
{
    int i;
    for(i = 0; i < str.size(); i++) {
        if(str.at(i)!=' ') break;
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
bool is_operator(char c)
{
    return c=='+' || c=='-' || c=='*' || c=='/' || c=='%';
}
// hack for now
const char* prec = "/*%-+p";
bool precedence(char o1, char o2)
{
    for(int i = 0; i < 6; i++) {
        if(prec[i]==o2)
            return false;
        else if(prec[i]==o1)
            return true;
    }
}
struct Token
{

};
#include <iostream>
int main()
{
    
    std::string source_code;
    std::cout << sizeof(source_code);
    Value val(Value::INT);
    std::cout << sizeof(val);
    return 0;
    
    if(!test_open_file(&source_code, "test1.txt")) {
        return 1;
    }
    // Current status
    unsigned int current_id;
    Type current_type;
    std::stringstream helper;
    helper << source_code;
    std::string line;
    std::vector<Value> stack;

    std::vector<char> os;
    std::vector<int> vs;

    std::vector<Token> tokenized_input;

    while(std::getline(helper,line))
    {
        remove_white_space(line);
        if(line.empty() || line.at(0)=='#') {
            continue;
        }
        bool in_word = false;
        bool in_num = false;
        int word_index = 0;
        for(int i = 0; i < line.size(); i++) {
            if(line[i]=='(')
                os.push_back('(');
            if(!in_word && isalpha(line[i])) {
                word_index = i;
                in_word = true;
            }
            else if(in_word && line[i]==' ') {
                in_word = false;
                std::cout << "Symbol found: " << line.substr(word_index, i) << std::endl;

                // Temporary
                os.push_back('p');
            }
            else if(in_word) {
                if(!isalpha(line[i]) && line[i]!='_') {
                    printf("INVALID SYMBOL SYNTAX: %c\n", line[i]);
                    return 1;
                }
            }
            // 
            
            else if(is_operator(line[i])) {
                // while back operator is above current
                if(os.back()=='(') {
                    goto SKIP;
                }
                while(precedence(os.back(), line[i])) {
                    int val2 = vs.back();
                    vs.pop_back();
                    int val1;
                    if(os.size()>0) {
                    val1 = vs.back();
                    vs.pop_back();
                    }
                    int res;
                    switch(os.back())
                    {
                    case '*':
                        res = val1*val2;
                        break;
                    case '/':
                        res = val1/val2;
                        break;
                    case '+':
                        res = val1+val2;
                        break;
                    case '-':
                        res = val1-val2;
                        break;
                    case 'p':
                        std::cout << val2 << std::endl;
                        goto SKIP;
                        break;
                    }
                    vs.push_back(res);
                }
                SKIP:
            }
            else if(isdigit())


        }
    }

    return 0;
}