/*	
*	Author 	Martin Krajnak, xkrajn02@stud.fit.vutbr.cz
*	Date 	18.4.2015
*	Desc	Project 2 for IOS course at fit.vutbr.cz
*			Building H2O via semaphores
*			more information about problem provided in 
*			Allen B. Downing book The Little Book of Semaphores
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#define SYSERR 2
#define ERR 1
#define MAXTIME 5000
#define H 'H'
#define O 'O'
#define STARTED "started"
#define WAITING "waiting"
#define READY "ready"
#define BEGINBONDING "begin bonding"
#define BONDED "bonded"
#define FINISHED "finished"
#define HELP "--help"

typedef struct {
	sem_t mutex;		//only one proc in time is made
	sem_t hQ;			//waiting for the second H or O
	sem_t oQ;			//waiting for both H
	sem_t barrier;		//all procs wait there till end
	sem_t writeOut;		//only one can write in certain time
	sem_t bonder;		//
	sem_t bonding;		//
	sem_t allBonded;	//check whether all procs succesfully finished bonding
	sem_t waiting;		//providing succesfull output of waiting period  
	signed int hydrogen;		//hydrogen atoms counter
	signed int oxygen;			//oxygen atoms counter
	signed int counter;			//completed atoms counter, 
	signed int sharedCounter;	//h2o.out line counter
	signed int bonded;			//indicate if all procs are finished bonding operations
	signed int bond;			//counting while bonding
	signed int oxygenTime;		//amount of time needed to create one oxygen
	signed int hydrogenTime;	//amount of time needed to create one hydrogen
	signed int bondingTime;		//amount of time to bind a molecule 
	signed int n;				//number of molecules
}Share;

int shareId=0;
Share *share;					//data in shared memmory 

/*
*	Error handling
*/
void error(char *err, int code){
	fprintf(stderr, "Error: %s,\nType --help for more\n",err );
	exit(code);
}

