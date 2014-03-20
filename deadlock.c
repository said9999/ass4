
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

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
int *finish;

pthread_cond_t  condition_var   = PTHREAD_COND_INITIALIZER;

double getTime(){
	struct timeval time;

	gettimeofday(&time, NULL);
	return (time.tv_sec + time.tv_usec/1000000.0);
}

int* requestGenerator(int pid){
	int *request = (int *)malloc(sizeof(int) * no_res);

	int i;
	for(i = 0;i<no_res;i++){
		request[i] = rand() % max[pid][i]; 
	}

	return request;
}

int isSafe(int pid);

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

	avail = malloc(sizeof(int) * no_res);
	for(i=0;i<no_res;i++){
		avail[i] = rand()%range_res+1;
	}

	 max = malloc(sizeof(int *) * no_proc);
	 for (i = 0; i < no_proc; ++i){		
	 	max[i] = malloc(no_res * sizeof(int));
	 	for (j = 0; j < no_res; ++j){
	 		max[i][j] = rand()%avail[j]+1;
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
	
	while(1){
		int is_release = rand()%2;

		if (is_release){
			release(p_id);
			sleep(1);
		}else{
			allocation(p_id);
			sleep(1);
		}
	}
}

void release(int pid){
	pthread_mutex_lock(&critical_mutex);	

	printf("is releasing proc %d\n", pid);

	int i;
	
	for(i = 0;i<no_res;i++){
		//int seed = rand()%2;
		if(hold[pid][i] != 0 ){
			avail[i] += hold[pid][i];
			need[pid][i] += hold[pid][i];
			hold[pid][i] = 0;

			printf("is releasing res %d from proc %d, total \n", i, pid);
			break;
		}
	}

	pthread_mutex_unlock(&critical_mutex);
	pthread_cond_signal(&condition_var);
}

void bankerAllocation(int pid){
	pthread_mutex_lock(&critical_mutex);

	int *request;
	request = requestGenerator(pid);
	
	int i;	
	banker_step_1:
		for(i=0;i<no_res;i++){
			if (request[i] > need[pid][i]){
				goto banker_exit;
			}
		}

	banker_step_2:
		for(i=0;i<no_res;i++){
			if (request[i] > avail[i]){
				//wait
				pthread_cond_wait( &condition_var, &critical_mutex );
				goto banker_step_1;
			}
		}

	banker_step_3:
		for(i = 0; i < no_res; i++){
			avail[i] = avail[i] - request[i];
			hold[pid][i] = hold[pid][i] + request[i];
			need[pid][i] = need[pid][i] - request[i]; 
		}

		if(!isSafe(pid)){
			for(i = 0; i < no_res; i++){
				avail[i] = avail[i] + request[i];
				hold[pid][i] = hold[pid][i] - request[i];
				need[pid][i] = need[pid][i] + request[i]; 
			}
			//wait
			pthread_cond_wait( &condition_var, &critical_mutex );
			goto banker_step_1;
		}

	banker_exit:
		pthread_mutex_unlock(&critical_mutex);
		pthread_cond_signal(&condition_var);
}

int isSafe(int pid){
	safe_step_1:
		work = (int *)malloc(sizeof(int) * no_res);
		finish = (int *)malloc(sizeof(int) * no_proc);

		int i;
		for(i=0;i<no_res;i++){
			work[i] = avail[i];
		}

		for(i=0;i<no_proc;i++){
			finish[i] = 0;
		}

	safe_step_2:
		for(i=0;i<no_proc;i++){
			if(finish[i] == 0 && need[pid][i] <= work[i]){
				goto safe_step_3;
			}
		}
		goto safe_step_4;

	safe_step_3:
		work[i] += hold[pid][i];
		finish[i] = 1;
		goto safe_step_2;

	safe_step_4:
		for(i = 0;i<no_proc;i++){
			if(finish[i] == 0){
				return 0;
			}
		}

		return 1;
}

void detectionAllocation(int pid){
	pthread_mutex_lock(&critical_mutex);

	//request resource allocation
	int **request;
	int i,j;

	request = malloc(sizeof(int *) * no_proc);
	 for (i = 0; i < no_proc; i++){		//Question: Is the pid the same as i, starting from 0 to 4?
	 	request[i] = requestGenerator(i);
	 	}
	 }

	 //for(i = 0; i < no_proc; i++){

	 //check whether there are enough resources available
	 for(j=0;j<no_res;j++){
		if (request[i][j] > avail[j]){
			//wait
			pthread_cond_wait( &condition_var, &critical_mutex );
	 }

	 //resource allocation 
	 for(j = 0; j < no_res; i++){
			avail[j] = avail[j] - request[i][j];
			hold[i][j] = hold[i][j] + request[i][j];
			//need[pid][i] = need[pid][i] - request[i]; 
		}

	  //deadlock detection

 	detection_step_1:

 	  work = (int *)malloc(sizeof(int) * no_res);
	  finish = (int *)malloc(sizeof(int) * no_proc);

	  for(j=0;j<no_res;j++){
			work[j] = avail[j];
		}

	  for(i=0;i<no_proc;i++){
	  		if(avail[j]!=0){
	  			finish[j] = FALSE;
	  		}
			else{
				finish[j]= TRUE;
			}
		}	

	  detection_step_2:

	  	for(i=0;i<no_proc;i++){
			if(finish[i] == 0 && need[pid][i] <= work[i]){
				goto safe_step_3;
			}
		}

	  detection_step_3:

	}


	pthread_mutex_unlock(&critical_mutex);
	pthread_cond_signal(&condition_var);
}
