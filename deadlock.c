#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define RUN_INTERVAL 5

int no_proc,
	strategy,
	no_res,
	range_res,
	**max,
	**need,
	**hold,
	*avail,
	*work,
	*finish,
	*is_sleep,
	**request_matrix;

double simulate_time,
		start_time,
		end_time,
		*thread_run_time,
		*thread_last_starting_time;

void (*allocation)(int);

pthread_cond_t  condition_var   = PTHREAD_COND_INITIALIZER;
pthread_mutex_t critical_mutex = PTHREAD_MUTEX_INITIALIZER;

double getTime(){
	struct timeval time;

	gettimeofday(&time, NULL);
	return (time.tv_sec + time.tv_usec/1000000.0);
}

int* requestGenerator(int pid){
	int *request = (int *)malloc(sizeof(int) * no_res);

	int i;
	for(i = 0;i<no_res;i++){
		request[i] = rand() % max[pid][i]+1; 
	}

	return request;
}

void simulateTimer(){
	start_time = getTime();

	while(1){
		if(getTime() - start_time > simulate_time){
			printf("time is up\n");
			int i;
			double thread_sum = 0.0f;
			for(i=0;i<no_proc;i++){
				if(!is_sleep[i]){
					thread_run_time[i] += getTime() - thread_last_starting_time[i];
				}

				printf("proc %d has run %lf sec\n",i,thread_run_time[i]);
				thread_sum += thread_run_time[i];
			}

			printf("there are %d proc, and simulate time %lf, total %lf\n", no_proc,simulate_time, simulate_time * no_proc);
			printf("threads sum is %lf\n", thread_sum);
			exit(1);
		}
	}
}

int isSafe(int pid);
void input();
void *commandGenerator(void *id);
void release(int pid);
void bankerAllocation(int pid);
void detectionAllocation(int pid);
int isDeadlock(int pid, int *finish_vector);
void recoverDeadlock(int pid, int *finish_vector);



int main(int argc, char *argv[]){
	int i,
		j;

	input();

	//test data
	no_proc = 5;
	strategy = 0;
	no_res = 4;
	range_res = 2;
	simulate_time = 10;

	//INIT PART
	 srand(time(NULL));
	 pthread_t childThread[no_proc];

	 if (strategy == 0){
	 	allocation = &bankerAllocation;
	 }else{
	 	allocation = &detectionAllocation;
	 }

	thread_run_time = (double *)malloc(sizeof(double) * no_proc);
	thread_last_starting_time = (double *)malloc(sizeof(double) * no_proc);
	is_sleep = (int *)malloc(sizeof(int) * no_proc);

	avail = malloc(sizeof(int) * no_res);

	printf("avail is : [");
	for(i=0;i<no_res;i++){
		avail[i] = rand()%range_res+1;
		printf("%d ",avail[i]);
	}
	printf("]\n");

	printf("max is : [");
	max = malloc(sizeof(int *) * no_proc);
	for (i = 0; i < no_proc; ++i){		
		max[i] = malloc(no_res * sizeof(int));
		for (j = 0; j < no_res; ++j){
			max[i][j] = rand()%avail[j]+1;
			printf("%d ", max[i][j]);
		}
		printf("\n");
	}
	printf("]\n");

	request_matrix = malloc(sizeof(int *) * no_proc);
	for (i = 0; i < no_proc; ++i){		
		request_matrix[i] = malloc(no_res * sizeof(int));
		for (j = 0; j < no_res; ++j){
			request_matrix[i][j] = 0;
			printf("%d ", request_matrix[i][j]);
		}
		printf("\n");
	}

	printf("need is : [");
	 need = malloc(sizeof(int *) * no_proc);
	 for (i = 0; i < no_proc; ++i){		
	 	need[i] = malloc(no_res * sizeof(int));
	 	for (j = 0; j < no_res; ++j){
	 		need[i][j] = max[i][j];
	 		printf("%d ", need[i][j]);
	 	}
	 	printf("\n");
	 }
	  printf("]\n");

	 printf("hold is : [");
	 hold = malloc(sizeof(int *) * no_proc);
	 for (i = 0; i < no_proc; ++i){		
	 	hold[i] = malloc(no_res * sizeof(int));
	 	for (j = 0; j < no_res; ++j){
	 		hold[i][j] = 0;
	 		printf("%d ", hold[i][j]);
	 	}
	 	printf("\n");
	 }
	 printf("]\n");

	 sleep(3);
	//CREATE PROC
	for(i = 0; i < no_proc; i++){
		pthread_t thread_c;
		int *tmp = malloc(sizeof(*tmp));
		*tmp = i;
		
		pthread_create( &thread_c, NULL, &commandGenerator, (void *) tmp);
		childThread[i] = thread_c;
	}

	simulateTimer();
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

	//time recording
	thread_run_time[p_id] = 0.0f;	//start time;
	thread_last_starting_time[p_id] = getTime();
	is_sleep[p_id] = 0;

	//test codes
	printf("create proc w id %d\n",p_id);
	sleep(1);

	while(1){
		int run_time = rand()%RUN_INTERVAL;
		
		//sleep(run_time);			//simulate running time
		
		allocation(p_id);	

		run_time = rand()%RUN_INTERVAL;

		//sleep(run_time);			//simulate running time

	}
}

