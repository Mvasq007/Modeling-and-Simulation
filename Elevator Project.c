// Simulation of the Hertz airport shuttle bus, which picks up passengers
// from the airport terminal building going to the Hertz rental car lot.

#include "cpp.h"
#include <string.h>
#include <cmath>

#define NUM_SEATS 6		// number of seats available on shuttle
#define NUM_FLOORS 20

#define TINY 1.e-20		// a very small time period

#define TERMNL 0		// named constants for labelling event set
#define CARLOT 1

facility termnl ("termnl");	// customer queue at airport terminal
facility carlot ("carlot");	// customer queue at rental car lot
facility rest ("rest");		// dummy facility indicating an idle shuttle

event get_off_now ("get_off_now"); // all customers can get off shuttle

event on_termnl ("on_termnl");	// one customer can get on shuttle at terminal
event on_carlot ("on_carlot");	// one customer can get on shuttle at car lot
event boarded ("boarded");	// one customer has gotten on and taken a seat

event_set shuttle_called ("call button", NUM_FLOORS);
				// call buttons at terminal and car lot

void arrivals();	// function declarations
void arr_cust();
void departures();
void interfloor();
void dep_cust(long);
void inter_cust(long, long);
void whats_happening();
void elevator();
int source[NUM_FLOORS] ={0};
int going_to[NUM_FLOORS] ={0};

int direction = 1;
int current_location = 0;
long group_size();


extern "C" void sim()		// main process
{
	create("sim");
	arrivals();		// start a stream of arriving customers
	departures();
	interfloor();		// start a stream of departing customers
	elevator();		// create a single shuttle
	whats_happening();	
	hold (1440*60);		// wait for a whole day (in minutes) to pass
	report();
}

// Model segment 1a: generate groups of customers arriving at the airport

void arrivals()
{
	create("arrivals");

	while(clock < 1440*60.)	//
	{
		hold(expntl(10)); // exponential interarrivals, mean 10 minutes
		long group = group_size();
		source[0] += 1;
		for (long i=0;i<group;i++){
			arr_cust();	// new customer appears at the airport
		}
	}
}


// Model segment 2a: generate groups of customers at car lot, heading to airport

void departures()		// this model segment spawns departing customers
{
	create("departures");

	while(clock < 1440*60.)	//
	{
		hold(expntl(10)); // exponential interarrivals, mean 10 minutes
		long leaving = uniform(1,NUM_FLOORS);
		long group = group_size();
		source[leaving] += 1;
		for (long i=0;i<group;i++){			
			dep_cust(leaving);	// new customer appears at the car lot
		}
	}
}

void interfloor()		// this model segment spawns departing customers
{
	create("interfloor");

	while(clock < 1440*60.)	//
	{
		hold(expntl(10)); // exponential interarrivals, mean 10 minutes
		long leaving = uniform(1,NUM_FLOORS);
		long ping_pong = uniform(1,NUM_FLOORS);
		while(ping_pong == leaving) ping_pong = uniform(1,NUM_FLOORS);
		
		long group = group_size();
		source[leaving] += 1;
		for (long i=0;i<group;i++){
			inter_cust(leaving, ping_pong);	// new customer appears at the car lot
		}
	}
}


// Model segment 1b: activities followed by individual  airport customers

void arr_cust()
{
	create("arr_cust");
	carlot.reserve();	// join the queue at the airport terminal
	shuttle_called[0].set();	// head of queue, so call shuttle
	on_carlot.queue();	// wait for shuttle and invitation to board
	shuttle_called[0].clear();	// clear call; next in line will push 
	boarded.set();		// tell driver you are in your seat
	long going = uniform(1,NUM_FLOORS);
	going_to[going] += 1;
	carlot.release();	// now the next person (if any) is head of queue
	get_off_now.wait();	// everybody off when shuttle reaches car lot
}


// Model segment 2b: activities followed by individual car lot customers

void dep_cust(long leaving)
{
	create("dep_cust");

	carlot.reserve();	// join the queue at the car lot
	shuttle_called[leaving].set();	// head of queue, so call shuttle
	on_carlot.queue();	// wait for shuttle and invitation to board
	shuttle_called[leaving].clear();	// clear call; next in line will push 
	boarded.set();		// tell driver you are in your seat
	going_to[0] += 1;
	carlot.release();	// now the next person (if any) is head of queue
	get_off_now.wait();	// everybody off when shuttle reaches car lot
}


void inter_cust(long leaving, long ping_pong)
{
	create("dep_cust");

	carlot.reserve();	// join the queue at the car lot
	shuttle_called[leaving].set();	// head of queue, so call shuttle
	on_carlot.queue();	// wait for shuttle and invitation to board
	shuttle_called[leaving].clear();	// clear call; next in line will push 
	boarded.set();		// tell driver you are in your seat
	going_to[ping_pong] += 1;
	carlot.release();	// now the next person (if any) is head of queue
	get_off_now.wait();	// everybody off when shuttle reaches car lot
}
// Model segment 3: the shuttle bus


