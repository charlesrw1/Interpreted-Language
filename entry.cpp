#include "lang.h"

using std::string;

typedef string Logic;
struct ID
{
    enum Type
    {
        REPLY,
        MESSAGE,
        LEAVE,
        OTHER,
    }type;
    unsigned int index;
};
struct Message
{
    string person;
    string message;
    
    Logic show_condition;
    ID default_exit;
    
    // Message condition, skips if false
    Logic main_logic;
};
#define MAX_REPLIES 6
struct ReplySet
{
    unsigned int reply_ids[MAX_REPLIES];
    unsigned char num_replies;   
};
struct Reply
{
    string message;
    Logic main_logic;

    Logic show_condition;
    bool display_if_false=false;

    bool seen=false;
    bool hide_if_choosen=false;
};


int main()
{
    Interpreter::init_functions();
    std::string source = "(print (2*3+2))\n"
                        "(if(1)(print \"Working\"))\n";
    Interpreter inter;
    inter.interpret(source);

    return 0;
};