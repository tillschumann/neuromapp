/*
 * Neuromapp - main.cpp, Copyright (c), 2015,
 * Kai Langen - Swiss Federal Institute of technology in Lausanne,
 * kai.langen@epfl.ch,
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

/**
 * @file neuromapp/event_passing/drivers/main.cpp
 * Event Passing Miniapp
 */

#include <iostream>
#include <string>
#include <sstream>
#include <boost/program_options.hpp>
#include <stdlib.h>

#include "hdf5/drivers/h5read.h"

#include "hdf5/data/helper.h"
#include "utils/error.h"
#include "neuromapp/utils/mpi/mpi_helper.h"

/** namespace alias for boost::program_options **/
namespace po = boost::program_options;

/** \fn spike_help(int argc, char *const argv[], po::variables_map& vm)
    \brief Helper using boost program option to facilitate the command line manipulation
    \param argc number of argument from the command line
    \param argv the command line from the driver or external call
    \param vm encapsulate the command line
    \return error message from mapp::mapp_error
 */
int hdf5_h5read_help(int argc, char* const argv[], po::variables_map& vm){
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help", "produce this help message")
    ("numprocs", po::value<size_t>()->default_value(8),"the number of MPI processes")
    ("numthreads", po::value<size_t>()->default_value(8),"the number of OMP threads per process")
    ("run", po::value<std::string>()->default_value(launcher_helper::mpi_launcher()),"the command to run parallel jobs")
    ("path", po::value< std::string >()->default_value(hdf5::testdata_compound()),"path to hdf5 synapse file")
    ("dataset", po::value< std::string >()->default_value("syn"),"name of the dataset")
    ("names", po::value< std::string >()->default_value("target,delay,weight,U0,TauRec,TauFac"),"names of used datasets, split names with comma")
    ("transferSize", po::value< size_t >()->default_value(524288),"specify number of loaded columns per io call")
    ("totalSize", po::value< size_t >()->default_value(-1),"specify number of loaded columns from file")
    ("flags", po::value< std::string >()->default_value(""),"set additional flags");

    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")){
        std::cout << desc;
        return mapp::MAPP_USAGE;
    }

    if(vm["numthreads"].as<size_t>() < 1){
        std::cout<<"must execute on at least 1 thread"<<std::endl;
        return mapp::MAPP_BAD_ARG;
    }

    if(vm["numprocs"].as<size_t>() < 1){
        std::cout<<"must execute on at least 1 process"<<std::endl;
        return mapp::MAPP_BAD_ARG;
    }

    if (vm["path"].as< std::string >() == "") {
        std::cout<<"path to hdf5 has to be set correctly"<<std::endl;
        return mapp::MAPP_BAD_ARG;
    }

    return mapp::MAPP_OK;
}

/** \fn event_content(po::variables_map const& vm)
    \brief Execute event passing app by calling mpirun/srun on binary file
    \param vm encapsulate the command line and all needed informations
 */
void hdf5_h5read_content(po::variables_map const& vm){
    std::stringstream command;
    std::string path = helper_build_path::mpi_bin_path();

    // no-multi thread so far
    //size_t nthread = vm["numthreads"].as<size_t>();
    std::string mpi_run = vm["run"].as<std::string>();
    size_t nproc = vm["numprocs"].as<size_t>();
    //command line args
    size_t transferSize = vm["transferSize"].as<size_t>();
    size_t totalSize = vm["totalSize"].as<size_t>();
    std::string filepath = vm["path"].as< std::string >();
    std::string dataset = vm["dataset"].as< std::string >();
    std::string names = vm["names"].as< std::string >();
    std::string flags = vm["flags"].as< std::string >();

    std::string exec ="h5read_distributed_exec";

    command << " " <<
        mpi_run <<" -n "<< nproc << " " <<
        flags << " " <<
        path << exec << " " <<
        filepath << " " <<
        dataset << " " <<
        transferSize << " " <<
        totalSize << " ";
    //split names list
    std::string delimiter = ",";
    size_t pos = 0;
    std::string token;
    while ((pos = names.find(delimiter)) != std::string::npos) {
        token = names.substr(0, pos);
        command << token << " ";
        names.erase(0, pos + delimiter.length());
    }

    std::cout<< "Running command " << command.str() <<std::endl;
	system(command.str().c_str());
}

int hdf5::h5read::execute(int argc, char* const argv[]){
    try {
        po::variables_map vm; // it contains everything
        if(int error = hdf5_h5read_help(argc, argv, vm)) return error;
        hdf5_h5read_content(vm); // execute the miniapp
    }
    catch(std::exception& e){
        std::cout << e.what() << "\n";
        return mapp::MAPP_UNKNOWN_ERROR;
    }
    return mapp::MAPP_OK; // 0 ok, 1 not ok
}
