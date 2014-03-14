
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
void bankerAllocation(int pid);
void detectionAllocation(int pid);
void (*allocation)(int);


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

	 max = malloc(sizeof(int *) * no_proc);
	 for (i = 0; i < no_proc; ++i){		
	 	max[i] = malloc(no_res * sizeof(int));
	 	for (j = 0; j < no_res; ++j){
	 		max[i][j] = rand()%range_res;
	 	}
	 }

	 need = malloc(sizeof(int *) * no_proc);
	 for (i = 0; i < no_proc; ++i){		
	 	need[i] = malloc(no_res * sizeof(int));
	 	for (j = 0; j < no_res; ++j){
	 		need[i][j] = max[i][j];
	 	}
	 }

	 hold = malloc(sizeof(int *) * no_proc);
	 for (i = 0; i < no_proc; ++i){		
	 	hold[i] = malloc(no_res * sizeof(int));
	 	for (j = 0; j < no_res; ++j){
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

	
	while(1){
		
		if (is_release){
			release(p_id);
		}else{
			allocation(p_id);
		}
	}
}

void release(int pid){
	pthread_mutex_lock(&critical_mutex);	

	printf("is releasing proc %d\n", pid);

	int index = rand()%no_res;
	int i;
	
	for(i = 0;i<no_res;i++){
		if(hold[pid][(index+i)%no_res] != 0){
			hold[pid][(index+i)%no_res] = 0;
			printf("is releasing res %d from proc %d, total \n", (index+i)%no_res, pid,hold[pid][(index+i)%no_res]);
			break;
		}
	}

	pthread_mutex_unlock(&critical_mutex);
}

void bankerAllocation(int pid){
	pthread_mutex_lock(&critical_mutex);

	int request_no = rand()%range_res;
	int request_res = rand()%no_res;
	
	step_1:
	if (request_no > need[pid][request_res])
	{
		printf("error, impossible request amount\n");
		goto banker_end;
	}

	step_2:
	if (request_no > avail[request_res])
	{
		/* code */
	}

	banker_end:
		pthread_mutex_unlock(&critical_mutex);
}

void detectionAllocation(int pid){
	pthread_mutex_lock(&critical_mutex);

	pthread_mutex_unlock(&critical_mutex);
}