void release(int pid){
	pthread_mutex_lock(&critical_mutex);	

	printf("is releasing proc %d\n", pid);
	printf("proc %d is holding the lock \n",pid);

	int i;
	
	for(i = 0;i<no_res;i++){
		//int seed = rand()%2;
		printf("is releasing res %d from proc%d, total %d\n", i, pid,hold[pid][i]);
		avail[i] += hold[pid][i];
		need[pid][i] += hold[pid][i];
		hold[pid][i] = 0;

		
	}

	pthread_mutex_unlock(&critical_mutex);
	printf("proc %d is releasing the lock \n",pid);
	pthread_cond_broadcast(&condition_var);
}

void bankerAllocation(int pid){
	pthread_mutex_lock(&critical_mutex);

	printf("proc %d is holding the lock\n", pid);

	int *request;
	request = requestGenerator(pid);
	
	int i;	
	banker_step_1:
		for(i=0;i<no_res;i++){
			if (request[i] > need[pid][i]){
				printf("impossible assign\n");
				sleep(1);
				goto banker_exit;
			}
		}

	banker_step_2:
		for(i=0;i<no_res;i++){
			if (request[i] > avail[i]){
				//time recording before sleep
				thread_run_time[pid] += getTime() - thread_last_starting_time[pid]; 
				is_sleep[pid] = 1;
				//wait
				printf("proc %d is going to wait\n", pid);
				
				pthread_cond_wait( &condition_var, &critical_mutex );
				//update timeing after waking up
				thread_last_starting_time[pid] = getTime();
				is_sleep[pid] = 0;

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
			printf("Not safe to allocate resource to proc %d\n",pid);
			for(i = 0; i < no_res; i++){
				avail[i] = avail[i] + request[i];
				hold[pid][i] = hold[pid][i] - request[i];
				need[pid][i] = need[pid][i] + request[i]; 
			}

			//time recording before sleep
			thread_run_time[pid] += getTime() - thread_last_starting_time[pid]; 
			//wait
			printf("proc %d is going to wait\n",pid);
			is_sleep[pid] = 1;

			pthread_cond_wait( &condition_var, &critical_mutex );

			//update timeing after waking up
			thread_last_starting_time[pid] = getTime();
			is_sleep[pid] = 0;
			goto banker_step_1;
		}else{
			printf("safe to allocate resource to proc %d\n",pid);
		}

	banker_exit:
		pthread_mutex_unlock(&critical_mutex);
		printf("proc %d is releaseing the lock\n", pid);
		pthread_cond_broadcast(&condition_var);

		release(p_id);
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
		for (i = 0; i < no_res; ++i){
			work[i] += hold[pid][i];
		}

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
	int i,j;
	int *request;
	request = requestGenerator(pid);

	 work = (int *)malloc(sizeof(int) * no_res);
	 finish = (int *)malloc(sizeof(int) * no_proc);

	for (i = 0; i < no_res; ++i)
	{
		request_matrix[pid][i]=request[i];
	}

	 //check whether there are enough resources available
	detection_step_1:
		for(i=0;i<no_res;i++){
			if (request[i] > need[pid][i]){
				printf("impossible assign\n");
				sleep(1);
				goto detection_exit;
			}
		}

	detection_step_2:
		for(i=0;i<no_res;i++){
			if (request[i] > avail[i]){
				//time recording before sleep
				thread_run_time[pid] += getTime() - thread_last_starting_time[pid]; 
				is_sleep[pid] = 1;
				//wait
				printf("proc %d is going to wait\n", pid);
				
				pthread_cond_wait( &condition_var, &critical_mutex );
				//update timeing after waking up
				thread_last_starting_time[pid] = getTime();
				is_sleep[pid] = 0;

				goto detection_step_1;
			}
		}

	detection_step_3:
		for(i = 0; i < no_res; i++){
			avail[i] = avail[i] - request[i];
			hold[pid][i] = hold[pid][i] + request[i];
			need[pid][i] = need[pid][i] - request[i]; 
		}

		int *finish_vector;

		if(isDeadlock(pid, finish_vector)){
			recoverDeadlock(pid,finish_vector);
		}

	detection_exit:
		pthread_mutex_unlock(&critical_mutex);
		pthread_cond_broadcast(&condition_var);
}

int isDeadlock(int pid, int *finish_vector){
	deadlock_step_1:
		work = (int *)malloc(sizeof(int) * no_res);
		finish = (int *)malloc(sizeof(int) * no_proc);

		int i,j;
		for(i=0;i<no_res;i++){
			work[i] = avail[i];
		}

		for(i=0;i<no_proc;++i){
			finish[i]=1;
			for (j = 0; j < no_res ; ++j)
			{
				if(hold[i][j]!=0){
					finish[i] = 0;
					break;
				}
			}	
		}

		finish_vector = finish;

	deadlock_step_2:
		for(i=0;i<no_proc;++j){
			
			if(finish[i] == 0){
				for (j = 0; j < no_res; ++j){
					if(request_matrix[i][j]>work[j]){
						goto deadlock_step_4;
					}
				}
			}else{
				goto deadlock_step_4;
			}
		}

	deadlock_step_3:
		for (i = 0; i < no_res; ++i){
			work[i] += hold[pid][i];
		}
		finish[i] = 1;
		goto deadlock_step_2;

	deadlock_step_4:
		for(i = 0;i<no_proc;i++){
			if(finish[i] == 0){
				return 0;
			}
		}

}

void recoverDeadlock (int pid, int *finish_vector){
	int i,*vector;
	for (i = 0; i < no_proc; ++i){
		if(finish[i]==0){
			release(i);
			finish[i]=1;
			break;
		}
	}

	if(isDeadlock(pid, vector)){
		recoverDeadlock(pid,vector);
	}

}


