#include <iostream>           // std::cout
#include <thread>   
#include <vector>
#include <queue>      
#include <map>      // std::thread
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


std::mutex fille;

inline int day_to_int(std::string a){
	return ((a[0]-'0')*10 + a[1]-'0')*3600 + ((a[3]-'0')*10 + a[4]-'0')*60 + ((a[6]-'0')*10 + a[7]-'0');
}
std::mutex print_mut;

			// едет            стоит
		// вверх вниз открыт закрыт закрывается  
enum state{UP, DOWN, OPEN, CLOSE, IDLE};
int C, T_s, T_o, T_i, T_c, T_in, T_out;



struct node{
	int i = 0;
};

class Lift{
public:
	int state_ = CLOSE;
    int tmp = 0;
    int fill = 0;
	int to = 1;
    int floor = 1;
    int id = 0;
	std::map<int, node> pop; // на каком сколько оставить
};

class Manag{
public:
	int K = 0;
};

struct pep{
	int to;
	int when;
};
struct directions{
	std::queue<pep> up, down;
};


std::vector<directions> people;  // на каком этаже кого забрать и на каком оставить
std::vector<Lift> lift_data;
std::queue<std::pair<int,pep>> que; // очеередб в которую кладутся заяуки для которых не нащелся исполнитель


int get_lift(int from, int to, int k){
	int rez = -1;
	bool up = to > from;
	int minn = -1;

	for(int i = 0; i < k; ++i){
		if((minn == -1 or minn > abs(lift_data[i].floor - from))  and ((lift_data[i].state_ != DOWN and lift_data[i].state_ != UP) or (up and (lift_data[i].state_ == UP and lift_data[i].floor <= from))
		or (!up and (lift_data[i].state_ == DOWN and lift_data[i].floor >= from)))){
			
			rez = i;
			minn = abs(from - lift_data[i].floor);
			printf("CHOOSE %d with %d\n", i, minn);
		}
	}
	printf("CHOOSEN %d with %d\n", rez, minn);

	return rez;
}

int msg_m;
int msg_l;
int timem = 0;

void manager_ff(int k){
	struct msgbuf mbuf = {1 , 'a'};
	struct msgbuf mbuf_1;
	bool firstly = true, end = false;	
	std::string time_s, from_s, to_s;
	int at = 0;

    while(true){

		// print_mut.lock();
		printf("now is %d\n", timem);
		// print_mut.unlock();
		int which = -1;
		while(!que.empty()){
			std::pair<int, pep> tmp = que.front(); 
			which = get_lift(tmp.first, tmp.second.to, k);
			if(which == -1) {
				which = -2;
				break;
			}else{
				if((lift_data[which].state_ != UP and lift_data[which].state_ != DOWN) or (lift_data[which].state_ == UP and lift_data[which].to < tmp.first) or (lift_data[which].state_ == DOWN and lift_data[which].to > tmp.first) ){
					lift_data[which].to = tmp.first;
					if(tmp.first > tmp.second.to) people[tmp.first].down.push(tmp.second);
					else people[tmp.first].up.push(tmp.second);
				}
				que.pop();
			}
			
		}

		if(firstly){
			std::cin>>time_s;
			at = day_to_int(time_s);
			timem = at;
			firstly = false;
		}

		while(at == timem and which != -2 and !end){

			std::cin>>from_s>>to_s;
			int from = atoi(from_s.c_str());
			int to = atoi(to_s.c_str());
			int which = get_lift(from, to, k);
			if(which == -1){
				pep tmp =  {to, at};
				que.push({from, tmp});
				which = -2;
			}
			else{
					lift_data[which].to = from;
					std::cout<< from << " "<<to<<"\n";
					sleep(1);
					pep tmp =  {to, at};
					if(from > to) people[from].down.push(tmp);
					else people[from].up.push(tmp);
				
			}
			std::cin>>time_s;
			if(time_s == "end"){
				end = true;
			}
			at = day_to_int(time_s);
		}
bbb:
		for(int i = 0; i < k; ++i){
			mbuf.mtype = i + 1;
			if(msgsnd(msg_l, &mbuf, 1, IPC_NOWAIT) == -1){
				perror("manager_send");
				exit(1);
			}
		} 
        for(int i = 0; i < k; ++i){
			if(msgrcv(msg_m, &mbuf_1,1,1, 0) == -1){
				perror("manager_reseive");
				exit(1);

			}
        }
		
		if(timem == 24*3600) timem = 0;
		timem +=1;
        

    }
}


void* manager_f(void * a){
    Manag* M = (Manag*)a;
	int k = M->K;
	try{
		manager_ff(k);
	} catch(...){
		printf("catch;\n");
		msgctl(msg_l, IPC_RMID, 0);
		msgctl(msg_m, IPC_RMID, 0);
	}
	return NULL;
}
 

void fill(Lift* data, std::queue<pep>& qu){
	if(data->state_ == CLOSE or data->state_ == UP or data->state_ == DOWN) data->tmp += T_o + T_c;
	while(!qu.empty() and data->fill != C){
		int a = qu.front().to;
		qu.pop();
		data->pop[a].i += 1;
		data->tmp += T_in; 
		data->fill += 1;
		if((data->state_ == UP and a > data->to) or (data->state_ == DOWN and a < data->to)) data->to = a;
	}
}

