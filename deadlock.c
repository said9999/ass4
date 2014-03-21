#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define RUN_INTERVAL 5

int no_proc,				//no of process
	strategy,				//strategy to use 0:banker 1:detection
	no_res,					//type of resources
	range_res,				//range of resources
	**max,					//max matrix
	**need,					//need matrix
	**hold,					//hold/allocation mactrix
	*avail,					//available vector
	*work,					//work 	vector
	*finish,				//finish vector
	*is_sleep,				//is_sleep array to check whether a thread is waiting
	**request_matrix;		//request matrix

double simulate_time,		//running time for the simulator
		start_time,			//start time for the simulator
		end_time,			//end time for the simulator
		*thread_run_time,	//run time for each thread
		*thread_last_starting_time;//last start time for each thread, will update after waking up

void (*allocation)(int);	//allocation strategy

pthread_cond_t  condition_var   = PTHREAD_COND_INITIALIZER;
pthread_mutex_t critical_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * This function get the current time by using sec
 */
double getTime(){
	struct timeval time;

	gettimeofday(&time, NULL);
	return (time.tv_sec + time.tv_usec/1000000.0);
}

/*
 * This function randomly generate request vector for each process
 */

int* requestGenerator(int pid){
	int *request = (int *)malloc(sizeof(int) * no_res);

	int i;
	for(i = 0;i<no_res;i++){
		request[i] = (rand()%max[pid][i])/2+1; 
	}

	return request;
}

/*
 * This function kill the main thread after simulate time and print out result data
 */
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

void input();											//get input argument
int isSafe(int pid);									//isSafe in banker argumentlgorithm
void *commandGenerator(void *id);						//generate cmd for process
void release(int pid);									//release resource for one process
void bankerAllocation(int pid);							//banker algorithm
void detectionAllocation(int pid);						//detection algorithm
int isDeadlock(int pid, int *finish_vector);			//isdeaklock in detection algorithm
void recoverDeadlock(int pid, int *finish_vector);		//recover deadlock



int main(int argc, char *argv[]){
	//GET INPUT
	input();

	/************
	 *INIT PART
	 ************/
	int i,
		j;
	
	pthread_t childThread[no_proc];
	srand(1024);										//set seed 
	
	if (strategy == 0){
		allocation = &bankerAllocation;
	}else{
		allocation = &detectionAllocation;
	}

	thread_run_time = (double *)malloc(sizeof(double) * no_proc);
	thread_last_starting_time = (double *)malloc(sizeof(double) * no_proc);
	is_sleep = (int *)malloc(sizeof(int) * no_proc);

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

	request_matrix = malloc(sizeof(int *) * no_proc);
	for (i = 0; i < no_proc; ++i){		
		request_matrix[i] = malloc(no_res * sizeof(int));
		for (j = 0; j < no_res; ++j){
			request_matrix[i][j] = 0;
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
	
	/***********
	 * CREATE PROC
	 *************/
	for(i = 0; i < no_proc; i++){
		pthread_t thread_c;
		int *tmp = malloc(sizeof(*tmp));
		*tmp = i;
		
		pthread_create( &thread_c, NULL, &commandGenerator, (void *) tmp);
		childThread[i] = thread_c;
	}

	/**********
	 * Start Timer
	 ***********/
	simulateTimer();
	
	/************
	 * JOIN PROCS
	 ***********/
	for(i = 0; i< no_proc; i++){
		pthread_join(childThread[i],NULL);
	}

	return 0;
}

void input(){

	printf("Please enter the number of process:");
	scanf("%d",&no_proc);
	printf("Please enter the number of resource:");
	scanf("%d",&no_res);
	printf("Please enter the number of range for each resource:");
	scanf("%d",&range_res);
	printf("Please enter the strategy to use, 0 for Banker's Algorithm, 1 for Deadlock Detection:");
	scanf("%d",&strategy);
	printf("Please enter the time to simulate:");
	scanf("%lf",&simulate_time);

}

void *commandGenerator(void *id){
	int p_id = *((int *)id);

	//time recording
	thread_run_time[p_id] = 0.0f;	//start time;
	thread_last_starting_time[p_id] = getTime();
	is_sleep[p_id] = 0;

	while(1){
		int run_time = rand()%RUN_INTERVAL;			
		
		allocation(p_id);	

		run_time = rand()%RUN_INTERVAL;

		sleep(run_time);	

		release(p_id);
	}
}

void release(int pid){
	pthread_mutex_lock(&critical_mutex);	

	int i;
	
	for(i = 0;i<no_res;i++){
		avail[i] += hold[pid][i];
		need[pid][i] += hold[pid][i];
		hold[pid][i] = 0;
	}

	pthread_mutex_unlock(&critical_mutex);
	pthread_cond_broadcast(&condition_var);
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
				thread_run_time[pid] += getTime() - thread_last_starting_time[pid]; 
				is_sleep[pid] = 1;
				
				pthread_cond_wait( &condition_var, &critical_mutex );
				
				thread_last_starting_time[pid] = getTime();
				is_sleep[pid] = 0;

				goto banker_step_1;
			}
		}

	banker_step_3:
		for(i = 0; i < no_res; i++){					//provisional allocation
			avail[i] = avail[i] - request[i];
			hold[pid][i] = hold[pid][i] + request[i];
			need[pid][i] = need[pid][i] - request[i]; 
		}

		if(!isSafe(pid)){								//if not safe, cancel allocation
			for(i = 0; i < no_res; i++){
				avail[i] = avail[i] + request[i];
				hold[pid][i] = hold[pid][i] - request[i];
				need[pid][i] = need[pid][i] + request[i]; 
			}

			thread_run_time[pid] += getTime() - thread_last_starting_time[pid]; 
			is_sleep[pid] = 1;

			pthread_cond_wait( &condition_var, &critical_mutex );

			thread_last_starting_time[pid] = getTime();
			is_sleep[pid] = 0;

			goto banker_step_1;
		}else{
			///printf("safe to allocate resource to proc %d\n",pid);
		}

	banker_exit:
		pthread_mutex_unlock(&critical_mutex);
		pthread_cond_broadcast(&condition_var);
}

