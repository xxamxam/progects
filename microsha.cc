#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <limits.h>
#include <set>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>
 #include <pwd.h>

using namespace std;

char current_path[PATH_MAX] = { 0 };
set<string> int_commands = { "cd", "pwd", "time" };

using namespace std;
//обработка рег выр

bool reg_v(const char* str, const char* vyr){
    if (*str == '\0') {
        if (*vyr != '\0'){
			if(*vyr == '*')
				return reg_v(str, vyr + 1);
			else
                return false;
        }
        return true;
    }
    if (*str == *vyr or *vyr == '?')
        return reg_v(str + 1, vyr + 1);
    if (*vyr == '*') {
        int i = -1;
        do {
            ++i;
            if (reg_v(str + i, vyr + 1))
                return true;

        } while (str[i] != '\0');
    }
    return false;
}


vector<string> walker(string const dirname, bool is_dir, bool home, bool dot, char * reg_vv){
	vector<string> ret;
	DIR* dir = opendir(dirname.c_str());
	if(dir == nullptr){ 
		printf("foo");
		return ret;
		}
	
	for( dirent* de = readdir(dir); de != NULL; de = readdir(dir)){
		bool rv = reg_v(de->d_name,reg_vv);
		if(!dot and de->d_name[0] =='.') continue; 
		if(is_dir and de-> d_type == DT_DIR and rv ){
			if(home) ret.push_back(de->d_name + string("/"));
			else ret.push_back(dirname + de->d_name + string("/"));
			}
		else if(rv and !is_dir){
			if(home) ret.push_back(de->d_name );
			else ret.push_back(dirname + de->d_name);
			}
		}
    closedir(dir);
	return ret;
	}
	
	
int add_this(vector<string>& argss, char* s){
	int i = 0;
	int st_pos = 0;
	vector<string> rez;
	bool home = true;
	bool dot = false;
	if(s[0] == '/'){
		++i, ++st_pos;
		home = false;
		rez.push_back("/");
	}
	else rez.push_back("./");
	if(s[0] == '.') dot = true;
	while(s[i] != '\0'){
		while(s[i] != '\0' and s[i] != '/') ++ i;
		if(s[i] == '\0') break;
		s[i++] = '\0'; 
		vector <string> tmp;
		dot = false;
		if(s[st_pos] == '.') dot = true;
		for(size_t n = 0 ; n < rez.size(); ++n){
			vector<string> tmp1 = walker(rez[n], true, home, dot, s + st_pos);
			tmp.insert(tmp.end(), tmp1.begin(), tmp1.end());
			}
		home = false;
		rez = tmp;
		s[i-1] = '/';
		st_pos = i;
	}
	if(st_pos != i){
		vector <string> tmp;
		dot = false;
		if(s[st_pos] == '.') dot = true;
		for(size_t n = 0 ; n < rez.size(); ++n){
			vector<string> tmp1 = walker(rez[n], false, home, dot, s + st_pos);
			tmp.insert(tmp.end(), tmp1.begin(), tmp1.end());
			}
		home = false;
		rez = tmp;
		}
	int rezult = rez.size();
	for(size_t i = 0; i <  rez.size(); ++i) argss.push_back(rez[i]);
	if(rezult == 0) argss.push_back(s), ++rezult;
	return rezult;
	}

// обработка строки и вспомогательные функции

int get_line(string& s){
    char c;
    while ((c = getchar()) == ' ' or c == '\t'){
    }
    if (c == '\n')
        return 1;
    if (c == EOF) return 2;
    s += c;
    while ((c = getchar()) != ' ' and c != '\n' and c!= EOF)
        s += c;
    if (c == ' ')
        return 0;
    if(c == EOF) return 2;
    return 1;
}

struct kvart {
    short a[5];
};
struct triple {
    bool a;
    int num, pos;
};

// ссылка на описание автомата
// https://photos.google.com/share/AF1QipNNVOwa27BOWmWKarusBxji_8EES91UVZPCSgkv5SW8J3q0TcMyss0Og6lvftQ_wA?pli=1&key=dW1JVzNLUEZmWTZ4X0lYTjY3V1hTRERfdEdrdW9B
short str_checker(short pos, short symb){
    static const struct kvart avto1[13] = {
        { 1, -1, -1, -1, 12 }, { 1, 2, 6, 10, -1 }, { 3, -1, -1, -1, -1 },
        { -1, -1, 4, -1, -1 }, { 5, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1 },
        { 7, -1, -1, -1, -1 }, { -1, 8, -1, 10, -1 }, { 9, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1 }, { 11, -1, -1, -1, -1 }, { 11, 8, -1, 10, -1 },
        { 1, -1, -1, -1, -1 }
    };
    return avto1[(size_t)pos].a[(size_t)symb];
}

