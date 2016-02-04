/*
 * Neuromapp - test.cpp, Copyright (c), 2015,
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
 * @file neuromapp/test/queuing/test.cpp
 *  Test on the Queueing Miniapp.
 */

#define BOOST_TEST_MODULE QueueingTest
#define IMPL T::impl

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/filesystem.hpp>

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include "coreneuron_1.0/queueing/queueing.h"
#include "coreneuron_1.0/queueing/pool.h"
#include "coreneuron_1.0/queueing/thread.h"
#include "utils/error.h"
#include "utils/storage/neuromapp_data.h"
#include "coreneuron_1.0/common/data/helper.h"
#include "test/coreneuron_1.0/test_header.hpp"

namespace bfs = ::boost::filesystem;

//UNIT TESTS

/*
 * Unit test for Pool::chooseDst function
 *
 *     - Test boundary case where percent-ite = 0%, all events should be self-events
 *       (i.e. if src == 0, dst == 0)
 *     - Test boundary where percent-ite = 100%, no events should be self-events
 *       (i.e. if src == 0, dst != 0)
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(pool_choose_dest, T, full_test_types){
	//percent-ite = 0%
	queueing::Pool<IMPL> pl1(false, 20, 0, false, false);
	int dst = 0;
	for(int i = 0; i < 10; ++i){
		dst = pl1.chooseDst(0);
		BOOST_CHECK(dst == 0);
	}

	//percent-ite = 100%
	queueing::Pool<IMPL> pl2(false, 20, 100, false, false);
	for(int i = 0; i < 10; ++i){
		dst = pl2.chooseDst(0);
		BOOST_CHECK(dst != 0);
	}
}

/*
 * Unit test for Thread::selfSend function
 *
 *    - after n number of self-send events:
 *    		PQSize == n
 *          enqueued_ == n
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(thread_self_send, T, full_test_types){
	queueing::NrnThreadData<IMPL> nt;
	double data = 0.0;
	double time = 0.0;
	int n = rand() % 10;

	for(int i = 0; i < n; ++i){
		nt.selfSend(data,(time + i));
	}
	BOOST_CHECK(nt.PQSize() == n);
	BOOST_CHECK(nt.enqueued_ == n);
}

/*
 * Unit test for Thread::interThreadSend function
 *
 *    - after n number of interThreadSend events:
 *    		interThreadSize == n
 *          inter_thread_received_ == n
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(thread_inter_send, T, full_test_types){
	queueing::NrnThreadData<IMPL> nt;
	double data = 0.0;
	double time = 0.0;
	int n = rand() % 10;

	for(int i = 0; i < n; ++i){
		nt.interThreadSend(data, (time + i));
	}
	BOOST_CHECK(nt.interThreadSize() == n);
	BOOST_CHECK(nt.ite_received_ == n);
}

/*
 * Unit test for Thread::enqueueEvents function
 *
 *    - checks the combination of enqueue and selfsend:
 *    		n self sends increases PQSize by n
 *    		m interThreadSends followed by an enqueue increases PQSize by m
 *    - PQSize = m + n
 *    - enqueued = m + n
 *    - interThread should be empty
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(thread_enqueue, T, full_test_types){
	queueing::NrnThreadData<IMPL> nt;
	double data = 0.0;
	double time = 0.0;
	int m = (rand() % 10) + 1;
	int n = (rand() % 10);

	nt.selfSend(0.0,4.0);

	//interThreadSends/enqueue
	for(int i = 0; i < n; ++i){
		nt.interThreadSend(data,(time + i));
	}
	nt.enqueueMyEvents();

	//selfSends
	for(int i = 0; i < (m - 1); ++i){
		nt.selfSend(data,(time + i));
	}

	BOOST_CHECK(nt.PQSize() == (m + n));
	BOOST_CHECK(nt.interThreadSize() == 0);
	BOOST_CHECK(nt.enqueued_ == (m + n));
}

/*
 * Unit test for Thread::deliver function
 *
 *     - verify that deliver function performs correctly
 *     (all events with time <= til are delivered)
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(thread_deliver, T, full_test_types){
    queueing::NrnThreadData<IMPL> nt;

	//enqueue 6 events
    nt.selfSend(0.0,4.0);
    nt.interThreadSend(0.0,1.0);
    nt.interThreadSend(0.0,2.0);
    nt.interThreadSend(0.0,3.0);
    nt.selfSend(0.0,5.0);
    nt.enqueueMyEvents();
    nt.selfSend(0.0,6.0);
    BOOST_CHECK(nt.PQSize() == 6);


	//deliver the first item
	int til = 6;
    nt.deliver(0,til);
    BOOST_CHECK(nt.delivered_ == 1);
    BOOST_CHECK(nt.PQSize() == 5);

	//deliver the next 2
	til = 3;
    while(nt.deliver(0,til))
        ;
    BOOST_CHECK(nt.delivered_ == 3);
    BOOST_CHECK(nt.PQSize() == 3);
    std::cout<<"Delivered "<<nt.delivered_<<std::endl;
    std::cout<<"PQSize "<<nt.PQSize()<<std::endl;

	//deliver the remaining
	til = 6;
    while(nt.deliver(0,til))
        ;
    BOOST_CHECK(nt.delivered_ == 6);
    BOOST_CHECK(nt.PQSize() == 0);
}

/**
 * Unit test for net_receive function
 *
 * Checks the validity of "coreneuron_1.0_net_receive_data after net_receive
 * using helper_check function (provided from coreneuron_1.0/common/data)
 */
