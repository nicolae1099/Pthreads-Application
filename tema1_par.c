/*
 * APD - Tema 1
 * Octombrie 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#ifndef NOMINMAX

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif  /* NOMINMAX */


pthread_barrier_t barrier;

char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;
int number_of_threads;


// structura pentru un numar complex
typedef struct _complex {
	double a;
	double b;
} complex;

// structura pentru parametrii unei rulari
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;

params par1;
params par2;
int width_julia, width_mandel, height_julia, height_mandel;
int **result_julia;
int **result_mandel;

// citeste argumentele programului
void get_args(int argc, char **argv)
{
	if (argc < 6) {
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot P\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
	number_of_threads = atoi(argv[5]);
}

// citeste fisierul de intrare
void read_input_file(char *in_filename, params* par)
{
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}

// scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename, int **result, int width, int height)
{
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", width, height);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

// aloca memorie pentru rezultat
int **allocate_memory(int width, int height)
{
	int **result;
	int i;

	result = malloc(height * sizeof(int*));
	if (result == NULL) {
		printf("Eroare la malloc!\n");
		exit(1);
	}

	for (i = 0; i < height; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	return result;
}

// elibereaza memoria alocata
void free_memory(int **result, int height)
{
	int i;

	for (i = 0; i < height; i++) {
		free(result[i]);
	}
	free(result);
}

// ruleaza algoritmul Julia
void run_julia(params *par, int **result, int width, int height, int arg)
{
	int w, h;
	int thread_id = arg;
	int start = thread_id * height / number_of_threads;
	int end = min((thread_id + 1) * height / number_of_threads, height);

	for (w = 0; w < width; w++) {
		for (h = start; h < end; h++) {
			int step = 0;
			complex z = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + par->c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par->c_julia.b;

				step++;
			}
			result_julia[height-h-1][w] = step % 256;
		}
		pthread_barrier_wait(&barrier);
	}
}


// ruleaza algoritmul Mandelbrot
void run_mandelbrot(params *par, int **result, int width, int height, int arg)
{
	int w, h;
	int thread_id = arg;
	int start = thread_id * height / number_of_threads;
	int end = min((thread_id + 1) * height / number_of_threads, height);

	for (w = 0; w < width; w++) {
		for (h = start; h < end; h++) {
			complex c = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };
			complex z = { .a = 0, .b = 0 };
			int step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}
			result_mandel[height-h-1][w] = step % 256;
		}
		pthread_barrier_wait(&barrier);
	}

	pthread_exit(NULL);
}

void *thread_function(void *arg) {
	int thread_id = *(int *)arg;

	run_julia(&par1, result_julia, width_julia, height_julia, thread_id);
	pthread_barrier_wait(&barrier);

	run_mandelbrot(&par2, result_mandel, width_mandel, height_mandel, thread_id);
	pthread_barrier_wait(&barrier);

	pthread_exit(NULL);	
}


int main(int argc, char *argv[])
{
	// se citesc argumentele programului
	get_args(argc, argv);
	pthread_t tid[number_of_threads];
	int thread_id[number_of_threads];
	pthread_barrier_init(&barrier, NULL, number_of_threads);
	
	read_input_file(in_filename_julia, &par1);
	width_julia = (par1.x_max - par1.x_min) / par1.resolution;
	height_julia = (par1.y_max - par1.y_min) / par1.resolution;
	result_julia = allocate_memory(width_julia, height_julia);

	read_input_file(in_filename_mandelbrot, &par2);
	width_mandel = (par2.x_max - par2.x_min) / par2.resolution;
	height_mandel = (par2.y_max - par2.y_min) / par2.resolution;
	result_mandel = allocate_memory(width_mandel, height_mandel);
	
	for (int i = 0; i < number_of_threads; i++) {
		thread_id[i] = i;
		pthread_create(&tid[i], NULL, thread_function, &thread_id[i]);
	}
		// se asteapta thread-urile
	for (int i = 0; i < number_of_threads; i++) {
		pthread_join(tid[i], NULL);
	}
	pthread_barrier_destroy(&barrier);

	write_output_file(out_filename_julia, result_julia, width_julia, height_julia);
	free_memory(result_julia, height_julia);

	write_output_file(out_filename_mandelbrot, result_mandel, width_mandel, height_mandel);
	free_memory(result_mandel, height_mandel);
	return 0;
}
