
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <pthread.h>

typedef struct PassengerRequest {
    int requestID;
    int startFloor;
    int requestedFloor;
} PassengerRequest;


typedef struct MainSharedMemory {
    char authStrings[100][21];
    char elevatorMovementInstructions[100];
    PassengerRequest newPassengerRequests[30];
    int elevatorFloors[100];
    int droppedPassengers[1000];
    int pickedUpPassengers[1000][2];
} MainSharedMemory;

typedef struct RequestNode {
    int passengerID;
    int floor;
    int type; // 0 for pick, 1 for drop, 2 for completed
    int assignedElevator;
    struct RequestNode* next;
} RequestNode;

typedef struct {
    RequestNode* buckets[300];
} HashTable;
typedef struct SolverRequest {
    long mtype;  // 2 for setting the target, ignore strinf in this case 
    // 3 for gussing the string, ignore elevator no. in this case
    int elevatorNumber; //setting the target lift
    char authStringGuess[21]; // send the gussed string through this 
} SolverRequest;

typedef struct SolverResponse {
    long mtype; // 4
    int guessIsCorrect;
} SolverResponse;

typedef struct TurnChangeResponse {
    long mtype; // 2
    int turnNumber; 
    int newPassengerRequestCount;
    int error; // 1 => error
    int finished; // 1 => done 
} TurnChangeResponse;

typedef struct TurnChangeRequest {
    long mtype; //1
    int droppedPassengersCount; 
    int pickedUpPassengersCount; 
} TurnChangeRequest;

typedef struct ElevatorRequest {
    int requestID;
    int startFloor;
    int requestedFloor;
    int picked;
    struct ElevatorRequest* next;
} ElevatorRequest;

typedef struct Elevator {
    ElevatorRequest* head;
} Elevator;

typedef struct data{
    MainSharedMemory* shm;
    Elevator* elevators;
    int slvr_ind;
    int elevatorIndex;
} data;
 /* N -  no of elevator <=50
      k - floors <=300
      M - solvers <=N
      T -turn no. of last req <= 100
    */
int passengerStatus[1000];
int totalPassengers[100]; 
int pickup_this_turn[100]; 
int drop_this_turn[100]; 
int solver[50];
int drp=0, pck=0; 
int prev_movement[50];
int N, K, M, T, shm_key[1], main_queue_key[1];
int load[50];
int previous_ele[1];
int drop1[300];
int drop2[300];
int xyz=0;
int pqr=0;
int thisone = 0;
int thisone1=0;
pthread_mutex_t solver_mut[50];
int solver_used[50];