bool ended(short c){
    if (c == 0 or c == 1 or c == 3 or c == 7 or c == 5 or c == 9 or c == 11) return true;
    return false;
}

short num(string& s)
{
    const char* ss = s.c_str();
    if (strcmp(ss, ">") == 0)
        return 1;
    if (strcmp(ss, "<") == 0)
        return 2;
    if (strcmp(ss, "|") == 0)
        return 3;
    if (strcmp(ss, "time") == 0)
        return 4;
    return 0;
}
bool my_strstr(string& s)
{
    for (size_t i = 0; s[i] != '\0'; ++i) {
        if (s[i] == '*' or s[i] == '?')
            return true;
    }
    return false;
}

// получение и разбивка строки

// получает на вход место где хранить строки и ещё два вектора
// redir показывает где есть символы < >
// conveer показывает где есть |
   //correct   time
pair<int, bool> parce(vector<string>& vargs, vector<vector<char*>>& args,
    vector<triple>& redir, vector<short>& conveer)
{
    int i = 0;
    short checker = 0;
    bool time = false;
    for (;;) {
        string s;
        int check = get_line(s);
        if(check == 2){
             return {2,0};
        }
        if (s.empty())
            break;

        short numm = num(s);
        if (numm == 3)
            conveer.push_back(i);
        else if(numm == 4) time = true;   
        else if (numm == 1 or numm == 2) {
            triple aa = { (bool)(numm - (short)1), (int)conveer.size(),
                i - (conveer.size() > 0 ? conveer[conveer.size() - 1]: 0) };     
            redir.push_back(aa);
        }
        else if (my_strstr(s)) {
            i += add_this(vargs, (char*)s.c_str());

        }     
        else vargs.push_back(s), ++i;
        if ((checker = str_checker(checker, numm)) == -1) {
            printf(" ошибка в %d : %s слове\n", i, s.c_str());
            return {0, time};
        }

        if (check == 1)
            break;
    }
    if (!ended(checker) or vargs.size()== 0) {
        printf(" наберите правильно\n");
        return {0, time};
    }

    for (size_t k = 0; k < conveer.size() + 1; ++k) {
        args.push_back({});
        size_t to, i;
        k == conveer.size() ? to = vargs.size() : to = conveer[k];
        k == 0 ? i = 0 : i = conveer[k - 1];
        for (; i < to; ++i) {
            args[k].push_back((char*)vargs[i].c_str());
        }
        args[k].push_back(nullptr);
    }
    return {1, time};
}


// выполнение функции и конвеер

inline int funk(vector<vector<char*>>& args, int j)
{
    if (strcmp(args[j][0], "pwd") == 0)
        printf("%s\n", current_path);
    else if (strcmp(args[j][0], "echo") == 0) {
        int i = 1;
        while (args[j][i])
            printf("%s ", args[j][i++]);
        printf("\n");
    } else {
        int t = execvp(args[j][0], &args[j][0]);
        if (t == -1)
            perror(args[j][0]);
            exit(-1);
    }
    return 0;
}


