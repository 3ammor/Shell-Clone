// Mostafa Gamal Ahmed - Sec 2 - BN 16
// Omar Abdelrahman Mohamed - Sec 1 - BN 31

//Includes of the required libraries.
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <vector>
#include <string>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <termios.h>
#include <errno.h>
#include <sstream>
#include <dirent.h>
#include <fcntl.h>
#include <list>
#include <algorithm>
///////////////////End includes.

using namespace std; 

bool triggered;
stringstream ss;                                                                    //The shell stringstream.
const int csize = 40; list<string> commands; list<string>::iterator it;             //List Datastructure is used to store the commands.

struct termios control_kb_buffer(bool disable,struct termios* my_old = NULL);       //Function that stops the keyboard buffer, in order to use our own created buffer.
void my_chdir(char* input);                                                         //Function that changes the current working directory.
char* my_cd();                                               //Function that takes the input directory and calls my_chdir();
void draw_screen(char* path);                                //Function that displays the logged in username, the machine name, and the current working directory.
void change_signal_handler();                                //Function that changes the SIGINT handler into int_signal_handler()
void int_signal_handler(int signum);                         //Handler for SIGINT signal.
vector<char*> list_dir(const char* path, const string& target);     //Function that is used by the tab key handler to list all files and folders corresponding to a certain target in a given path.
void tab_handler(string& buffer);                            //The tab key handling function.
bool getch_f(string& buffer);                                //Our own getchar function that uses our own keyboard buffer.
void execute(const string& file);                            //Function that executes a required process either in foreground or in backgound.
char* init_path();                                           //Function that initializes the current working directory to the home directory.
int stricmp(const char* a, const char* b);                   //Function for sorting alphabetically ignoring character casing.
bool compare(const char *c1, const char *c2);                //Compare function in used in sorting directories and files alphabetically.

int main()
{
    char* current_path = init_path(); triggered=false;
    change_signal_handler();
	struct termios my_confg = control_kb_buffer(true);          //disable system keyboard buffer.

	while(1) {
		string buf,operation;
		ss.clear();
		ss.str("");
		
		draw_screen(current_path);
		while(!triggered && !getch_f(buf));
        
            if(!triggered)                          //The user hasn't pressed CTRL + C 
            {
                ss << buf;
                ss >> operation;
            }
            else triggered = false;
            
            if(operation == "cd")
                {
                 current_path = my_cd();
                }
            else if(operation == "exit") {
                break;
                }
            else if(operation == "")
                continue;
            else execute(operation);
	}

	control_kb_buffer(false, &my_confg);                    //enable system keyboard buffer.
	return 0;
}

void change_signal_handler()                                //Change the SIGINT handler permanently in the shell (parent) process.
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = int_signal_handler;
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

struct termios control_kb_buffer(bool disable,struct termios* my_old) {
	if(disable)
	{
		struct termios old_tio, new_tio;
		unsigned char c;
		tcgetattr(STDIN_FILENO,&old_tio);       // get the terminal settings for stdin 
		new_tio=old_tio;            // we want to keep the old setting to restore them a the end 
		new_tio.c_lflag &=(~ICANON );           // disable canonical mode (buffered i/o) and local echo 
		tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);       // set the new settings immediately
		return old_tio;                 //return the original keyboard buffer settings.
	}
	else 
	{
		if (my_old!=NULL) 
			tcsetattr(STDIN_FILENO,TCSANOW, my_old);
		return *my_old;                 
	}
}

void my_chdir(char* input) {
    int ret = chdir(input);
    if(ret == -1)
    {
        string str = "tash: cd: "+string(input);
        perror(str.c_str());
    }
}

char* my_cd() {
	string inp; ss>>inp;
    char* input = (char*) inp.c_str();
    my_chdir(input);
    return get_current_dir_name();              //return the current directory name after finishing.
}

