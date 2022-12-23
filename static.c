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
	int x, y, numProcs, rank, iterations, chunkSize;
	int size = SIZE, rest = 0;
	double answer = 0, finalResult = 0, startTime, endTime;

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

	if (rank == ROOT)
	{
		startTime = MPI_Wtime();

		chunkSize = size / numProcs; //calculate the size of each chunk per process 
		if ((size % numProcs) != 0)
		{
			rest = size % numProcs;
		}
	}
	if (numProcs < 2)
	{
		printf("Number of processes must be at least 2.\n");
		exit(1);
	}

	MPI_Bcast(&chunkSize, 1, MPI_INT, ROOT, MPI_COMM_WORLD); // send each worker the size of each chunk per process 

	for (int i = 0; i < iterations; i++)//do all the work from start to finish by the size of the iterations that was given
	{
		answer = 0, finalResult = 0;

		for (x = 0; x < size; x++)
			for (y = ((rank - 1) * chunkSize) ; y < (rank * chunkSize); y++)// each worker does only the relative chunk he needs to do
				answer += heavy(x, y);
		
		if (rank != ROOT)
		{
			if (rest > 0)
				for (x = 0; x < size; x++)
					for (y = size - rest - 1; y < size; y++)//the root will calculate the reminder of the work that had been left
						answer += heavy(x, y);
		}
		MPI_Reduce(&answer, &finalResult, 1, MPI_DOUBLE, MPI_SUM, ROOT, MPI_COMM_WORLD);// all the process sends there answer and the root calculates the final result by suming all the results
		MPI_Barrier(MPI_COMM_WORLD);//wait for all to return reult before going to next iteration
		
	}

	if (rank == ROOT)
	{
		endTime = MPI_Wtime();
		printf("answer = %e\n", finalResult);
		printf("Average Time measured: %.3f seconds.\n", (endTime - startTime) / iterations);
	}

	MPI_Finalize(); //end process

	return EXIT_SUCCESS;
}
