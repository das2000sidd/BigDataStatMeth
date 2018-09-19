#include <RcppEigen.h>
#include "beachmat/numeric_matrix.h"   // To access numeric matrix
#include "beachmat/integer_matrix.h"   // To access integer matrix
#include "beachmat/character_matrix.h" // To access character matrix

using Eigen::MatrixXd;                  // variable size matrix, double precision
using Eigen::MatrixXi;                  // variable size matrix, integer

using namespace Rcpp;

// [[Rcpp::export]]
Rcpp::RObject BDtrans_numeric (const Rcpp::RObject & x)
{
  
  auto dmtype = beachmat::find_sexp_type(x);
  
  if ( dmtype == INTSXP ) {
    
    auto dmat = beachmat::create_integer_matrix(x);
    const size_t ncols = dmat->get_ncol();
    const size_t nrows = dmat->get_nrow();
    
    Eigen::MatrixXi mateigen(nrows,ncols);
    Rcpp::IntegerVector output(dmat->get_nrow());
    
    for (size_t ccol=0; ccol<ncols; ++ccol) {
      dmat->get_col(ccol, output.begin());
      mateigen.col(ccol) = Rcpp::as<Eigen::VectorXi >(output);
    }
    
    mateigen.transposeInPlace();
    
    beachmat::output_param oparam(dmat->get_matrix_type(), FALSE, TRUE);  
    auto out_dmat = beachmat::create_integer_output(ncols, nrows, oparam);
    
    Rcpp::IntegerMatrix matint = wrap(mateigen);
    Rcpp::IntegerVector vint;
    
    for (size_t nrow=0; nrow <ncols; ++nrow) {
      vint = matint.row(nrow);
      out_dmat ->set_row(nrow, vint.begin());
    }
    
    return(out_dmat->yield());
    
  } else if (dmtype==REALSXP) {
    
    auto dmat = beachmat::create_numeric_matrix(x);
    const size_t ncols = dmat->get_ncol();
    const size_t nrows = dmat->get_nrow();
    
    Eigen::MatrixXd mateigen(nrows,ncols);
    Rcpp::NumericVector output(dmat->get_nrow());
    
    for (size_t ccol=0; ccol<ncols; ++ccol) {
      dmat->get_col(ccol, output.begin());
      mateigen.col(ccol) = Rcpp::as<Eigen::VectorXd >(output);
    }
    
    mateigen.transposeInPlace() ;
    
    beachmat::output_param oparam(dmat->get_matrix_type(), FALSE, TRUE);  
    auto out_dmat = beachmat::create_integer_output(ncols, nrows, oparam);
    
    Rcpp::NumericMatrix matd = wrap(mateigen);
    Rcpp::NumericVector vd;
    
    for (size_t nrow=0; nrow <ncols; ++nrow) {
      vd = matd.row(nrow);
      out_dmat ->set_row(nrow, vd.begin());
    }
    
    return(out_dmat->yield());
    
  } else {
    throw std::runtime_error("unacceptable matrix type");
  }
  
  
}


// [[Rcpp::export]]
Rcpp::RObject BDtrans_hdf5(const Rcpp::RObject & x)
{
  
  auto dmtype = beachmat::find_sexp_type(x);
  
  
  if ( dmtype == INTSXP ) {
    auto dmat = beachmat::create_integer_matrix(x);
    const size_t ncols = dmat->get_ncol();
    const size_t nrows = dmat->get_nrow();
    
    beachmat::output_param oparam(dmat->get_matrix_type(), FALSE, TRUE);  
    auto out_dmat = beachmat::create_integer_output(ncols, nrows, oparam);
    
    Rcpp::IntegerVector v_in(dmat->get_nrow());
    Rcpp::IntegerVector v_out(out_dmat->get_nrow());
    
    for (size_t ccol=0; ccol<ncols; ++ccol) {
      dmat->get_col(ccol, v_in.begin());
      out_dmat ->set_row(ccol,v_in.begin());
    }
    
    return (out_dmat->yield());
    
  } else if (dmtype==REALSXP) {
    
    auto dmat = beachmat::create_numeric_matrix(x);
    const size_t ncols = dmat->get_ncol();
    const size_t nrows = dmat->get_nrow();
    
    beachmat::output_param oparam(dmat->get_matrix_type(), FALSE, TRUE);  
    auto out_dmat = beachmat::create_numeric_output(ncols, nrows, oparam);
    
    Rcpp::NumericVector v_in(dmat->get_nrow());
    Rcpp::NumericVector v_out(out_dmat->get_nrow());
    
    for (size_t ccol=0; ccol<ncols; ++ccol) {
      dmat->get_col(ccol, v_in.begin());
      out_dmat ->set_row(ccol,v_in.begin());
    }
    
    return (out_dmat->yield());
    
  } else if (dmtype==STRSXP) {
    
    auto dmat = beachmat::create_character_matrix(x);
    const size_t ncols = dmat->get_ncol();
    const size_t nrows = dmat->get_nrow();
    
    beachmat::output_param oparam(dmat->get_matrix_type(), FALSE, TRUE);  
    auto out_dmat = beachmat::create_character_output(ncols, nrows, oparam);
    
    Rcpp::CharacterVector v_in(dmat->get_nrow());
    Rcpp::CharacterVector v_out(out_dmat->get_nrow());
    
    for (size_t ccol=0; ccol<ncols; ++ccol) {
      dmat->get_col(ccol, v_in.begin());
      out_dmat ->set_row(ccol,v_in.begin());
    }
    
    return (out_dmat->yield());
    
  } else {
    throw std::runtime_error("unacceptable matrix type");
  }
  
}


// You can include R code blocks in C++ files processed with sourceCpp
// (useful for testing and development). The R code will be automatically 
// run after the compilation.
//

/*** R

library(Rcpp)
library(DelayedArray)
library("microbenchmark")



# PROVES MATRIU ENTERS
  val <- matrix(rep(1:200, 1:200), ncol = 100)
  da_mat <- DelayedArray(seed = val) ; da_mat

  results <- microbenchmark(transR <-  t(da_mat),
                            transCppEigen <-  BDtrans_numeric(da_mat),
                            transCpp <-  BDtrans_hdf5(da_mat),
                            times = 50L)

  print(summary(results)[, c(1:7)],digits=1)
  
  #all.equal(transR,transCppEigen)
  #all.equal(transCpp,transCppEigen)
  

# PROVES MATRIU REALS
  provesdelay <- matrix(runif(100000), ncol=200, nrow=500)
  X <- DelayedArray(blah)

  results <- microbenchmark(transR <- t(X),
                            transCppEigen <- BDtrans_numeric(X),
                            transCpp <- BDtrans_hdf5(X),
                            times = 50L)
  print(summary(results)[, c(1:7)],digits=1)
  
# PROVES MATRIU CARACTERS
  delaystring <- matrix(sample(LETTERS, 10000, TRUE), ncol=200, nrow=50)
  X <- DelayedArray(delaystring)
  
  
  results <- microbenchmark(transR <- t(X),
                            #transCppEigen <- BDtrans_numeric(X),
                            transCpp <- BDtrans_hdf5(X),
                            times = 50L)
  print(summary(results)[, c(1:7)],digits=1)
*/
