#include <hdf5.h>
#include <mpi.h>
#include <algorithm>
#include <numeric>
#include <vector>
#include <cassert>

#include "nest/h5import/h5reader.h"

using namespace h5import;

  h5reader::h5reader( const std::string& path,
                      const std::vector< std::string >& datasets,
                      const unsigned long long& transfersize,
                      const unsigned long long& limittotalsize)
        : file_id_      ( H5I_INVALID_HID ),
          gid_          ( H5I_INVALID_HID ),
          memtype_      ( H5I_INVALID_HID ),
          dataset_ptr_  ( NULL ),
          global_offset_( 0 ),
          totalsize_    ( limittotalsize ),
          transfersize_ ( transfersize ),
          num_compound_ ( datasets.size() )
  {
    MPI_Comm_size( MPI_COMM_WORLD, &num_processes_ );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank_ );


    hid_t fapl_id = H5Pcreate( H5P_FILE_ACCESS );
    file_id_ = H5Fopen( path.c_str(), H5F_ACC_RDONLY, fapl_id );
    H5Pclose( fapl_id );

    gid_ = H5Gopen( file_id_, "/", H5P_DEFAULT );
    dataset_ptr_ = new h5dataset( this, "syn" );

    // read size and adapt if set
    const unsigned long long  real_size = this->size();
    if ( real_size < totalsize_ )
         totalsize_ = real_size;

    //only works if int and float have the same length
    assert( sizeof( int ) == sizeof( float ) );

    //create memtype
    memtype_ = H5Tcreate( H5T_COMPOUND, sizeof( int ) + num_compound_ );

    //target field is interpreted as int, everything else is interpreted as float
    for ( int i = 0; i < datasets.size(); i++ )
      if ( datasets[i] == "target" )
          H5Tinsert( memtype_, "target", i * sizeof( int ), H5T_NATIVE_INT );
      else
          H5Tinsert( memtype_, datasets[ i ].c_str(), i * sizeof( int ), H5T_NATIVE_FLOAT );


  }

  h5reader::~h5reader()
    {
      H5Tclose( memtype_ );

      delete dataset_ptr_;

      H5Gclose( gid_ );
      H5Fclose( file_id_ );
    }

    /*
     * Get num_syns synapses from datasets collectivly
     * Move file pointer to for next function call
     *
     */
    void h5reader::readblock( NESTSynapseList* synapses )
    {
      hsize_t private_offset = transfersize_ * rank_ + global_offset_;

      hssize_t count = std::min( ( hssize_t ) transfersize_,
        ( ( hssize_t ) totalsize_ - ( hssize_t ) private_offset ) );
      if ( count < 0 )
        count = 0;
      h5view dataspace_view( count, private_offset );

      hid_t dataspace_id = H5Dget_space( dataset_ptr_->id() );
      hid_t memspace_id = H5I_INVALID_HID;

      // be careful if there are no entries to load
      if ( dataspace_view.count[ 0 ] > 0 )
      {
        H5Sselect_hyperslab( dataspace_id,
          H5S_SELECT_SET,
          dataspace_view.offset,
          dataspace_view.stride,
          dataspace_view.count,
          dataspace_view.block );
        memspace_id = H5Screate_simple( 1, dataspace_view.count, NULL );
      }
      else
      {
        H5Sselect_none( dataspace_id );
        memspace_id = H5Scopy( dataspace_id );
        H5Sselect_none( memspace_id );
      }

      synapses->resize( dataspace_view.count[ 0 ] );

      // setup collective read operation
      hid_t dxpl_id = H5Pcreate( H5P_DATASET_XFER );

      H5Dread( dataset_ptr_->id(),
               memtype_,
               memspace_id,
               dataspace_id,
               dxpl_id,
               &synapses->property_pool_[0] );

      H5Pclose( dxpl_id );

      H5Sclose( memspace_id );
      H5Sclose( dataspace_id );

      //increase offset for next iteration
      global_offset_ += transfersize_ * num_processes_;
    }

