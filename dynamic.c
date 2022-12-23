#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include <sys/time.h>

#define ROOT 0
#define HEAVY  1000
#define SIZE   40
#define RADIUS 10
#define WORK_TAG 1
#define STOP_TAG 0


double master(int, int);
void slave(void);
double calculate_dynamic(int, int, int);

// This function performs heavy computations, 
// its run time depends on x and y values
// DO NOT change the function
double heavy(int x, int y) {
	int i, loop;
	double sum = 0;

	if (sqrt((x - 0.75*SIZE)*(x - 0.75*SIZE) + (y - 0.25*SIZE)*(y - 0.25*SIZE)) < RADIUS)
		loop = 5 * x*y;
	else
		loop = y + x;

	for (i = 0; i < loop*HEAVY; i++)
		sum += sin(exp(cos((double)i / HEAVY)));

	return  sum;
}

// Sequencial code to be parallelized
int main(int argc, char *argv[]) {
	int numProcs, rank, iterations, i;
	int size = SIZE;
	double finalResult = 0, startTime, endTime;

	if (argc >= 2)
	{
		iterations = atoi(argv[1]);
			if (iterations < 1)
			{
				printf("Expected a positive number for iterations, recieved %s", argv[1]);
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			}
	}
	else {
		printf("Please enter a number.\n");
		exit(1);
	}

	MPI_Init(&argc, &argv); //initialize the processes
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (numProcs < 2)
	{
		printf("Number of processes must be at least 2.\n");
		exit(1);
	}

	startTime = MPI_Wtime();
	for (i = 0; i < iterations; i++)
		finalResult = calculate_dynamic(numProcs, rank, size);

	if (rank == ROOT)
	{
		endTime = MPI_Wtime();
		printf("answer = %e\n", finalResult);
		printf("Average Time measured: %.3f seconds.\n", ((endTime - startTime) / iterations));
	}

	MPI_Finalize(); //end process
	return EXIT_SUCCESS;
}

double calculate_dynamic(int numProcs, int rank, int size)
{
	double answer = 0;
	if (rank == ROOT)
		answer = master(numProcs, size);
	else
		slave();

	return answer;
}


void slave()
{
	MPI_Status status;
	double result;
	int values[2];
	while (1)
	{
		MPI_Recv(values, 2, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		if (status.MPI_TAG == STOP_TAG)
			break;
		result = heavy(values[0], values[1]);

		MPI_Send(&result, 1, MPI_DOUBLE, ROOT, WORK_TAG, MPI_COMM_WORLD);
	}
}

double master(int numProcs, int size)
{
	MPI_Status status;
	double result;
	double answer = 0;
	int values[2] = {0,0}, i, activeSlaves = numProcs - 1;//in values[2] = {x,y}

	for (i = 1; i < numProcs; i++)
	{
		MPI_Send(values, 2, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD);
		values[0]++;
	}

	while (activeSlaves)// as long as activeSlaves != the while loop will procced
	{
		MPI_Recv(&result, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		answer += result;
		if (values[0] >= size)
		{
			values[0] = 0;
			values[1]++;
		}
		if (values[1] >= size) 
		{
			MPI_Send(values, 2, MPI_INT, status.MPI_SOURCE, STOP_TAG, MPI_COMM_WORLD);
			activeSlaves--;
		}
		else
		{
			MPI_Send(values, 2, MPI_INT, status.MPI_SOURCE, WORK_TAG, MPI_COMM_WORLD);
			values[0]++;
		}
	}
	return answer;
}
