/*
 * Neuromapp - connector_base.h, Copyright (c), 2015,
 * Kai Langen - Swiss Federal Institute of technology in Lausanne,
 * kai.langen@epfl.ch,
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CONNECTOR_BASE_H
#define CONNECTOR_BASE_H

#include <cstdlib>
#include <iostream>
#include <vector>
#include <cassert>

#include <folly/small_vector.h>

#include "nest/synapse/node.h"
#include "nest/synapse/event.h"

// when to truncate the recursive instantiation
#define K_CUTOFF 8

/* Removed suicide_and_resurrect function */

namespace nest
{


// base class to provide interface to decide
// - homogeneous connector (containing =1 synapse type)
//    -- which synapse type stored (syn_id)
// - heterogeneous connector (containing >1 synapse type)
//
class tsodyks2; //forward declaration

template <class T = tsodyks2>
class ConnectorBase
{

/* Removed many functions that didnt relate to Connector build or send stage.
 * Some functions include getters and setters for synapse status, connections,
 * target gid's and homogeneous_model check.
 * Assume all models are homogeneous.
 */
public:
  ConnectorBase():t_lastspike_(0){}

  void send( event& e ) // , NEST: thread t  not necessary for MiniApp (see synapse)
  {
    for ( size_t i = 0; i < size(); i++ )
      v[i].send( e, get_t_lastspike());  //simplified thread is passed in real NEST too

    set_t_lastspike( e.get_stamp().get_ms() );
  }

  inline double get_t_lastspike() const {
    return t_lastspike_;
  }

  inline void set_t_lastspike( const double t_lastspike ) {
    t_lastspike_ = t_lastspike;
  }

  void push_back (const T& c){
      v.push_back(c);
  };
  size_t size (){
      return v.size();
  };

private:
  double t_lastspike_;
  folly::small_vector<T,8> v;
//  std::vector<T> v;
};
} //end namespace
#endif