BOOST_AUTO_TEST_CASE(net_receive){
    char name[] = "coreneuron_1.0_net_receive_data";
    std::string data = mapp::data_test();

    std::vector<char> chardata(data.begin(), data.end());
    chardata.push_back('\0');
    NrnThread* nt_ = (NrnThread*) storage_get(name, make_nrnthread, &chardata[0], free_nrnthread);
    if(nt_ == NULL){
        std::cout<<"Error: Unable to open data file"<<std::endl;
        storage_clear(name);
	exit(EXIT_FAILURE);
    }

    mech_net_receive(nt_, &(nt_->ml[18]));

    mapp::helper_check(name, "net_receive", mapp::data_test());
}

//INTEGRATION TESTS
/*
 * Tests that the program executes using mutex/vector implementation
 */
BOOST_AUTO_TEST_CASE(mutex_test){
    char arg1[]="NULL";
    char arg2[]="--numthread=8";
    char arg3[]="--eventsper=25";
    char arg4[]="--simtime=50";
    char arg5[]="--percent-ite=90";
    char * const argv[] = {arg1, arg2, arg3, arg4, arg5};
    int argc = 5;
    BOOST_CHECK(queueing_execute(argc,argv)==0);
    neuromapp_data.clear("inter_received");
    neuromapp_data.clear("enqueued");
    neuromapp_data.clear("delivered");
    neuromapp_data.clear("spikes");
}

/*
 * Tests that the program executes using spinlock/linked-list implementation
 */
BOOST_AUTO_TEST_CASE(spinlock_test){
    char arg1[]="NULL";
    char arg2[]="--numthread=8";
    char arg3[]="--eventsper=25";
    char arg4[]="--simtime=50";
    char arg5[]="--percent-ite=90";
    char arg6[]="--spinlock";
    char * const argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
    int argc = 6;
    BOOST_CHECK(queueing_execute(argc,argv)==0);
    neuromapp_data.clear("inter_received");
    neuromapp_data.clear("enqueued");
    neuromapp_data.clear("delivered");
    neuromapp_data.clear("spikes");
}

/*
 * Tests that the program executes using with-algebra option
 */
