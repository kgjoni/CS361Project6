// KRISTI GJONI
// kgjoni2
// 673904708
// CS 361
// Homework 6



#include "hw6.h"
#include <stdio.h>
#include<pthread.h>
#include <time.h>
#include <stdlib.h>



// Declare global variables

int counter = 0;
int new_client = 0;
int new_range = 0; 
int n = 0;
int m = 0;
int sum = 0;	// hold count of passengers
int i = 0;
int positive_number = 0;	// random positive number

//srand(time(0)); 		// not used

// Mutual exclusion
pthread_mutex_t lock;

struct Passenger {

  // Block threads
  pthread_cond_t cond;
  // Mutual exclusion
  pthread_mutex_t lock;
        
  int to_floor;
  int from_floor;
		
  // Added variables
  int terminated;
  int assigned_e;

} P[PASSENGERS];


struct Elevator {

  enum {ELEVATOR_ARRIVED = 1, ELEVATOR_OPEN = 2, ELEVATOR_CLOSED = 3} state;

  int current_floor;
  int direction;
  int occupancy;
        
  // Added variable
  int following;

  // Mutual exclusion		
  pthread_mutex_t lock;
  // Block threads
  pthread_barrier_t barrier;

} E[ELEVATORS];


void scheduler_init() 
{

  // Initialize fields contained in Elevator struct
  do 
  {
				
    E[m].state = ELEVATOR_ARRIVED;
    E[m].current_floor = 0;
    E[m].occupancy = 0;
    E[m].direction = 1;
    E[m].following = -1;
   
    // Set up mutex             
    pthread_mutex_init(&E[m].lock,0);
    // Set up barrier
    pthread_barrier_init(&E[m].barrier,0,2);
                
    // Increment index
    m++;
				
  } while (m < ELEVATORS);

  
  // Passenger struct	
  do 
  {
    // Set up cond			
    pthread_cond_init(&P[n].cond,0);
    // Set up mutex
    pthread_mutex_init(&P[n].lock,0);
                
    // Increment index
    n++;
				
  }while (n < PASSENGERS);

}


void passenger_request(int passenger, int from_floor, int to_floor, void (*enter)(int, int), void(*exit)(int, int))
{

  int assigned_e = 0;

  // Set up mutex for passenger
  pthread_mutex_lock(&lock);
		
  if(sum <= passenger)
  {

    int increment_sum;
    increment_sum = passenger + MAX_CAPACITY;				
    sum = increment_sum;
		
  }
        
  P[passenger].terminated = 0;

  P[passenger].assigned_e = -1;

  P[passenger].to_floor = to_floor;

  P[passenger].from_floor = from_floor;

  
  // Release mutex for passenger		
  pthread_mutex_unlock(&lock);

  
  // Add mutex lock      
  pthread_mutex_lock(&P[passenger].lock);
  
  // Wait selecting the elevator
  pthread_cond_wait(&P[passenger].cond, &P[passenger].lock);
  
  // Release mutex lock
  pthread_mutex_unlock(&P[passenger].lock);

  
  int waiting = 1;

  // Assign a specific passenger to a specific elevator		
  assigned_e = P[passenger].assigned_e;
        
  while(waiting) 
  {
    
    // Barrier wait            
    pthread_barrier_wait(&E[assigned_e].barrier);
    
    // Add mutex lock for elevator
    pthread_mutex_lock(&E[assigned_e].lock);

    if((E[assigned_e].current_floor == from_floor) && (E[assigned_e].state == ELEVATOR_OPEN) && (E[assigned_e].occupancy == 0))
    {	 
                        
      enter(passenger, assigned_e);
						
      E[assigned_e].occupancy = E[assigned_e].occupancy + MAX_CAPACITY;
      E[assigned_e].following = to_floor;
						
      waiting = 0;
                
    }

    // Release mutex lock for elevator
    pthread_mutex_unlock(&E[assigned_e].lock);

    // Barrier wait
    pthread_barrier_wait(&E[assigned_e].barrier);
        
  }
		
  int riding=1;
        
  while(riding) 
  {
    
    // Barrier wait
    pthread_barrier_wait(&E[assigned_e].barrier);
    
    // Add mutex lock for elevator
    pthread_mutex_lock(&E[assigned_e].lock);

    if((E[assigned_e].current_floor == to_floor) && (E[assigned_e].state == ELEVATOR_OPEN)) 
    {
    
      exit(passenger, assigned_e);
                        
      E[assigned_e].following = -1;
      E[assigned_e].occupancy = E[assigned_e].occupancy - 1;
                        
      riding = 0;
    }

    // Release mutex lock for elevator				
    pthread_mutex_unlock(&E[assigned_e].lock);
                
    // Barrier wait
    pthread_barrier_wait(&E[assigned_e].barrier);
        
  }

}


void elevator_ready(int elevator, int at_floor, void(*move_direction)(int, int), void(*door_open)(int), void(*door_close)(int)) 
{

  new_range = DELAY;

  new_client = -1;

  // Add mutex lock for elevator
  pthread_mutex_lock(&E[elevator].lock);

  if(E[elevator].state == ELEVATOR_OPEN) 
  {

    door_close(elevator);
    
    E[elevator].state = ELEVATOR_CLOSED;
  }

  else if((E[elevator].following == at_floor) && (E[elevator].state == ELEVATOR_ARRIVED))
  {

    door_open(elevator);

    E[elevator].state = ELEVATOR_OPEN;

    // Release mutex lock for elevator
    pthread_mutex_unlock(&E[elevator].lock);

    pthread_barrier_wait(&E[elevator].barrier);

    // Client has time to get on elevator
    pthread_barrier_wait(&E[elevator].barrier);

  }

		
  else 
  {
                
    if(E[elevator].following <= -1) 
    {

      // Add mutex lock
      pthread_mutex_lock(&lock);
						
						
      for(counter = 0; counter < sum; counter++) 
      {
        
        // Generate a positive number in specified range
        positive_number = rand() % (MAX_CAPACITY + 1 - 0) + 0;	
							
        if((positive_number < new_range) && (!P[counter].terminated))
        {
	
	  new_client = counter;
          
          new_range = positive_number;                               										
        }
      } 
						
      if((new_client > -1) || (new_client < -1)) 
      {

        P[new_client].assigned_e = elevator;

        E[elevator].following = P[new_client].from_floor;        
        
        P[new_client].terminated = MAX_CAPACITY;
        
        pthread_cond_signal(&P[new_client].cond);
                        
      }

      // Release mutex lock
      pthread_mutex_unlock(&lock);
                
    }

                
    if(E[elevator].following > -1) 
    {

      if(E[elevator].following < at_floor)
      {
                                
        move_direction(elevator,-1);
                                
        E[elevator].current_floor = E[elevator].current_floor - 1;
                        
      }

      if(E[elevator].following > at_floor) 
      {
                                
        move_direction(elevator,1);
                                
        E[elevator].current_floor = E[elevator].current_floor + MAX_CAPACITY;
                        
      }
    }
                
    E[elevator].state = ELEVATOR_ARRIVED;
       
  }
 
  // Release mutex lock for elevator       
  pthread_mutex_unlock(&E[elevator].lock);

}