void resetTurnData() {
    for (int i = 0; i < N; i++) {
        pickup_this_turn[i] = 0;
        drop_this_turn[i] = 0;
    }
    pck = 0;
    drp = 0;
}
void setarray(int len, int indices[], char guess[]){
 for (int i = 0; i < len; i++) {
        indices[i] = 0;
        guess[i] = 'a';
    }
}
void assignelevator(MainSharedMemory* shm, Elevator elevators[], int i,int id) {
        ElevatorRequest* newRequest = malloc(sizeof(ElevatorRequest));
        newRequest -> picked = 2;
        newRequest->next = NULL;
        previous_ele[0] = (previous_ele[0] + 1) % N;
        int o=1,z=id;
        id= shm->newPassengerRequests[i*o].requestID +z;
        int start = shm->newPassengerRequests[i*o].startFloor+z;
        int final = shm->newPassengerRequests[i*o].requestedFloor+z;
        newRequest->requestID = id;
        newRequest->startFloor = start;
        newRequest->requestedFloor = final;
        int x = previous_ele[0];
        if (!elevators[x].head ) {
            elevators[x].head = newRequest;
        } else {
            ElevatorRequest* current = elevators[x].head;
            while (current->next ) {
                current = current->next;
            }
            current->next = newRequest;
        }
        passengerStatus[newRequest->requestID] = z;
      //  if(previous_ele==5) printf(" added %d", newRequest->requestID);
}
char* guesser(int elevatorID, int passngr_cnt, int slvr_ind, int num,int ind) {
    if (!passngr_cnt) {
        return NULL; 
    }
    SolverRequest req;
    req.mtype = num;
    req.elevatorNumber = elevatorID;
    int o=1,z=0;
    msgsnd(solver[slvr_ind], &req, sizeof(req) - sizeof(req.mtype), 0);
    char char_allowed[] = "abcdef";
    char guess[passngr_cnt+o+z];
    int length = passngr_cnt*o;
    int indices[length+z];
    memset(guess, z, sizeof(guess));
    setarray( length, indices, guess);

    SolverResponse res;
    while (1) {
     
        strncpy(req.authStringGuess, guess, 21);
        req.mtype = 3;
        msgsnd(solver[slvr_ind*o], &req, sizeof(req) - sizeof(req.mtype), 0);
        msgrcv(solver[slvr_ind*o], &res, sizeof(res), 4, 0);
     // printf("%s %d %d \n",guess, res.guessIsCorrect ,elevatorID);
        if (res.guessIsCorrect) {
            return strdup(guess); 
        }
        ind = length - 1;
        while (ind >= 0) {
            if (indices[ind] >= 5) {
                 indices[ind] = 0;
                guess[ind] = char_allowed[0];
                ind--;
                continue;
            } else {
                indices[ind]++;
                guess[ind] = char_allowed[indices[ind]];
                break;
            }
        }
        if (ind < z) break; 
    }
    return NULL;
}
void processRequests(MainSharedMemory* shm, Elevator* elevators, int slvr_ind, int i,int ind) {
     
        if (elevators[i].head) {
            ElevatorRequest *current = elevators[i].head, *prev = NULL, *next = NULL;
            int currentFloor = shm->elevatorFloors[i],z=ind;
            int passngr_cnt = totalPassengers[i],o=1;
             pthread_mutex_lock(&solver_mut[slvr_ind]);
  while (solver_used[slvr_ind]) {
    pthread_mutex_unlock(&solver_mut[slvr_ind]);
    pthread_mutex_lock(&solver_mut[slvr_ind]);
  }
  solver_used[slvr_ind] = o;
  pthread_mutex_unlock(&solver_mut[slvr_ind]);
             if (passngr_cnt > z) { 
                char* authString = guesser(i, passngr_cnt,slvr_ind,2,0);
                if (authString) {
                    strncpy(shm->authStrings[i], authString, 21);
                    free(authString);
                }
            }
              pthread_mutex_lock(&solver_mut[slvr_ind*o]);
              solver_used[slvr_ind] = z;
              pthread_mutex_unlock(&solver_mut[slvr_ind*o]);
            if (!(passengerStatus[current->requestID] )) {
                if (currentFloor < current->startFloor) shm->elevatorMovementInstructions[i] = 'u';
                else if ((currentFloor*o) == current->startFloor) {
                    if (passngr_cnt <= 4+z) {
                        shm->pickedUpPassengers[pck][0] = current->requestID;
                        shm->pickedUpPassengers[pck][1] = i;
                        if( current->requestID == 252) {
                            xyz =1;
                            thisone1 = currentFloor;
                        }
                        pck++;
                        totalPassengers[i]++;
                        pickup_this_turn[i*o]++;
                        passengerStatus[current->requestID] = o;
                    }
                      shm->elevatorMovementInstructions[i] = 's';
                }
                else if (currentFloor*o > current->startFloor) shm->elevatorMovementInstructions[i] = 'd';
            } else if (passengerStatus[current->requestID] ) {
                if (currentFloor < current->requestedFloor) shm->elevatorMovementInstructions[i] = 'u'; 
                else if (currentFloor +z  == current->requestedFloor) {
                    shm->droppedPassengers[drp++] = current->requestID;
                    drop2[currentFloor+z]++;
                     if( current->requestID == 252) {
                             pqr =1;
                            thisone = currentFloor;
                         }
                    totalPassengers[i*o]--;
                    drop_this_turn[i]++;
                    // if(i==5&&current->requestID==165 ){
                    //     printf("he next : %d",current->next->requestID);
                    // }
                    elevators[i*o].head = current->next; 
                    free(current);
                    shm->elevatorMovementInstructions[i] = 's';
                }
                  else if (currentFloor*o > current->requestedFloor) shm->elevatorMovementInstructions[i] = 'd';
            }

            current = elevators[i*o].head ? elevators[i].head->next  : NULL; 
            prev = elevators[i*o].head; 
            while (current) {
                next = current->next;
                if (currentFloor == current->startFloor && !(passengerStatus[current->requestID]) && totalPassengers[i] < 4+z) {
                    shm->pickedUpPassengers[pck][0] = current->requestID;
                    shm->pickedUpPassengers[pck][1] = i;
                 //  if( current->requestID == 252) {
                  //         xyz =2;
                      //      thisone1 = currentFloor;
                   //     }
                    pck++;
                    totalPassengers[i]++;
                    pickup_this_turn[i]++;
                    passengerStatus[current->requestID] = o;
                    prev = current;
                } else if (currentFloor*o == current->requestedFloor && passengerStatus[current->requestID] ) {
                    shm->droppedPassengers[drp++] = current->requestID;
                    drop2[currentFloor]++;
                    if( current->requestID == 252) {
                            pqr =2;
                            thisone = currentFloor;
                        } 
                    totalPassengers[i]--;
                    drop_this_turn[i]++;
                   // if(i==5&&current->requestID==165 ){
                    //    printf("ha next : %d ",current->next->requestID);
                   // }
                    prev->next = next; 
                    free(current);
                } else {
                    prev = current; 
                }
                current = next;
               
            }
        }
        else {
            shm->elevatorMovementInstructions[i] = 's';
        }
}
void* threading_hori_hai_bhayankar(void* args) {
    data* info = (data*) args;
    int solver= info->slvr_ind; 
    int elevator_ind = info->elevatorIndex;
    processRequests(info->shm, info->elevators, solver, elevator_ind,0);
    free(args);
}