BOOST_AUTO_TEST_CASE(mech_solver){
    char arg1[]="NULL";
    char arg2[]="--numthread=8";
    char arg3[]="--eventsper=25";
    char arg4[]="--simtime=100";
    char arg5[]="--percent-ite=0";
    char arg6[]="--with-algebra";
    char * const argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
    int argc = 6;

    BOOST_CHECK(queueing_execute(argc,argv)==0);
    neuromapp_data.clear("inter_received");
    neuromapp_data.clear("enqueued");
    neuromapp_data.clear("delivered");
    neuromapp_data.clear("spikes");
}

/**
 * Verifies that the expected results occur when percent_ite = 100%:
 *
 * 	  - inter_received should equal the total number of events in the simulation
 * 	    (simtime * cellgroups * eventsper)
 * 	  - enqueued events should not exceed the number of inter_thread events
 * 	  - spikes should equal 0
 */
BOOST_AUTO_TEST_CASE(full_ite){
    char arg1[]="NULL";
    char arg2[]="--numthread=8";
    char arg3[]="--eventsper=25";
    char arg4[]="--simtime=25";
    char arg5[]="--percent-ite=100";
    char * const argv[] = {arg1, arg2, arg3, arg4, arg5, arg5};
    int argc = 5;
    BOOST_CHECK(queueing_execute(argc,argv)==0);
    std::string key1("inter_received");
    BOOST_CHECK(neuromapp_data.has<int>(key1));
    std::string key2("enqueued");
    BOOST_CHECK(neuromapp_data.has<int>(key2));
    std::string key3("spikes");
    BOOST_CHECK(neuromapp_data.has<int>(key3));

    const int simtime = 25;
    const int cellgroups = 64;
    const int eventsper = 25;
    //verify the correct number of inter-thread events were received
    BOOST_CHECK(neuromapp_data.get<int>(key1) == (simtime * cellgroups * eventsper));
    BOOST_CHECK(neuromapp_data.get<int>(key2) <= (simtime * cellgroups * eventsper));
    BOOST_CHECK(neuromapp_data.get<int>(key3) == 0);
    neuromapp_data.clear("inter_received");
    neuromapp_data.clear("enqueued");
    neuromapp_data.clear("delivered");
    neuromapp_data.clear("spikes");
}

/**
 * Verifies that the expected results occur when percent_ite = 100% (spinlock):
 *
 * 	  - inter_received should equal the total number of events in the simulation
 * 	    (simtime * cellgroups * eventsper)
 * 	  - enqueued events should not exceed the number of inter_thread events
 * 	  - spikes should equal 0
 */
BOOST_AUTO_TEST_CASE(full_ite_spinlock){
    char arg1[]="NULL";
    char arg2[]="--numthread=8";
    char arg3[]="--eventsper=25";
    char arg4[]="--simtime=25";
    char arg5[]="--percent-ite=100";
    char arg6[]="--spinlock";
    char * const argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
    int argc = 6;
    BOOST_CHECK(queueing_execute(argc,argv)==0);
    std::string key1("inter_received");
    BOOST_CHECK(neuromapp_data.has<int>(key1));
    std::string key2("enqueued");
    BOOST_CHECK(neuromapp_data.has<int>(key2));
    std::string key3("spikes");
    BOOST_CHECK(neuromapp_data.has<int>(key3));

    const int simtime = 25;
    const int cellgroups = 64;
    const int eventsper = 25;
    //verify the correct number of inter-thread events were received
    BOOST_CHECK(neuromapp_data.get<int>(key1) == (simtime * cellgroups * eventsper));
    BOOST_CHECK(neuromapp_data.get<int>(key2) <= (simtime * cellgroups * eventsper));
    BOOST_CHECK(neuromapp_data.get<int>(key3) == 0);
    neuromapp_data.clear("inter_received");
    neuromapp_data.clear("enqueued");
    neuromapp_data.clear("delivered");
    neuromapp_data.clear("spikes");
}

/**
 * Verifies that the expected results occur when percent_ite = 0%:
 *
 * 	  - inter_received should equal 0
 * 	  - enqueued events should equal the total number of events
 * 	    (simtime * cellgroups * eventsper)
 */