int isSafe(int pid){
	safe_step_1:
		work = (int *)malloc(sizeof(int) * no_res);
		finish = (int *)malloc(sizeof(int) * no_proc);

		int i,j;
		for(i=0;i<no_res;i++){
			work[i] = avail[i];
		}

		for(i=0;i<no_proc;i++){
			finish[i] = 0;
		}

	safe_step_2:
		for(i=0;i<no_proc;i++){
			if(finish[i] == 0 ){
				int isValid = 1;
				int m;
				for(m = 0; m < no_res; m++){
					if (need[i][m] > work[m]){
						isValid = 0;
					}
				}

				if(isValid){
					goto safe_step_3;
				}
			}
		}
		goto safe_step_4;

	safe_step_3:
		for (j = 0; j < no_res; ++j){
			work[j] += hold[i][j];
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

	int i,j;
	int *request;
	request = requestGenerator(pid);

	work = (int *)malloc(sizeof(int) * no_res);
	finish = (int *)malloc(sizeof(int) * no_proc);

	for (i = 0; i < no_res; ++i){
		request_matrix[pid][i]=request[i];
	}

	detection_step_1:
		for(i=0;i<no_res;i++){
			if (request[i] > need[pid][i]){
				goto detection_exit;
			}
		}

	detection_step_2:
		for(i=0;i<no_res;i++){
			if (request[i] > avail[i]){				//if resource cannot be granted immediately
				int *finish_vector;	

				if(isDeadlock(pid, finish_vector)){ //check deadlock first
					recoverDeadlock(pid,finish_vector);
				}else{								//else wait
					thread_run_time[pid] += getTime() - thread_last_starting_time[pid]; 
					is_sleep[pid] = 1;
					
					pthread_cond_wait( &condition_var, &critical_mutex );

					thread_last_starting_time[pid] = getTime();
					is_sleep[pid] = 0;
				}

				goto detection_step_1;
			}
		}

	detection_step_3:
		for(i = 0; i < no_res; i++){
			avail[i] = avail[i] - request[i];
			hold[pid][i] = hold[pid][i] + request[i];
			need[pid][i] = need[pid][i] - request[i]; 
			request_matrix[pid][i] = 0;
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

	deadlock_step_2:
		for(i=0;i<no_proc;++i){
			if(finish[i] == 0){
				int isValid = 1;

				for (j = 0; j < no_res; ++j){
					if(request_matrix[i][j]>work[j]){
						isValid = 0;
						break;
					}
				}
				if(isValid){
					goto deadlock_step_3;
				}
			}
		}
		goto deadlock_step_4;

	deadlock_step_3:
		for (j = 0; j < no_res; ++j){
			work[j] += hold[i][j];
		}
		finish[i] = 1;
		goto deadlock_step_2;

	deadlock_step_4:
		finish_vector = finish;
		for(i = 0;i<no_proc;i++){
			if(finish[i] == 0){
				return 1;
			}
		}

		return 0;
}

void recoverDeadlock (int pid, int *finish_vector){
	int i,*vector;

	int offset = rand()%100;
	int index = offset%no_proc;

	//randomly release a deadlock process
	for (i = 0; i < no_proc; ++i){

		if(finish[index]==0){
			thread_run_time[index] = 0;
			thread_last_starting_time[index] = getTime();
			int j;
	
			for(j = 0;j<no_res;j++){
				avail[j] += hold[index][j];
				need[i][j] += hold[index][j];
				hold[index][j] = 0;
			}		

			finish[index]=1;
			break;
		}

		index++;
		index = index%no_proc;
	}


	if(isDeadlock(pid, vector)){		//if still deadlock, then resolve deadlock again
		recoverDeadlock(pid,vector);
	}

}