void draw_screen(char* path) {              //This function uses system calls to get the logged in username, computername..etc.
	int bufsize;

	if ((bufsize = sysconf(_SC_GETPW_R_SIZE_MAX)) == -1)
		abort();

	char buffer[bufsize];
	struct passwd pwd, *result = NULL;
	if (getpwuid_r(getuid(), &pwd, buffer, bufsize, &result) != 0 || !result)
		abort();
	char hostname[HOST_NAME_MAX];
	gethostname(hostname, HOST_NAME_MAX);
	printf("\r%s@%s:%s$ ",pwd.pw_name,hostname,path);
	
}

void int_signal_handler(int signum) {
    printf("\n");
    triggered = true;
}

vector<char*> list_dir(const char* path, const string& target) {
    DIR *dir; vector<char*> files;
    struct dirent* ent;
    if((dir = opendir(path))!=NULL)             //Get all filenames and directories that starts with the same name in target.
    {
        while((ent = readdir(dir))!=NULL){
            int len = (int) target.length();
            string temp = string(ent->d_name);
            if(len == 0 || (temp.length()>=len && target == temp.substr(0,len)))
                files.push_back(ent->d_name);
        }
        closedir(dir);
    }
    return files;
}

int stricmp(const char* a, const char* b)           //function for sorting alphabetically ignoring character casing.
{
    unsigned int sz = min(strlen(a),strlen(b));
    for (unsigned int i = 0; i < sz; ++i)
        if (tolower(a[i]) > tolower(b[i]))
            return 1;
        else if (tolower(a[i]) < tolower(b[i]))
            return -1;
    if(strlen(a)<strlen(b))
        return -1;
    if(strlen(a)>strlen(b))
        return 1;
    return 0;
}

bool compare(const char *c1, const char *c2)
{
    return stricmp(c1,c2) < 0;
}

void tab_handler(string& buffer) {          //Tab handling function. Handles different situations for completion.
        string path = "", target = "";
        int ind = -1; int i;
        for (i=(int)buffer.length()-1;i>=0;i--)
        {
            if(ind == -1) 
            {
                if(buffer[i]=='/') {
                    ind = i;
                    target = (i+1<(int) buffer.length()) ? buffer.substr(i+1,string::npos) : "";
                }
                else if(buffer[i] == ' ') {
                    target = (i+1<(int) buffer.length()) ? buffer.substr(i+1,string::npos) : "";
                    path = get_current_dir_name();
                    break;
                }
            }
            else
            {
                if(buffer[i] == ' ') 
                {
                    path = buffer.substr(i+1,ind-i); 
                    break;
                }
            }      
        }
        if(i==-1) 
        {
            if(target != "") 
                path = buffer.substr(0,ind+1); 
            else 
            {
                if(ind==-1){
                    target = buffer;
                    path = get_current_dir_name();
                } else {
                    path = buffer.substr(0,ind+1);
                }
            }
        }
    
        vector<char*> files = list_dir(path.c_str(),target); int sz = (int)files.size();
        sort(files.begin(),files.end(),compare);
        if(sz == 0)                  //Not a file or a directory, then remove the printed tab.
        {
            draw_screen(get_current_dir_name());
            printf("%s",buffer.c_str());   
        }
        else if(sz > 1) {                //Listing them if their number exceed 1 entry.
            for(int i=0;i<(int) files.size();i++)
            {
                if(!(i%5)) printf("\n");
                printf("%s   ",files[i]);
            }
            printf("\n");
            draw_screen(get_current_dir_name());
            printf("%s",buffer.c_str());   
        }
        else if (sz ==1)           //Completing the required filename or directory.
        {
            string filename = files[0];
            printf("\r");
            draw_screen(get_current_dir_name());
            string remaining = filename.substr(target.length(),string::npos);
            buffer += remaining;
            commands.back() = buffer;
            printf("%s",buffer.c_str());
        }
}   

