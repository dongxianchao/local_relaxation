/*
    Copyright 2017 Zheyong Fan, Ville Vierimaa, Mikko Ervasti, and Ari Harju
    This file is part of GPUMD.
    GPUMD is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    GPUMD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with GPUMD.  If not, see <http://www.gnu.org/licenses/>.
*/




#include "gpumd.cuh"

#include "force.cuh"
#include "validate.cuh"
#include "integrate.cuh"
#include "ensemble.cuh" 
#include "measure.cuh"
#include "atom.cuh"
#include "memory.cuh"
#include "error.cuh"
#include "io.cuh"

#include <ctype.h>




GPUMD::GPUMD(char *input_dir)
{
    // Data structures:
    Atom        atom(input_dir);
    Force       force;
    Integrate   integrate;
    Measure     measure(input_dir);

    run(input_dir, &atom, &force, &integrate, &measure);
}




GPUMD::~GPUMD(void)
{
    // nothing
}




/*----------------------------------------------------------------------------80
    run a number of steps for a given set of inputs
------------------------------------------------------------------------------*/
static void process_run 
(
    char **param, 
    unsigned int num_param, 
    char *input_dir,  
    Atom *atom,
    Force *force,
    Integrate *integrate,
    Measure *measure
)
{
    integrate->initialize(atom);
    measure->initialize(atom);

    // record the starting time for this run
    clock_t time_begin = clock();

    // Now, start to run!
    for (int step = 0; step < atom->number_of_steps; ++step)
    {  
        // update the neighbor list
        if (atom->neighbor.update)
        {
            atom->find_neighbor(0);
        }

        // set the current temperature;
        if (integrate->ensemble->type >= 1 && integrate->ensemble->type <= 20)
        {
            integrate->ensemble->temperature = atom->temperature1 
                + (atom->temperature2 - atom->temperature1)
                * real(step) / atom->number_of_steps;   
        }

        // integrate by one time-step:
        integrate->compute(atom, force, measure);

        // measure
        measure->compute(input_dir, atom, integrate, step);

        if (atom->number_of_steps >= 10)
        {
            if ((step + 1) % (atom->number_of_steps / 10) == 0)
            {
                printf("INFO:  %d steps completed.\n", step + 1);
            }
        }
    }
    
    // only for myself
    if (0)
    {
        validate_force(force, atom, measure);
    }

    printf("INFO:  This run is completed.\n\n");

    // report the time used for this run and its speed:
    clock_t time_finish = clock();
    real time_used = (time_finish - time_begin) / (real) CLOCKS_PER_SEC;
    printf("INFO:  Time used for this run = %g s.\n", time_used);
    real run_speed = atom->N * (atom->number_of_steps / time_used);
    printf("INFO:  Speed of this run = %g atom*step/second.\n\n", run_speed);

    measure->finalize(input_dir, atom, integrate);
    integrate->finalize();
}




/*----------------------------------------------------------------------------80
    set some default values after each run
------------------------------------------------------------------------------*/
static void initialize_run(Atom* atom, Measure* measure)
{
    atom->neighbor.update = 0;
    measure->heat.sample     = 0;
    measure->shc.compute     = 0;
    measure->vac.compute     = 0;
    measure->hac.compute     = 0;
    measure->hnemd.compute   = 0;
    atom->fixed_group     = -1; // no group has an index of -1
}




/*----------------------------------------------------------------------------80
	Read the input file to memory in the beginning, because
	we do not want to keep the FILE handle open all the time
------------------------------------------------------------------------------*/
static char *get_file_contents (char *filename)
{

    char *contents;
    int contents_size;
    FILE *in = my_fopen(filename, "r");

    // Find file size
    fseek(in, 0, SEEK_END);
    contents_size = ftell(in);
    rewind(in);

    MY_MALLOC(contents, char, contents_size + 1);
    int size_read_in = fread(contents, sizeof(char), contents_size, in);
    if (size_read_in != contents_size)
    {
        print_error ("File size mismatch.");
    }

    fclose(in);
    contents[contents_size] = '\0'; // Assures proper null termination

    return contents;
}




/*----------------------------------------------------------------------------80
	Parse a single row
------------------------------------------------------------------------------*/
static char *row_find_param (char *s, char *param[], int *num_param)
{
    *num_param = 0;
    int start_new_word = 1, comment_found = 0;
    if (s == NULL) return NULL;

    while(*s)
    {
        if(*s == '\n')
        {
            *s = '\0';
            return s + sizeof(char);
        }
        else if (comment_found)
        {
            // Do nothing
        }
        else if (*s == '#')
        {
            *s = '\0';
            comment_found = 1;
        }
        else if(isspace(*s))
        {
            *s = '\0';
            start_new_word = 1;
        }
        else if (start_new_word)
        {
            param[*num_param] = s;
            ++(*num_param);
            start_new_word = 0;
        }
        ++s;
    }
    return NULL;
}




/*----------------------------------------------------------------------------80
    Read and process the inputs from the "run.in" file.
------------------------------------------------------------------------------*/
void GPUMD::run
(
    char *input_dir,  
    Atom *atom,
    Force *force,
    Integrate *integrate,
    Measure *measure 
)
{
    char file_run[FILE_NAME_LENGTH];
    strcpy(file_run, input_dir);
    strcat(file_run, "/run.in");
    char *input = get_file_contents(file_run);
    char *input_ptr = input; // Keep the pointer in order to free later

    // Iterate the rows
    const int max_num_param = 10; // never use more than 9 parameters
    int num_param;
    char *param[max_num_param];

    initialize_run(atom, measure); // set some default values before the first run

    while (input_ptr)
    {
        // get one line from the input file
        input_ptr = row_find_param(input_ptr, param, &num_param);
        if (num_param == 0) { continue; } 

        // set default values
        int is_potential = 0;
        int is_velocity = 0;
        int is_run = 0;

        // parse a line of the input file 
        parse
        (
            param, num_param, atom, force, integrate, measure,
            &is_potential, &is_velocity, &is_run
        );

        // check for some special keywords
        if (is_potential) 
        {  
            force->initialize(input_dir, atom);
            force->compute(atom, measure);
            #ifdef FORCE
            // output the initial forces (for lattice dynamics calculations)
            int m = sizeof(real) * atom->N;
            real *cpu_fx = cpu_data->fx;
            real *cpu_fy = cpu_data->fy;
            real *cpu_fz = cpu_data->fz;
            CHECK(cudaMemcpy(cpu_fx, atom->fx, m, cudaMemcpyDeviceToHost));
            CHECK(cudaMemcpy(cpu_fy, atom->fy, m, cudaMemcpyDeviceToHost));
            CHECK(cudaMemcpy(cpu_fz, atom->fz, m, cudaMemcpyDeviceToHost));
            char file_force[FILE_NAME_LENGTH];
            strcpy(file_force, input_dir);
            strcat(file_force, "/f.out");
            FILE *fid_force = my_fopen(file_force, "w");
            for (int n = 0; n < atom->N; n++)
            {
                fprintf
                (
                    fid_force, "%20.10e%20.10e%20.10e\n", 
                    cpu_fx[n], cpu_fy[n], cpu_fz[n]
                );
            }
            fflush(fid_force);
            fclose(fid_force);
            #endif
        }
        if (is_velocity) { atom->initialize_velocity(); }
        if (is_run)
        {
            process_run
            (param, num_param, input_dir, atom, force, integrate, measure);
            initialize_run(atom, measure); // change back to the default
        }
    }

    MY_FREE(input); // Free the input file contents
}