void help(void){ // not implemted in main
	printf("Building H2O via semaphores.\n" 
			"Examples of usage:\n"
				"\t./h2o N GH GO B\n"
				"Where N represents number(integer) of hydrogen process,\n"
				"Where GH represents amount(integer) of time(ms) needed for creating hydrogen process,\n"
				"Where GO represents amount(integer) of time(ms) needed for creating oxygen process,\n"
				"Where B represents amount(integer) of time(ms) needed for processes to bond in one molecule,\n"
				"Also: N>0; GH,GN,B >= 0; GH,GN,B < 5001;\n"
			"\tcreated by: Martin Krajnak\n\txkrajn02@stud.fit.vutbr.cz\n");
	exit(0);
}
/*
*	Cleaning alocated resources
*/
void clean(void){	
	if (sem_destroy(&(share->mutex)) == -1)			error("sem_destroy failed",SYSERR);
	if (sem_destroy(&(share->hQ)) == -1)			error("sem_destroy failed",SYSERR);
	if (sem_destroy(&(share->oQ)) == -1)			error("sem_destroy failed",SYSERR);
	if (sem_destroy(&(share->barrier)) == -1)		error("sem_destroy failed",SYSERR);
	if (sem_destroy(&(share->writeOut)) == -1)		error("sem_destroy failed",SYSERR);
	if (sem_destroy(&(share->bonder)) == -1)		error("sem_destroy failed",SYSERR);
	if (sem_destroy(&(share->bonding)) == -1)		error("sem_destroy failed",SYSERR);
	if (sem_destroy(&(share->allBonded)) == -1)		error("sem_destroy failed",SYSERR);
	if (sem_destroy(&(share->waiting)) == -1)		error("sem_destroy failed",SYSERR);
	
	if (shmdt(share) == -1)					error("shmdt failed ",SYSERR);

	if (shmctl(shareId, IPC_RMID, NULL))	error("semctl failet",SYSERR);
	return;
}
/*
*	Allocation of resources
*/
void init(void){
	if ((shareId = shmget(IPC_PRIVATE, sizeof(Share), IPC_CREAT | 0666)) == -1) {
		error("shmget failed",SYSERR);
	}
	if ((share=(Share *)shmat(shareId,NULL,0)) ==(Share *)(-1)) 
	{
		error("shmget failed",SYSERR);
		clean();
		return;
	}
	share->hydrogen=0;		
	share->oxygen=0;
	share->sharedCounter=1;
	share->bonded=0;
	share->bond=0;
	share->counter=0;
	if (sem_init(&share->mutex,1,1) == -1)			error("sem_init failed",SYSERR);
	if (sem_init(&share->hQ,1,0) == -1)				error("sem_init failed",SYSERR);
	if (sem_init(&share->oQ,1,0) == -1)				error("sem_init failed",SYSERR);
	if (sem_init(&share->barrier,1,0) == -1)		error("sem_init failed",SYSERR);
	if (sem_init(&share->writeOut,1,1) == -1)		error("sem_init failed",SYSERR);
	if (sem_init(&share->bonder,1,0) == -1)			error("sem_init failed",SYSERR);
	if (sem_init(&share->bonding,1,1) == -1)		error("sem_init failed",SYSERR);
	if (sem_init(&share->allBonded,1,1) == -1)		error("sem_init failed",SYSERR);
	if (sem_init(&share->waiting,1,1) == -1)		error("sem_init failed",SYSERR);
	return;
}
/*
*	writing to file, critical section is procethed by semaphore
*/
void writer(FILE *f, int i, char kind, char *op)
{
	sem_wait(&share->writeOut);
	fprintf(f,"%d\t: %c %d\t:%s\n",share->sharedCounter++,kind,i,op);
	fflush(f);
	sem_post(&share->writeOut);	
	return;
}
/*
*	molecules proceeding to bond a molecule H2O
*/
void ready(void){
	sem_post(&share->hQ);
	sem_post(&share->hQ);
	share->hydrogen -= 2;
	sem_post(&share->oQ);
	share->oxygen -= 1;
	return;
}
/*
*	//bonding + critical sections
*/
void bond(void)	
{
	usleep(share->bondingTime);
	sem_wait(&share->bonding);
	share->bond++;
	if ( share->bond == 1)
	{	
		sem_wait(&share->waiting);
	}
	if ( share->bond == 3)
	{
		share->bond=0;
		sem_post(&share->bonder);
		sem_post(&share->bonder);
		sem_post(&share->bonding);
	}
	else
	{
		sem_post(&share->bonding);
		sem_wait(&share->bonder);
	}
	return;		
}
/*
*	Handling interrupt signals cleaning memmory 
*	and killing processes
*/
void sighandler(int sig)	
{
   	clean();
   	kill(getpid(),sig);
	error("Caught iterrupt, killing all",ERR); 	
	return;  
}
/*
*	Waiting for last molecule, than finishing 
*	all procs 
*/
void proceedBarrier(void)
{
	sem_wait(&share->allBonded);
	share->bonded++;
	share->counter++;
	if ( share->bonded == 3)
	{	
		share->bonded=0;
		sem_post(&share->waiting);
		sem_post(&share->mutex);
		sem_post(&share->allBonded);
	}
	else
	{
		sem_post(&share->allBonded);
	}
	if(share->counter == 3*share->n)
		sem_post(&share->barrier);
	else
		sem_wait(&share->barrier);
	return;
}
/*
*	parsing numbers from stdin provided by user
*	+ input error detection
*/
int checkNumArgs(char **argv,int argPos)
{
   	char *white;  
   	int num =(int)strtod(argv[argPos],&white);  
   	if(strlen(white) != 0)
   	{ 
     	error("Enexpected input",ERR);
     	return 0;
   	}
   	else 
   	{ 
      return num;   //no char detected, numeric value is returned 
 	}
}
/*
*	parsing args checking input data intervals
*	providing help
*/
void checkArgs(int argc, char **argv)
{	
	int args[argc];
	if(argc == 2 && (strcmp(argv[1],HELP)==0)){
		help();
	}
	else if (argc != 5){
		error("Wrong arguments inserted",ERR);
	}
	for (int i = 1; i < argc; ++i)
	{
		args[i]=checkNumArgs(argv,i);
		if (i==1 && args[i]<=0 )
		{
			error("Unexpected N value",ERR);
		}
		if ((args[i]<0 || args[i]>MAXTIME) && (i != 1))
		{
			error("Unexpected time value",ERR);
		}
	}
	long int const MAXHYDROGENTIME = args[2]+1;
	long int const MAXOXYGENTIME = args[3]+1;
	long int const MAXBONDINGTIME = args[4]+1;
	share->hydrogenTime=(random() % MAXHYDROGENTIME) * 1000;
	share->oxygenTime=(random () % MAXOXYGENTIME)* 1000;
	share->bondingTime=(random () % MAXBONDINGTIME)* 1000;
	share->n=args[1];
	return;
}
/*
*	OXyGEN CODE
*/
void doOxygen(FILE *f)
{
	pid_t o[share->n];
	for (int i = 1; i <= share->n; ++i)
	{
		o[i]=fork();
		if(o[i] == 0)
		{
			writer(f,i,O,STARTED);
			sem_wait(&share->mutex);	//watching crirical sections
			sem_wait(&share->waiting);
			share->oxygen++;
			if ( share->hydrogen >= 2 && share->oxygen >= 1)
			{	
				writer(f,i,O,READY);
				ready();
			}
			else
			{
				writer(f,i,O,WAITING);
				sem_post(&share->mutex);
			}
			sem_post(&share->waiting);
			sem_wait(&share->oQ);
			writer(f,i,O,BEGINBONDING);
			bond();
			writer(f,i,O,BONDED);	
			proceedBarrier();
			sem_post(&share->barrier);
			writer(f,i,O,FINISHED);
			exit(0);
		}
		else if(o[i] > 0)
		{
			usleep(share->oxygenTime);	
		}
		
	}
	for (int i = 1; i <= share->n; ++i)
	{
		waitpid(o[i],NULL,0);	//waiting for all children to finish
	}	
	exit(0);
	return;
}
/*
*	HYDROGEN CODE
*/
void doHydrogen(FILE *f)
{
	pid_t h[2*share->n]; 
	for (int i = 1; i <= 2*share->n; ++i)
	{	
		h[i]=fork();
		if(h[i] == 0)
		{	
			writer(f,i,H,STARTED);	
			sem_wait(&share->mutex);
			sem_wait(&share->waiting);
			share->hydrogen++;
			if ( share->hydrogen >= 2 && share->oxygen >= 1)
			{	
				writer(f,i,H,READY);
				ready();
			}
			else
			{
				writer(f,i,H,WAITING);
				sem_post(&share->mutex);
			}
			sem_post(&share->waiting);
			sem_wait(&share->hQ);
			writer(f,i,H,BEGINBONDING);
			bond();
			writer(f,i,H,BONDED);
			proceedBarrier();
			writer(f,i,H,FINISHED);
			sem_post(&share->barrier);
			exit(0);	
		}
		else if(h[i] > 0){
			usleep(share->hydrogenTime);
		}
	}
	for (int i = 1; i <= 2*share->n; ++i)	//wait for children to finish
	{
		waitpid(h[i],NULL,0);
	}	
	exit(0);
	return;
}
int main(int argc, char **argv)
{	
	signal(SIGINT, sighandler);
	init();
	checkArgs(argc,argv);
	FILE *f;
	if ( (f = fopen("h2o.out","w")) == NULL) {
		error("Could not open the file",SYSERR);
	}
	setbuf(f, NULL);
	pid_t hyd_pid = fork();	//making two threads from main thread
	if (hyd_pid == 0 )	 // handle hydrogen
	{	doOxygen(f);
		exit(0);
	}
	pid_t oxy_pid = fork();
	if (oxy_pid == 0 ) 	// handle oxygen
	{	
		doHydrogen(f);
		exit (0);
	}
	if (hyd_pid > 0 && oxy_pid > 0 ) 	// handle parent
	{	
		waitpid(oxy_pid,NULL,0); 		//waiting for all to finish
		waitpid(hyd_pid,NULL,0);
		fclose(f);						// closing file
		clean();						// cleaning memmory
		exit (0);	//parent ending program, all operations successfully done
	}
	exit (0);		
}
	
	