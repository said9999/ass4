
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int no_proc;
int strategy;
//int resource_type;
//int no_reusable;
//int no_consumable;
int no_res;
int range_res;
//int range_prc;

int **max;
int **need;
int **hold;
int *avail;
int *work;

pthread_mutex_t critical_mutex = PTHREAD_MUTEX_INITIALIZER;

void input();
void *commandGenerator(void *id);
void release(int pid);
void bankerAllocation();
void detectionAllocation();
void (*allocation)();


int main(int argc, char *argv[]){
	int i,
		j;

	input();

	//test data
	no_proc = 5;
	strategy = 0;
	no_res = 4;
	range_res = 2;

	//INIT PART
	 srand(time(NULL));
	 pthread_t childThread[no_proc];

	 if (strategy == 0){
	 	allocation = &bankerAllocation;
	 }else{
	 	allocation = &detectionAllocation;
	 }

	 max = malloc(sizeof(* int) * no_proc);
	 for (i = 0; i < no_proc; ++i){		
	 	max[i] = malloc(no_res * sizeof(int));
	 	for (j = 0; j < count; ++j){
	 		max[i][j] = rand()%range_res;
	 	}
	 }

	 need = malloc(sizeof(* int) * no_proc);
	 for (i = 0; i < no_proc; ++i){		
	 	need[i] = malloc(no_res * sizeof(int));
	 	for (j = 0; j < count; ++j){
	 		need[i][j] = max[i][j];
	 	}
	 }

	 hold = malloc(sizeof(* int) * no_proc);
	 for (i = 0; i < no_proc; ++i){		
	 	hold[i] = malloc(no_res * sizeof(int));
	 	for (j = 0; j < count; ++j){
	 		hold[i][j] = 0;
	 	}
	 }

	avail = malloc(sizeof(int) * no_res);
	for(i=0;i<no_res;i++){
		avail[i] = range_res;
	}

	//CREATE PROC
	for(i = 0; i < no_proc; i++){
		pthread_t thread_c;
		int *tmp = malloc(sizeof(*tmp));
		*tmp = j;
		
		pthread_create( &thread_c, NULL, &commandGenerator, (void *) tmp);
		childThread[j] = thread_c;
	}

	//JOIN PROCS
	for(i = 0; i< no_proc; i++){
		pthread_join(childThread[i],NULL);
	}

	return 0;
}

void input(){

}

void *commandGenerator(void *id){
	int p_id = *((int *)id);

	int is_release = rand()%2;

	if (is_release){
		release(p_id);
	}else{

	}

	while(true){

	}
}

void release(int pid){
	pthread_mutex_lock(&critical_mutex);	

	int index = rand()%no_res;
	int i;
	
	for(i = 0;i<no_res;i++){
		if(hold[pid][(index+i)%no_res] != 0){
			hold[pid][(index+i)%no_res] = 0;
			break;
		}
	}

	pthread_mutex_unlock(&critical_mutex);
}
