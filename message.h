#include <string>
using std::string;

enum Type
{
    REPLY,
    MESSAGE,
    LEAVE,
    OTHER,
};
struct Message
{
    string person;
    string message;
    
    // Default exit
    Type exit_type;
    unsigned int exit_id;

    // Message condition, skips if false
    string logic;
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
    string logic;
    bool display_if_false;
    bool seen;
    bool hide_if_choosen;
};
