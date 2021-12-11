// pthread_rwlock_t

#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mutex>
#include <sys/mman.h>

#include <stack>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#ifndef BZ
#pragma GCC optimize ("Ofast")
#pragma GCC optimize ("unroll-loops")
#pragma GCC optimize ("no-stack-protector")
#pragma GCC target ("tune=native")
#pragma GCC target ("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,avx")
//#pragma GCC optimize ("conserve-stack")
//#pragma GCC optimize ("no-stack-limit")
//#pragma clang optimize on
//#pragma clang loop unroll(enable)
#endif


using namespace std;

mutex write_mutex;
mutex stack_;
int msg_snd;

stack<string> dirnames;
vector<int> pref;
string search_str;

std::vector<int> prefix_function(std::string s) {
	int n = s.length();
	std::vector<int> pi (n);
	for (int i=1; i<n; ++i) {
		int j = pi[i-1];
		while (j > 0 && s[i] != s[j])
			j = pi[j-1];
		if (s[i] == s[j])  ++j;
		pi[i] = j;
	}
	return pi;
}

void walk_rec(std::string dirname,bool rec){
	
	DIR* dir = opendir(dirname.c_str());
	if(dir == nullptr){
		return;
		}
	for(dirent* de = readdir(dir); de != NULL; de = readdir(dir)) {
		if (strcmp(de->d_name, ".") == 0 or strcmp(de->d_name, "..") == 0)
            continue;
        std::string ss = dirname + "/" + de->d_name;
        if( de-> d_type == DT_REG){
            stack_.lock();
            dirnames.push(ss);
	    stack_.unlock();
            struct msgbuf mbuf = {1 , 'a'};
            if(msgsnd(msg_snd, &mbuf, 1, IPC_NOWAIT) == -1){
				perror("manager_send");
				exit(1);
			}
            
        }
        if (de->d_type == DT_DIR and rec == true) {
            walk_rec(ss, rec);
        }
	}
	closedir(dir);
}

struct vyz{
	string * dirname;
    pthread_rwlock_t* pw;
	bool recur;
    int N;
};

void* filler(void* a){
    vyz* tmp = (vyz*)a;
    walk_rec(*tmp->dirname, tmp->recur);
    struct msgbuf mbuf = {1 , 'b'};
    for(int i = 0; i < tmp->N; ++i){
        if(msgsnd(msg_snd, &mbuf, 1, IPC_NOWAIT) == -1){
				perror("manager_send");
				exit(1);
			}
    }    
    return NULL;
}   

void proccess(string & path){
    string s = search_str;
    int n = s.size();

    int pos = 0;
	size_t begin = 0;
	int line = 1;
	char * buf;
	int file;

    file = open(path.c_str(), O_RDONLY);
    if(file < 0){
		write_mutex.lock();
		printf("can not open %s \n", path.c_str());
		write_mutex.unlock();
        return;
	}

    struct stat bufff;
    fstat(file, &bufff);
    size_t siz = bufff.st_size;
    buf = (char*)mmap(NULL, siz, PROT_READ, MAP_PRIVATE, file, 0);

	for(size_t i = 0; i < siz; ++i){
		if( buf[i] == '\n') line ++, begin = i + 1, pos = 0;
		while(pos > 0 && (s)[pos] != buf[i]) pos = (pref)[pos-1];
		if ((s)[pos] == buf[i]){
			++pos;	
			}
		if(pos == n){
		while(i < siz and buf[i] !='\n') ++i;
		write_mutex.lock();
		printf("in line %d from %s:\n",line, path.c_str());
		write(1, buf + begin, i - begin);
		printf("\n");
		write_mutex.unlock();
		line ++, begin = i + 1, pos = 0;
		}
	
	}

    munmap(buf, siz);
	close(file);
    return;
}


void* searcher(void* a){
    pthread_rwlock_t* lock = (pthread_rwlock_t*)a;
    string s;
   
    struct msgbuf mbuf_m;
    while(true){
        if(msgrcv(msg_snd, &mbuf_m,1,1, 0) == -1){
				perror("manager_reseive");
				exit(1);
			}
        if(*mbuf_m.mtext == 'b') break;
        stack_.lock();
        s = dirnames.top();
        dirnames.pop();
        stack_.unlock();
        proccess(s);
    }
    return NULL;
}

int main(int argc, char** argv){
	std::string path = ".";
	int nums = 1;
	bool recur = true;
	bool search_str_get = false;
	bool path_get = false;
	if(argc == 1) {printf("введите аргументы\n"); return 0;}
	for(int i = 1; i < argc; ++i){
		if(argv[i][0] == '-'){
			if(argv[i][1] == 'n') recur = false;
			else if(argv[i][1] == 't') nums = atoi(argv[i] + 2);
			else {printf("нет такого флага\n"); return 0;}
			if(nums<=0) {printf("введите количество потоков правильно: -t#\n"); return 0;}
		} else{
			if(!search_str_get) search_str = argv[i], search_str_get = true;
			else if(!path_get) path = argv[i], path_get = true;
			else {printf("слишком много аргументов\n"); return 0;}
			}		
	}

    key_t first = ftok(".", 'm');
    if(first == -1){
    	perror("key:");
		exit(-1);
    }
    if((msg_snd = msgget(first, IPC_CREAT | 0666)) == -1) perror("msg_snd");

	pref = prefix_function(search_str);

    
    pthread_rwlock_t lockk;

    vyz tt = {&path, &lockk, recur, nums};
    pthread_t a;
    if(pthread_create(&a, NULL, filler, (void*) &tt) != 0){
        printf("не создался поток\n");
        return -1;
    }
    
    vector<pthread_t> searchers(nums);
    for(int i = 0; i < nums; ++i){
        if( pthread_create(&searchers[i], NULL, searcher, (void*)&lockk) != 0){
            printf("не создался %d поток поиска\n", i);
        }
    }

    pthread_join(a, NULL);
    for(int i = 0; i < nums; ++i) pthread_join(searchers[i], NULL);
    msgctl(msg_snd, IPC_RMID, 0);
	return 0;
}
