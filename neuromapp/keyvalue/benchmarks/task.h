/*
 * Neuromapp - task.h, Copyright (c), 2015,
 * Timothee Ewart - Swiss Federal Institute of technology in Lausanne,
 * timothee.ewart@epfl.ch,
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

#ifndef MAPP_TASK_H
#define MAPP_TASK_H

#include <vector>
#include <numeric>
#include <functional>

#include "keyvalue/meta.h"
#include "keyvalue/memory.h"
#include "keyvalue/memory.h"
#include "keyvalue/meta.h"
#include "keyvalue/utils/tools.h"
#include "keyvalue/utils/argument.h"
#include "keyvalue/utils/statistic.h"
#include "keyvalue/utils/trait.h"

#include "utils/mpi/timer.h"

//forward declaration
template<keyvalue::selector S>
class benchmark;

template<keyvalue::selector S>
keyvalue::statistic task(benchmark<S> const& b){
    typedef typename keyvalue::trait_meta<S>::meta_type meta_type; // get the good meta type
    // extract the group of memory and the argument
    keyvalue::group<meta_type> const& g = b.get_group(); //get the group
    keyvalue::argument const& a = b.get_args(); // get the arguments
    // build the needed function in function of the backend
    typename keyvalue::trait_meta<S>::keyvalue_type kv;

    // time variables
    int comp_time_us = 100 * a.usecase() * 1000;
    int adjust = 1000 * a.usecase()/a.threads();
    int sleep_time;

    // timers for computation with and without I/O
    mapp::timer t1,t2;

    do{

        /******************** Compute base line computation without I/O ********************/
        t1.tic();

        for (float st = 0; st < a.st(); st += a.md()) {
            for (float md = 0; md < a.md(); md += a.dt()) {
                #pragma omp parallel
                {
                    #pragma omp single nowait
                    {
                        for (int cg = 0; cg < a.cg(); cg++) {
                            const double * dep1 = g.meta_at(cg).value(); //address of the first element
                            const double * dep2 = (g.meta_at(cg).value()+1);  //address of the second element

                            sleep_time += (int) ((0.45 * comp_time_us) / a.threads()) + adjust;
                            #pragma omp task depend(inout:dep1)
                            {
                                // First computation - 45%
                                usleep((int) ((0.45 * comp_time_us) / a.threads()) + adjust);
                            }

                            sleep_time += (int) ((0.05 * comp_time_us) / a.threads()) + adjust;
                            #pragma omp task depend(inout:dep1, dep2)
                            {
                                // Linear algebra - 5%
                                usleep((int) ((0.05 * comp_time_us) / a.threads()) + adjust);
                            }

                            sleep_time += (int) ((0.38 * comp_time_us) / a.threads()) + adjust;
                            #pragma omp task depend(inout:dep1)
                            {
                                // Second computation - 38%
                                usleep((int) ((0.38 * comp_time_us) / a.threads()) + adjust);
                            }
                        }

                    } // omp single
                } // omp parallel

            }
            #pragma omp barrier

            sleep_time += (int) ((0.05 * comp_time_us)) + adjust;
            usleep((int) (0.05 * comp_time_us) + adjust);
        }

        t1.toc();

        /******************** Now run the computation with I/O ********************/
        t2.tic();
        // these two loops should be merged
        for (float st = 0; st < a.st(); st += a.md()) {
            for (float md = 0; md < a.md(); md += a.dt()) {
                #pragma omp parallel
                {
                    #pragma omp single nowait
                    {
                        for (int cg = 0; cg < a.cg(); cg++) { // correct cell group

                            const double * dep1 = g.meta_at(cg).value(); //address of the first element
                            const double * dep2 = (g.meta_at(cg).value()+1);  //address of the second element

                            #pragma omp task depend(inout:dep1)
                            {
                                // First computation - 45%
                                usleep((int) ((0.45 * comp_time_us) / a.threads()) + adjust);
                            }

                            #pragma omp task depend(inout:dep1, dep2)
                            {
                                // Linear algebra - 5%
                                usleep((int) ((0.05 * comp_time_us) / a.threads()) + adjust);
                            }

                            #pragma omp task depend(inout:dep2)
                            {
                                // I/O
                                kv.insert(g.meta_at(cg));
                            }

                            #pragma omp task depend(inout:dep1)
                            {
                                // Second computation - 38%
                                usleep((int) ((0.38 * comp_time_us) / a.threads()) + adjust);
                            }
                        } // end loop cg
                    } // omp single
                } // omp parallel
            } // for loop dt

            #pragma omp barrier

            // Spike exchange - 5%
            usleep((int) (0.05 * comp_time_us) + adjust);
        } // for loop st
        t2.toc();

        // Make sure time difference is positive
        if ((t2.time()-t1.time())< 0.)
            std::cout << "Negative time, computing again..." << std::endl;

    } while ((t2.time()-t1.time()) < 0);

    std::vector<double> vtime;
    vtime.push_back(t1.time());
    vtime.push_back(t2.time());

    return keyvalue::statistic(a, (a.st() / a.dt()) * a.cg(),
            a.voltages_size() * sizeof(keyvalue::nrnthread::value_type) / a.cg(), vtime);
}
#endif