BOOST_AUTO_TEST_CASE(no_ite){
    char arg1[]="NULL";
    char arg2[]="--numthread=8";
    char arg3[]="--eventsper=25";
    char arg4[]="--simtime=25";
    char arg5[]="--percent-ite=0";
    char * const argv[] = {arg1, arg2, arg3, arg4, arg5};
    int argc = 5;
    BOOST_CHECK(queueing_execute(argc,argv)==0);

    std::string key1("inter_received");
    BOOST_CHECK(neuromapp_data.has<int>(key1));
    std::string key2("enqueued");
    BOOST_CHECK(neuromapp_data.has<int>(key2));

    const int simtime = 25;
    const int cellgroups = 64;
    const int eventsper = 25;
    //verify that no inter-thread events were received
    BOOST_CHECK( neuromapp_data.get<int>(key1) == 0);
    //verify that the correct number of events were enqueued
    BOOST_CHECK(neuromapp_data.get<int>(key2) == (simtime * cellgroups * eventsper));
    neuromapp_data.clear("inter_received");
    neuromapp_data.clear("enqueued");
    neuromapp_data.clear("delivered");
    neuromapp_data.clear("spikes");
}

/**
 * Verifies that the expected results occur when percent_ite = 0% (spinlock):
 *
 * 	  - inter_received should equal 0
 * 	  - enqueued events should equal the total number of events
 * 	    (simtime * cellgroups * eventsper)
 */
BOOST_AUTO_TEST_CASE(no_ite_spinlock){
    char arg1[]="NULL";
    char arg2[]="--numthread=8";
    char arg3[]="--eventsper=25";
    char arg4[]="--simtime=25";
    char arg5[]="--percent-ite=0";
    char arg6[]="--spinlock";
    char * const argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
    int argc = 6;
    BOOST_CHECK(queueing_execute(argc,argv)==0);

    std::string key1("inter_received");
    BOOST_CHECK(neuromapp_data.has<int>(key1));
    std::string key2("enqueued");
    BOOST_CHECK(neuromapp_data.has<int>(key2));

    const int simtime = 25;
    const int cellgroups = 64;
    const int eventsper = 25;
    //verify that no inter-thread events were received
    BOOST_CHECK( neuromapp_data.get<int>(key1) == 0);
    //verify that the correct number of events were enqueued
    BOOST_CHECK(neuromapp_data.get<int>(key2) == (simtime * cellgroups * eventsper));
    neuromapp_data.clear("inter_received");
    neuromapp_data.clear("enqueued");
    neuromapp_data.clear("delivered");
    neuromapp_data.clear("spikes");
}

/**
 * Verifies that the expected results occur when spike_enabled:
 *
 * 	  - spike events should equal the total number of events * percentspike
 * 	  	(simtime * cellgroups * eventsper * percentspike / 100)
 */
BOOST_AUTO_TEST_CASE(spike_enabled){
    char arg1[]="NULL";
    char arg2[]="--numthread=8";
    char arg3[]="--eventsper=25";
    char arg4[]="--simtime=25";
    char arg5[]="--percent-ite=0";
    char arg6[]="--spike-enabled";
    char * const argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
    int argc = 6;
    BOOST_CHECK(queueing_execute(argc,argv)==0);

    std::string key1("spikes");
    BOOST_CHECK(neuromapp_data.has<int>(key1));

    const int simtime = 25;
    const int cellgroups = 64;
    const int eventsper = 25;
    const int percentspike = 3;
    //verify that the correct number of spikes were sent
    BOOST_CHECK(neuromapp_data.get<int>(key1) ==
		    (simtime * cellgroups * eventsper * percentspike / 100));
    neuromapp_data.clear("inter_received");
    neuromapp_data.clear("enqueued");
    neuromapp_data.clear("delivered");
    neuromapp_data.clear("spikes");
}
