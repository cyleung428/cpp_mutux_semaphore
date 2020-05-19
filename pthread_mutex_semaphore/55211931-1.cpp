#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <unistd.h>
#include <iomanip>
#include <semaphore.h>
using namespace std;

#define NUM_THREADS     3

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
sem_t empty;
int droppedToken = 0;
int sequenceNum = 0;
int fetchedToken = 0;
int generateToken = 0;
int tokenPflow = 0;

//the data struct of Queue
struct Queue {
    int front=0;
    int rear=0;
    int num[50];
    int size() {
        if (rear>=front)
            return rear-front;
        else
            return 51-front+rear;
    }
    void push(int n) {num[(rear++)%50] = n;}
    void pop() {(front++)%50;}
    bool empty() {return front==rear;}
    
};
//the struct for the input of threads
struct arg_struct {
    int arg1;
    int arg2;
};

Queue numQueue;
void* flow(void* arg){
    //get the input from main thread
    struct arg_struct *arguments = (struct arg_struct *)arg;
    int max = arguments->arg1;
    int interval = arguments->arg2;
    while(fetchedToken+droppedToken < max)
    {
        usleep(interval*1000000); //sleep
        if(fetchedToken+droppedToken < max)
        {
        int num = rand()%10+1;
        int array[10];
        pthread_mutex_lock(&mutex1); //criteral section
        pthread_mutex_lock(&mutex2);
        for (int i=0;i<num;i++)
        {
            array[i] = sequenceNum++;
        }
        if (numQueue.size()+num>=50) // When the queue size is over 50 after pushhing the tokens
        {
            droppedToken += numQueue.size()+num-50;
            int new_num = 50-numQueue.size();
            for(int i=0;i<new_num;i++)
            {
                numQueue.push(array[i]);
            }
        }
        else
        {
            for(int i=0;i<num;i++)
            {
                numQueue.push(array[i]);
            }
        }
        
        
        int extraToken=0, extraLast=0, extraQueue=0;
        generateToken += num;
        if (num>9)
            extraToken = 1;
        if (sequenceNum >999)
            extraLast = 3;
        else if (sequenceNum >99)
            extraLast = 2;
        else if (sequenceNum >9)
            extraLast = 1;
        if (numQueue.size()>9)
            extraQueue = 1;
        cout << num<<"(Flow)"<<setw(15-extraToken+extraLast)<<sequenceNum-1<<setw(24-extraLast+extraQueue)<<numQueue.size()<<endl;
        pthread_mutex_unlock(&mutex2);
        pthread_mutex_unlock(&mutex1);
        }
        
    }
    
    pthread_exit((void*) 0);
}

void* pflow(void* arg){
    //get the input from main thread
    struct arg_struct *arguments = (struct arg_struct *)arg;
    int max = arguments->arg1;
    while(fetchedToken+droppedToken < max)
    {
        sem_wait(&empty);
        if(fetchedToken+droppedToken < max)
        {
        int num = rand()%5+1;
        int array[5];
        pthread_mutex_lock(&mutex1); //criteral section
        pthread_mutex_lock(&mutex2);
        for (int i=0;i<num;i++)
        {
            array[i] = sequenceNum++;
        }

        for(int i=0;i<num;i++)
        {
            numQueue.push(array[i]);
        }
        
        
        int extraToken=0, extraLast=0, extraQueue=0;
        tokenPflow += num;
        if (num>9)
            extraToken = 1;
        if (sequenceNum >999)
            extraLast = 3;
        else if (sequenceNum >99)
            extraLast = 2;
        else if (sequenceNum >9)
            extraLast = 1;
        if (numQueue.size()>9)
            extraQueue = 1;
        cout << num<<"(Pflow)"<<setw(14-extraToken+extraLast)<<sequenceNum-1<<setw(24-extraLast+extraQueue)<<numQueue.size()<<endl;
        pthread_mutex_unlock(&mutex2);
        pthread_mutex_unlock(&mutex1);
        }
        
    }
    
    pthread_exit((void*) 0);
}

void* server(void* arg){
    int max = *(int *)arg;
    while(fetchedToken+droppedToken < max) //When the served token is over the max tokens
    {
        usleep(2000000);
        int num = rand()%20+1;
        int fetched;
        pthread_mutex_lock(&mutex1);
        if (numQueue.size()<num)
        {
            if (fetchedToken+droppedToken+numQueue.size()>max)
            {
                fetched = max-fetchedToken-droppedToken;
                for (int i=0;i<fetched;i++)
                {
                    numQueue.pop();
                }
            }
            else{
                fetched = numQueue.size();
                while(!numQueue.empty())
                {
                    numQueue.pop();
                }
            }
        }
        else
        {
            if (fetchedToken+droppedToken+num>max)
            {
                fetched = max-fetchedToken-droppedToken;
                for (int i=0;i<fetched;i++)
                {
                    numQueue.pop();
                }
            }
            else
            {
                fetched = num;
                for (int i=0;i<num;i++)
                {
                    numQueue.pop();
                }
            }
        }
        if(numQueue.size()==0)
            sem_post(&empty);
        fetchedToken+= fetched;
        int extraQueue=0,extraServer=0,extraTotal=0;
        if(numQueue.size()>9)
            extraQueue=1;
        if(fetched>9)
            extraServer = 1;
        if(fetchedToken>99)
            extraTotal=2;
        else if (fetchedToken>9)
            extraTotal=1;
        
        cout<<setw(46+extraQueue)<<numQueue.size()<<setw(23-extraQueue+extraServer)<<fetched<<setw(18-extraServer+extraTotal)<<fetchedToken<<endl;;
        pthread_mutex_unlock(&mutex1);
        
    }
    
    sem_post(&empty);
    pthread_exit((void*) 0);
}


int main(int argc, const char * argv[]) {
    sem_init(&empty,0,1);
    pthread_t threads[NUM_THREADS];
    int MaxToken = atoi(argv[1]);
    int flowInterval = atoi(argv[2]);
    printf("The MaxToken is %d and the flowInterval is %d\n",MaxToken,flowInterval);
    cout << "Flow"<<setw(46)<<"Queue"<<setw(25)<< "Server"<<endl;
    cout << "Token added"<< setw(30)<<"Last sequence number" <<setw(18)<< "Current Length"<<setw(23)<<"Token Fetched"<<setw(25)<<"Total Token Fetched"<<endl;
    int rc;
    void *retval;
    arg_struct arguments;
    arguments.arg1 = MaxToken;
    arguments.arg2 = flowInterval;
    rc = pthread_create(&threads[0], NULL, flow, (void*) &arguments);//create threads
    if (rc) {
        cout << "Error when creating flow thread!"<<endl;
        exit(-1);
    }
    rc = pthread_create(&threads[1], NULL, server, (void*) &MaxToken);
    if (rc) {
        printf("Error when creating server thread!\n");
        exit(-1);
    }
    rc = pthread_create(&threads[2], NULL, pflow, (void*) &arguments);
    
    for(int i=0;i<NUM_THREADS;i++)//join the threads
    {
        rc = pthread_join(threads[i],&retval);
        if (rc){
            cout <<"Error when joining thread!\n";
            exit(-1);
        }
    }
    sem_destroy(&empty);
    printf("The total number of tokens that have been generated by the flow is %d.\n", generateToken);
    printf("The total number of tokens that have been generated by the pflow is %d.\n", tokenPflow);
    printf("The total number of tokens that have been fetched by the server is %d.\n", fetchedToken);
    printf("The total number of tokens that have been dropped by the queue is %d.\n", droppedToken);
    
    
    pthread_exit(NULL);
}