bool getch_f(string& buffer) {
	char c; c = getchar(); static bool enter = true;
	if(c=='\n') {       //Enter character stores the executed command.
        enter = true;
        int sz = (int) commands.size();
        if(buffer.size()>0)
        {
            if(sz>=csize)
               commands.pop_front();
            if(sz == 0) commands.push_back(buffer);
            else commands.back() = buffer;
            it = commands.end();
        }
		return true;
    }
        
    if(c== 27 ) {       //Escape character Handling
            printf("\b \b\b \b");
            char c1 = getchar(); 
            if(c1 == 91)
            {
                char c2 = getchar();
                printf("\b \b\b \b");
                if(c2 == 65)            //Up arrow, get the previous command.
                {
                    list<string>::iterator wrong = commands.begin(); wrong--;
                    if((int)commands.size()>0)
                         {
                             it--; 
                             if (it != wrong) buffer = *it; else it = commands.begin();
                             printf("%c[2K\r",27);              //Clear the entire line.
                             draw_screen(get_current_dir_name());
                             printf("%s",buffer.c_str());
                         } 
                }
                else if(c2 == 66) {             //Down arrow, get the newer previous command.
                    list<string>::iterator ende = commands.end();
                    if((int)commands.size()>0 && it!=ende)
                         {
                             it++;  
                             if(it!= ende) buffer = *it; else { it = ende; it--;}
                             printf("%c[2K\r",27); 
                             draw_screen(get_current_dir_name());
                             printf("%s",buffer.c_str());
                         }
                }
            }
            else printf("\b \b\b \b"); 
        return false;
    }    
        
    if(c=='\t')             //If the given character is a Tab, then call tab_handler();
    {
       // enter = false;
        tab_handler(buffer);
        return false;
    }
   
	if(c == 127 || c == 8)      //Backspace character handles erasing.
	{
    //    enter = false;
		if((int)buffer.length()>0) {
			buffer.erase(buffer.begin()+(int)buffer.size()-1);
			printf("\b \b\b \b\b \b");
		}else printf("\b \b\b \b");
        if((int)commands.size()>0) 
            commands.back() = buffer;
        it = commands.end(); it--;
		return false;
	}
    
    buffer += c;                    //If not of the above, then push the current character to the keyboard buffer.
    int sz = (int) commands.size();
    if(sz>=csize)
           commands.pop_front();
    if(enter)
    { commands.push_back(buffer); enter=false; }
    else commands.back() = buffer;
    it = commands.end(); it--;
	return false;
}


void execute(const string& file) {              
	int pid = fork();                       //Create a copy of the working process.
	if(!pid) {	
		char* op = (char*)file.c_str();
		char** argv;

		string s;
		vector<string> args; 
		while(ss>>s)
			args.push_back(s);
		if((int)args.size()>0 && args[(int)args.size()-1] == "&") args.pop_back();
		int Size = (int) args.size()+2;
		argv = new char* [Size];
		argv[0] = op; 
		for(int i=1;i<Size-1;i++)
			argv[i] = (char*) args[i-1].c_str();
		argv[Size-1] = NULL; 
        
		if(execvp(op,argv)==-1)         //execvp() resets the signals to their defaults in the children processes, so SIGINT in the child will terminate it.
		{
            string str = "tash: "+file+": command not found";
			perror(str.c_str());
			exit(0);
		}
	}
	else 
	{
		string s;
		while(ss>>s);
		if(s=="&") {
			int stats; pid = waitpid(pid,&stats,WNOHANG);               //Do not wait for the child to finish.
		} else {
			int stats; pid = waitpid(pid,&stats,0);                     //Wait for the child to finish.
		}
	}
}

char* init_path() {
	int bufsize;
	char buffer[bufsize];
	struct passwd *pwd;
	if ((pwd=getpwuid(getuid()))==NULL)
		abort();
	my_chdir(pwd->pw_dir);	            //Get the home directory of the logged in user, and change directory to it.
    return pwd->pw_dir;
}