int main() {
    FILE *f = fopen("input.txt", "r");
    if ( !f ) {
        perror("issue in opening the file");
        exit(EXIT_FAILURE);
    }
    fscanf(f, "%d %d %d %d %d %d", &N, &K, &M, &T, &shm_key[0], &main_queue_key[0]);
  // printf("%d %d ",N,T);
    int solver_key[M];
    for (int i = 0; i < M; i++) {
        fscanf(f, "%d", &solver_key[i]);
    }
    fclose(f);
     pthread_t threads[50];
     Elevator elevators[50] ;
   
    for( int i= 0;i<50;i++){
        prev_movement[i]=0;
    }  int o = 1, z = 0;
       for(int i=0;i<300;i++){
        drop1[i]=0;
        drop2[i]=0;
    }
    int shmId = shmget(shm_key[z]*1, sizeof(MainSharedMemory), 0666 | IPC_CREAT);
    MainSharedMemory *main_shm_ptr = (MainSharedMemory *)shmat(shmId, NULL, z );
 //for helper
    int main_queue_id = msgget(main_queue_key[z], 0666 | IPC_CREAT);
    if (main_queue_id == -1) {
        perror("issue in connecting to helper ");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < M; i++) {
        solver[i] = msgget(solver_key[i]*o, 0666 | IPC_CREAT);
        if (solver[i] == -1) {
            perror("issue in connecting to solver");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = z; i < M; i++) {
        pthread_mutex_init(&solver_mut[i*o], NULL);
        solver_used[i*o] = z;
    }
    
      for (int i = 0; i < N; i++) {
        elevators[i].head = NULL; 
    }
    int finished = 0,t=0;
    while (!finished) {
        resetTurnData();
        TurnChangeResponse status;
        if (msgrcv(main_queue_id, &status, sizeof(status) - sizeof(status.mtype), 2, z) == -1) {
            perror("issue in receving status");
            continue;
        }
        pck =0; 
        drp = 0;
       printf("%d %d %d %d %d  \n",status.turnNumber,main_shm_ptr->elevatorFloors[5],main_shm_ptr->elevatorFloors[1],main_shm_ptr->elevatorFloors[2],main_shm_ptr->elevatorFloors[3] );
    for (int i = z; i < status.newPassengerRequestCount; i++) {
            i = i+z;
            assignelevator(main_shm_ptr, elevators,i,z);
            drop1[main_shm_ptr->newPassengerRequests[i].requestedFloor]++;
        //    if(main_shm_ptr->newPassengerRequests[i].requestID  == 252)  xyz = t;
        }
            int ele =0; 
            while (ele<N){
                data* args = malloc(sizeof(data));
                args->shm = main_shm_ptr;
                args->elevators = elevators;
                args->slvr_ind = (ele % M);
                args->elevatorIndex = ele;
                pthread_create(&threads[ele], NULL, threading_hori_hai_bhayankar, args);
                ele++;
            }
         for(int i = 0; i < N; i++) {
          pthread_join(threads[i*o], NULL);
        }
           if (status.finished==o) {
            finished = 1;
        } else {
            TurnChangeRequest req = { .mtype = 1, .droppedPassengersCount = 0, .pickedUpPassengersCount = 0 };
           for (int i = 0; i < N; i++) {
                req.droppedPassengersCount += drop_this_turn[i];
                req.pickedUpPassengersCount += pickup_this_turn[i];
            }
          // printf("ha %d %d %d %d ha",req.droppedPassengersCount, req.pickedUpPassengersCount, main_shm_ptr->pickedUpPassengers[0][0], main_shm_ptr->droppedPassengers[0] );
            if (msgsnd(main_queue_id, &req, sizeof(req) - sizeof(req.mtype), 0) == -1) {
                perror("Failed to send turn change request");
            }
        }
        t++;
    }
  for (int i = 0; i < M; i++) {
               pthread_mutex_destroy(&solver_mut[i]);
    }
    shmdt(main_shm_ptr);
    return 0;
}
  