int redirs(vector<vector<char*>>& args, const vector<triple>& redir, bool ins, bool outs){
	int in = -1, out = -1;
		for (size_t i = 0; i < redir.size(); ++i) {
			char* b = args[redir[i].num][redir[i].pos];
			if (redir[i].a and ins) {
				in = open(b, O_RDONLY);
				if (in < 0) {
					perror(b);
					return -1;
				}
				dup2(in, 0);
			} else if(! redir[i].a and outs){
				out = open(b, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				if (out < 0) {
					perror(b);
					return -1;
				}
				dup2(out, 1);
			}
			args[redir[i].num][redir[i].pos] = nullptr;
		}
	return 0;
	}

int dode(vector<vector<char*>>& args, const vector<triple>& redir)
{
    int n = args.size();
    if (n == 1) {
        pid_t pid = fork();
        if (pid == 0) {
			
			redirs(args, redir, 1, 1);
            funk(args, 0);
            return 0;

        } else if (pid > 0) {
            int status;
            pid = wait(&status);
            if (status != 0) {
                //perror("ошибка12");
                exit(1);
            }
            return 0;
        }
    }

    int fd[2][2];
    bool pred = false;
    pipe(fd[0]);
    pid_t pid = fork();
    if (pid == 0) {
		redirs(args, redir, 1, 0);
        close(fd[0][0]);
        dup2(fd[0][1], 1);
        funk(args, 0);
        return 0;

    } else {
        int i = 1;
        for (i = 1; i < n - 1; ++i) {
            close(fd[pred][1]);
            pipe(fd[!pred]);
            pid = fork();
            if (pid == 0) {
                dup2(fd[pred][0], 0);
                close(fd[!pred][0]);
                dup2(fd[!pred][1], 1);

                funk(args, i);
                return 0;
            }
            close(fd[pred][0]);
            pred = !pred;
        }
        close(fd[pred][1]);
        pid = fork();
        if (pid == 0) {
			redirs(args, redir, 0, 1);
            dup2(fd[pred][0], 0);

            funk(args, i);
            exit(0);
        } else if (pid > 0) {
            int status;
            pid = waitpid(pid, &status, 0);
            if (status != 0) {
               // perror("ошибка");
                exit(1);
            }
        }
    }
    return 0;
}

//доделать
bool init(void)
{
    uid_t id = getuid();
    struct passwd * userinfo = getpwuid(id);
    char c = '>';
    if (userinfo->pw_uid == 0)
        c = '!';
    
    printf("%s:%s %c", userinfo->pw_name, current_path, c);
    return 0;
}



void sigfunc(int val) {
	//printf("\n");
}
void sigfunc2(int val) {
	printf("\n");
}


int main(int argc, char** argv){
    char* P;
    P = getcwd(current_path, PATH_MAX);
    if (P == nullptr) {
        printf("ah shit we have broken\n");
        exit(1);
    }
	
	signal(SIGINT, sigfunc);
    pid_t pid;    
    vector<string> vargs;
    vector<vector<char*>> args;
	vector<short> conveer;
	vector<triple> redir;
	bool time;
	pair<int, bool> parc;
	struct timeval first_t, second_t;
    struct timezone tz;

a:	
	init();
	vargs.clear();
	args.clear();
	conveer.clear();
	redir.clear();
	parc = parce(vargs, args, redir, conveer);
     if (parc.first == 2) {
        vargs.clear();
	    args.clear();
	    conveer.clear();
	    redir.clear();
        return 0;
    }
    if (parc.first == 0) goto a;
	time = parc.second;
	if(time)  gettimeofday(&first_t, &tz);
	if (strcmp(args[0][0], "cd") == 0) {
		if(conveer.size()){
			printf("cd не используется в конвеере\n");
		} else {
			if(args[0].size() == 2){
				vargs.push_back(getenv("HOME"));
				args[0][1] = (char*)vargs[vargs.size() - 1].c_str();
				}
			chdir(args[0][1]);
			getcwd(current_path, PATH_MAX);
			}
		}
	else{
		pid = fork();
		if (pid == 0) {
			signal(SIGINT, sigfunc2);
			
			dode(args, redir);
			

		return 0;
		} else {
			signal(SIGINT, sigfunc);
			int status;
			pid = wait(&status);
			
			if (status != 0 and status != 2) {
				//printf("--%d\n", status);
				//perror("ошибка вызова");
			}
            if(time){
				struct rusage rus;
                gettimeofday(&second_t, &tz);
				if ( getrusage(RUSAGE_CHILDREN, &rus) != -1 ){
					printf("all: %lf \nsys : %lf\nuser: %lf\n",
                     ((double)second_t.tv_sec + (double)second_t.tv_usec/1000000.0 - (double)first_t.tv_sec - (double)first_t.tv_usec/1000000.0) ,
					(double)rus.ru_stime.tv_sec + (double)rus.ru_stime.tv_usec/1000000.0,
					(double)rus.ru_utime.tv_sec+  (double)rus.ru_utime.tv_usec / 1000000.0);
				}
			}
		}	
	}
    goto a;
}