void clear( std::queue<pep> &q )
{
   std::queue<pep> empty;
   std::swap( q, empty );
}

void lift_ff(Lift* data){
	struct msgbuf mbuf = {1 , 'a'};
	struct msgbuf mbuf_1;
    
	int napr = -1;
	int filling = -1;
	

    while (true){ 
		
		if(msgrcv(msg_l, (void*)&mbuf_1,1,data->id + 1,0) == -1){
			perror("lift_r");
			exit(2);
		}

		// print_mut.lock();
		printf("lift %d   at %d   fill %d   waits %d   needs %d \n", data->id, data->floor,data->fill, data->tmp,data->to);
	    // print_mut.unlock();
		if(data->tmp != 0) data->tmp -=1;
		else{
			
			if(data->pop[data->floor].i != 0){
			print_mut.lock();
	
			print_mut.unlock();

				napr = -1;
				if(data->state_ == UP or data->state_ == DOWN){
					napr = data->state_;
					data->state_ = CLOSE;
				}
				if(data->state_ == CLOSE){
					data->tmp = T_o;
					data->state_ = OPEN;
				}
				if(data ->state_ == OPEN){
					data->tmp += data->pop[data->floor].i * T_out;
					data->fill -= data->pop[data->floor].i; 
					data->pop[data->floor].i = 0;
					if(napr > 0 and data->fill != 0){
						data->state_ = napr;
						
					}
					else {
						data->state_ = CLOSE;
						data->tmp +=T_c;
					}
				}
			}
		

			fille.lock();

// 			if(filling == UP){
// `
// 			} else if(filling == DOWN){


// 			}

			if(data->to == data->floor) data->state_ = CLOSE;

			if(data->state_ != UP and data->state_ != DOWN){
				
				int to;
				if((to = data->to - data->floor) > 0){
					
					data->state_ = UP; 
				} else if(to < 0){
					data->state_ = DOWN;
					
				} else{
					
					if (!people[data->floor].up.empty() and !people[data->floor].down.empty() and people[data->floor].up.front().when > people[data->floor].down.front().when){
						data->state_ = DOWN;
						fill(data, people[data->floor].down);
						
						
								
					} else if(!people[data->floor].up.empty()){
						data->state_ = UP;
						fill(data, people[data->floor].up);	 
						
						
						}
					clear(people[data->floor].down);
					clear(people[data->floor].up);
					}
				
			}
			// printf("%d %d %d \n", data->state_, data->floor, !people[data->floor].down.empty());
			
			if(data->state_ == UP and !people[data->floor].up.empty()){
				fill(data, people[data->floor].up);
				clear(people[data->floor].up);
				

			}else if(data->state_ == DOWN and !people[data->floor].down.empty()){

				fill(data, people[data->floor].down);
				clear(people[data->floor].down);
				
			}

			if(data->state_ == UP){
			
				data->tmp += T_s;
				data->floor +=1;
			} else if(data->state_ == DOWN){
				data->tmp += T_s;
				data->floor -=1;
			}
			fille.unlock();
		}
		
		if(msgsnd(msg_m, &mbuf,1, IPC_NOWAIT) == -1){
			perror("lift_get");
			exit(2);
		}
		if(timem == 24*3600){
			break;
		}
    }

}

void* lift_f(void * a){
    Lift* data = (Lift*)a;
	try{
		lift_ff(data);
	} catch(...){
		printf("catch;\n");
		msgctl(msg_l, IPC_RMID, 0);
		msgctl(msg_m, IPC_RMID, 0);
	}
	return NULL;
}



int main(){
    int N, K;
    std::cin>>N>>K >> C>> T_s>> T_o>> T_i>> T_c>> T_in>> T_out;
	std::cout<<"st\n";




	key_t first = ftok(".", 'm');
	key_t second = ftok(".", 'n');
	if(first == -1 or second == -1){
		perror("key:");
		exit(-1);
	}
	if((msg_m = msgget(first, IPC_CREAT | 0666)) == -1) perror("msg_m");
	if((msg_l = msgget(second, IPC_CREAT|  0666)) == -1) perror("msg_l");


	std::vector<pthread_t> lift(K);
	people.resize(N + 1);
	lift_data.resize(K);
    Manag a = {K};
	pthread_t manager;
    

    for(int i = 0; i < K; ++i){
        lift_data[i].id = i;
        if( pthread_create(&lift[i], NULL, lift_f, (void*)&lift_data[i]) != 0){
            printf("не создался %d лифт\n", i);
        }
    }
	if(pthread_create(&manager, NULL, manager_f, (void*)&a) != 0){
        printf("не создался manager\n");
        return -1;
    }
    pthread_join(manager, NULL);
    for(int i = 0; i < K; ++i) pthread_join(lift[i], NULL);

	msgctl(msg_l, IPC_RMID, 0);
	msgctl(msg_m, IPC_RMID, 0);
	return 0;
}
