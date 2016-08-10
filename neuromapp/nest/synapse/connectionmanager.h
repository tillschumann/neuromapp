/*
 * connectionmanager.h
 *
 *  Created on: Jun 27, 2016
 *      Author: schumann
 */

#ifndef CONNECTIONMANAGER_H_
#define CONNECTIONMANAGER_H_

#include <boost/program_options.hpp>
#include "nest/synapse/connector_base.h"
#include "nest/libnestutil/sparsetable.h"
#include "nest/synapse/models/tsodyks2.h"
#include "nest/synapse/event.h"

#include "coreneuron_1.0/event_passing/environment/presyn_maker.h"

namespace po = boost::program_options;

struct PrintMemInfo
{
   int arena;     /* Non-mmapped space allocated (bytes) */
   int ordblks;   /* Number of free chunks */
   int smblks;    /* Number of free fastbin blocks */
   int hblks;     /* Number of mmapped regions */
   int hblkhd;    /* Space allocated in mmapped regions (bytes) */
   int usmblks;   /* Maximum total allocated space (bytes) */
   int fsmblks;   /* Space in freed fastbin blocks (bytes) */
   int uordblks;  /* Total allocated space (bytes) */
   int fordblks;  /* Total free space (bytes) */
   int keepcost;  /* Top-most, releasable space (bytes) */
    long id;
    struct mallinfo memInfo;
    PrintMemInfo(const int& id): id(id)
    {
        memInfo = mallinfo();
       arena = memInfo.arena;     /* Non-mmapped space allocated (bytes) */
       ordblks = memInfo.ordblks;   /* Number of free chunks */
       smblks = memInfo.smblks;    /* Number of free fastbin blocks */
       hblks = memInfo.hblks;     /* Number of mmapped regions */
       hblkhd = memInfo.hblkhd;    /* Space allocated in mmapped regions (bytes) */
       usmblks = memInfo.usmblks;   /* Maximum total allocated space (bytes) */
       fsmblks = memInfo.fsmblks;   /* Space in freed fastbin blocks (bytes) */
       uordblks = memInfo.uordblks;  /* Total allocated space (bytes) */
       fordblks = memInfo.fordblks;  /* Total free space (bytes) */
       keepcost = memInfo.keepcost;  /* Top-most, releasable space (bytes) */
    }
    ~PrintMemInfo()
    {
        memInfo = mallinfo();
        std::cout << "id=" << id <<  " mem.arena=" << memInfo.arena-arena << "\n";
        std::cout << "id=" << id <<  " mem.ordblks=" << memInfo.ordblks-ordblks << "\n";
        std::cout << "id=" << id <<  " mem.smblks=" << memInfo.smblks-smblks << "\n";
        std::cout << "id=" << id <<  " mem.hblks=" << memInfo.hblks-hblks << "\n";
        std::cout << "id=" << id <<  " mem.hblkhd=" << memInfo.hblkhd-hblkhd << "\n";
        std::cout << "id=" << id <<  " mem.usmblks=" << memInfo.usmblks-usmblks << "\n";
        std::cout << "id=" << id <<  " mem.fsmblks=" << memInfo.fsmblks-fsmblks << "\n";
        std::cout << "id=" << id <<  " mem.uordblks=" << memInfo.uordblks-uordblks << "\n";
        std::cout << "id=" << id <<  " mem.fordblks=" << memInfo.fordblks-fordblks << "\n";
        std::cout << "id=" << id <<  " mem.keepcost=" << memInfo.keepcost-keepcost << "\n";

    }
};

namespace nest
{

    typedef google::sparsetable< ConnectorBase<>* > tSConnector; // for all neurons having targets
    typedef std::vector< tSConnector > tVSConnector;           // for all threads

    class connectionmanager {

    private:
        int ncells;
        po::variables_map const& vm;


        ConnectorBase<>* validate_source_entry( thread tid, index s_gid);
    public:
        tVSConnector connections_;

        connectionmanager(po::variables_map const& vm);
        void connect(thread t, index s_gid, targetindex target);
        void send( thread t, index sgid, event& e );
    };

    void build_connections_from_neuron(const thread& thrd,
                                       const environment::continousdistribution& neuro_vp_dist,
                                       const environment::presyn_maker& presyns,
                                       const std::vector<targetindex>& detectors_targetindex,
                                       connectionmanager& cm);
};


#endif /* CONNECTIONMANAGER_H_ */
