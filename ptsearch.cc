
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
mutex change_mutex;

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

typedef struct kmp{
public:
	std::vector<int>* pref = nullptr;
	std::string* search = nullptr;
	std::string* path = nullptr;
	bool uses = 0;
	} kmp;
	
	
int thread_count = 0;

void *foo(void * a){
	
	kmp * kmpp = (kmp*)a;
	std::vector<int>* pref = kmpp->pref;
	std::string* s = kmpp->search;
	string path = *(kmpp->path);
	
	int n = s->length();
	
	int pos = 0;
	size_t begin = 0;
	int line = 1;
	char * buf;
	int file;
	size_t siz;
	
	FILE * file_s = fopen(path.c_str(), "r");
	if(!file_s){
		write_mutex.lock();
		printf("can not open %s \n", path.c_str());
		write_mutex.unlock();
		goto foo_b;
	}
	fseek(file_s,0, SEEK_END);
	siz = ftell(file_s);
	fseek(file_s, 0, SEEK_SET);
	fclose(file_s);
	
	file = open(path.c_str(), O_RDONLY);
	if(file < 0){
		write_mutex.lock();
		printf("can not open %s \n", path.c_str());
		write_mutex.unlock();
		goto foo_b;
	}
	
	buf = (char*)mmap(NULL, siz, PROT_READ, MAP_PRIVATE, file, 0);
	for(size_t i = 0; i < siz; ++i){
		if( buf[i] == '\n') line ++, begin = i + 1, pos = 0;
		while(pos > 0 && (*s)[pos] != buf[i]) pos = (*pref)[pos-1];
		if ((*s)[pos] == buf[i]){
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
foo_b:
	change_mutex.lock();
	kmpp->uses = false;
	thread_count--;
	change_mutex.unlock();
	
	return NULL;
	}
	
	
vector<kmp>   argss;
vector<string> strs;
vector<pthread_t> threads;
vector<bool> firstly;
void check(std::string path, kmp kmpp,const int max_pth){
	
//	printf("------%d %d \n", argss.size(), strs.size());
	static bool first = true;
	if(first){
		thread_count = 0;
		argss.resize(max_pth, {nullptr,nullptr,nullptr,false});
		strs.resize(max_pth);
		threads.resize(max_pth);
		firstly.resize(max_pth);
		for(int i = 0; i < max_pth; ++i) firstly[i] = true;
		first = false;
	}
	while(thread_count == max_pth){
		usleep(100);
		}
		
	int krug = 0;
	while(argss[krug].uses){
		++krug;
		
	}
	strs[krug] = path;
	argss[krug] = kmpp;
	argss[krug].path = &strs[krug];
	change_mutex.lock();
	pthread_t a;
	int code = pthread_create(&a, NULL, foo, (void *) &argss[krug]);
	if(code != 0){
		perror("не создался поток:");
		change_mutex.unlock();
		return;
		}
	if(!firstly[krug]) pthread_join(threads[krug], NULL);
	threads[krug] = a;
	firstly[krug] = false;
	argss[krug].uses = true;
	thread_count++;
	change_mutex.unlock();
	
	return;
	}

void walk_rec(std::string dirname,bool rec, const kmp& kmpp, const int max_pth){
	
	DIR* dir = opendir(dirname.c_str());
	if(dir == nullptr){
		return;
		}
	for(dirent* de = readdir(dir); de != NULL; de = readdir(dir)) {
		if (strcmp(de->d_name, ".") == 0 or strcmp(de->d_name, "..") == 0)
            continue;
        std::string ss = dirname + "/" + de->d_name;
        if( de-> d_type == DT_REG) check(ss, kmpp, max_pth);
      //  printf("%s %d\n", ss.c_str(), de-> d_type == DT_REG);
        if (de->d_type == DT_DIR and rec == true) {
			//printf("dd\n");
            walk_rec(ss, rec, kmpp, max_pth);
        }
        
		//можно отсюда начинать поиск
		}
	closedir(dir);
	}
	
	
struct vyz{
	string * dirname;
	bool recur;
	kmp k;
	int max_pth; 
	};
	
void procces__(vyz& a){
	//printf("%p %p %p %p %p\n", a.dirname, &a.dirname, &a.recur, &a.k, &a.max_pth);
	walk_rec(*(a.dirname),a.recur, a.k, a.max_pth);
	for(size_t i = 0 ; i < firstly.size(); ++i){
		if(!(firstly[i])){
			pthread_join(threads[i], NULL);
		}
	}
	return ;
	}



int main(int argc, char** argv){
	std::string path = ".";
	std::string search_str;
	vector<int> pref;
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
	pref = prefix_function(search_str);
	
	kmp a = {&pref, &search_str, nullptr, false};
	vyz tt = {&path,recur, a, nums};
	procces__(tt);
	
	argss.clear();
	strs.clear();
	
	return 0;
	}

