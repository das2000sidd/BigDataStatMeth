#ifndef rhdf5Utils
#define rhdf5Utils

  #include <RcppEigen.h>
  #include <iostream>
  #include <string>
  #include "H5Cpp.h"
  #include <sys/stat.h>
  #include "ReadDelayedData.h"

  // [[Rcpp::depends(RcppEigen)]]

  using namespace H5;
  using namespace Rcpp;
  
  const int	 RANK1 = 1;
  const int	 RANK2 = 2;
  const int	DIM1 = 1;
  const int	DIM2 = 2;
  const int	MAXSTRING = 32;
  
  // a typedef for our managed H5File pointer
  typedef std::shared_ptr<H5::H5File> H5FilePtr;

  bool ResFileExist(const std::string& name);
  bool RemoveFile(std::string filename);
  
  extern "C" H5FilePtr Open_hdf5_file(const std::string& fname);
  extern "C" int create_HDF5_dataset(H5std_string filename, const std::string CDatasetName,
                                    const size_t rows, const size_t cols, std::string strdatatype);

  extern "C" int create_HDF5_group(H5std_string filename, const H5std_string hiCGroup);
  extern "C" int Create_hdf5_file(std::string filename);
  extern "C" int create_HDF5_matrix(H5std_string filename, const std::string DatasetName, RObject DatasetValues);
  extern "C" int write_HDF5_matrix(H5std_string filename, const std::string CDatasetName, RObject DatasetValues);
  /*** extern "C" int read_HDF5_matrix_subset (H5std_string filename, const std::string CDatasetName,
                               IntegerVector ivoffset, IntegerVector ivcount,
                               IntegerVector ivstride, IntegerVector ivblock,
                               double* rdatablock); ***/
  
  extern "C" int write_HDF5_matrix_subset(H5std_string filename, const std::string CDatasetName, 
                                         IntegerVector ivoffset, IntegerVector ivcount,
                                         IntegerVector ivstride, IntegerVector ivblock,
                                         RObject DatasetValues);
  
  extern "C" int write_HDF5_matrix_subset_v2( H5File* file, DataSet* dataset,
                                             IntegerVector ivoffset, IntegerVector ivcount,
                                             IntegerVector ivstride, IntegerVector ivblock,
                                             RObject DatasetValues);
  
  extern "C" int read_HDF5_matrix_subset(H5File* file, DataSet* dataset,
                                         IntegerVector ivoffset, IntegerVector ivcount,
                                         IntegerVector ivstride, IntegerVector ivblock,
                                         double* rdatablock);
  
  int Create_HDF5_matrix_file(std::string filename, RObject mat, Rcpp::Nullable<std::string> folder);

#endif