int requests()
{
	for(unsigned i = 0; i<NUM_FLOORS; i++)
	{
		if(source[i] != 0) return 1;
	}
	return 0;
}

int destination()
{
	for(unsigned i = 0; i<NUM_FLOORS; i++)
	{
		if(going_to[i] != 0) return 1;
	}
	return 0;
}


int next_floor()
{ 
	if(current_location >= NUM_FLOORS-1) direction = -1;
	else if(current_location <= 0) direction = 1;
	
	for(unsigned i = current_location+direction; i<NUM_FLOORS && i>=0; i += direction) //Traverse array based on current direction
	{
		if(source[i] != 0 || going_to[i] != 0){ return i;} //SOMEONE MADE A REQUEST OR SOMEONE HAS DESTINATION IN MY CURRENT DIRECTION, RETURN ITS THE FLOOR #;
	}
	
	if(current_location >= NUM_FLOORS-1) direction = -1;
	else if(current_location <= 0) direction = 1;	
	direction *= -1; //REVERSE CURRENT DIRECTION SINCE NO ONE NEEDS SERVICE BEYOND OUR CURRENT LOCATION
	
	for(unsigned i = current_location+direction; i<NUM_FLOORS && i>=0; i += direction) //Traverse array based on current direction
	{
		if(source[i] != 0 || going_to[i] != 0){ return i;} //SOMEONE MADE A REQUEST OR SOMEONE HAS DESTINATION IN MY CURRENT DIRECTION, RETURN ITS THE FLOOR #;
	}	
	//NO REQUESTS/DESTINATION? WTF AM I DOING HERE GONNA RETURN NEGATIVE 1 AS A MESSAGE TO SHUT THE DOORS AND NOT MOVE;
	return 0;

}

void whats_happening()
{
	while(clock < 1440*60.)	//
	{
		hold(20);
		int num_in = 0;
		for(unsigned i = 0; i<NUM_FLOORS;i++)
		{
			num_in += going_to[i];
		}
		
		int n_floor = next_floor();
		printf("On floor %d going ", current_location);
		if(direction == 1) printf("UP");
		else printf("DN");
		printf(" to %d with %d ppl to be dropped off. ",  n_floor, going_to[n_floor]);

		printf("Current capacity = %d\n", num_in);
			for(unsigned i = 0; i<NUM_FLOORS;i++)
			{
				printf("%d has %d people going to it. ", i, going_to[i]);
				if(source[i] > 0 ) printf("That floor also has a pending request of size %d", source[i]);
				printf("\n");
			}		
	

		printf("==================================\n");
	}
}


void elevator()
{
	create ("elevator");
  
	while(1)		// loop forever
	{
		// start off in idle state, waiting for the first call...
		// should check here that bus is indeed empty!
		//rest.reserve();
		//long who_pushed = shuttle_called.wait_any();
			// relax at garage till called from either location
		//shuttle_called[who_pushed].set();
			// hack to satisfy loop exit, below
		//rest.release();

		
		//while(requests() == 1 || destination() == 1)
		//{
			if(source[current_location] > 0 || going_to[current_location] > 0) //HAS SOMEONE MADE A REQUEST ON THIS FLOOR?
			{
				hold(2); //OPEN THE DOOR

				
				while(carlot.num_busy() + carlot.qlength() > 0)
				{
					on_carlot.set();// invite one person to board
					boarded.wait();	// that person is now on board
					hold(TINY);
				}
				
				if(going_to[current_location]>0)
				{ 
					get_off_now.set(); //PEOPLE LEAVING?
					going_to[current_location] = 0; //Remove folk from elevator going to current location;
				}				
				// going_to[current_location] = 0; //Remove folk from elevator going to current location;
			
				hold (uniform(5,15));	//Load Passengers
				hold (3); //CLOSE THE DOOR
				source[current_location] = 0; //REQUEST HAS BEEN SERVICED
				int nxt_flr = next_floor(); //AM I SUPPOSED TO MOVE?		
				if(nxt_flr>=0)  //AM I MOVING?
				{
					hold(5*sqrt(abs(nxt_flr-current_location))); //Move to floor
					current_location = nxt_flr; //Change floor location
				}
	
			}
			else
			{
				int nxt_flr = next_floor(); //AM I SUPPOSED TO MOVE?
				if(nxt_flr>=0)  //AM I MOVING?
				{
					hold(5); //Move to floor
					current_location = nxt_flr; //Change floor location
	
				}	
			}
		//}
		
		
	}
}

long group_size()	// function gives the number of customers in a group
{
	double x = prob();
	if (x < 0.3) return 1;
	else
	{
		if (x < 0.7) return 2;
		else
			return 4;
	}
}// start a stream of departing customers

