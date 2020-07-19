#include "include/svdDecomposition.h"
#include "include/ReadDelayedData.h"

// SVD decomposition 
svdeig RcppbdSVD( Eigen::MatrixXd& X, int k, int ncv, bool bcenter, bool bscale )
{
  
  svdeig retsvd;
  Eigen::MatrixXd nX;
  int nconv;
  
  if( k==0 )    k = (std::min(X.rows(), X.cols()))-1;
  else if (k > (std::min(X.rows(), X.cols()))-1 ) k = (std::min(X.rows(), X.cols()))-1;
  
  if(ncv == 0)  ncv = k + 1 ;
  if(ncv<k) ncv = k + 1;
  
  {
    Eigen::MatrixXd Xtcp;
    //..//if(normalize ==true )  {
    if(bcenter ==true || bscale == true)  {
      // Xtcp =  Rcpp::as<Eigen::MatrixXd> (rcpp_parallel_tCrossProd( Rcpp::wrap(RcppNormalize_Data(X))));
      nX = RcppNormalize_Data(X, bcenter, bscale);
      Xtcp =  bdtcrossproduct(nX);
    }else {
      //Xtcp =  Rcpp::as<Eigen::MatrixXd> (rcpp_parallel_tCrossProd( Rcpp::wrap(X)));
      Xtcp =  bdtcrossproduct(X);
      
    }
    
    Spectra::DenseSymMatProd<double> op(Xtcp);
    Spectra::SymEigsSolver< double, Spectra::LARGEST_ALGE, Spectra::DenseSymMatProd<double> > eigs(&op, k, ncv);
    
    // Initialize and compute
    eigs.init();
    nconv = eigs.compute();
    
    if(eigs.info() == Spectra::SUCCESSFUL)
    {
      retsvd.d = eigs.eigenvalues().cwiseSqrt();
      retsvd.u = eigs.eigenvectors();
      retsvd.bokuv = true;
    } else {
      retsvd.bokuv = false;
    }
    
  }
  
  if(retsvd.bokuv == true)
  {
    Eigen::MatrixXd Xcp;
    if(bcenter ==true || bscale==true )  {
      // Xcp =  Rcpp::as<Eigen::MatrixXd> (rcpp_parallel_CrossProd( Rcpp::wrap(RcppNormalize_Data(X))));  
      Xcp =  bdcrossproduct(nX);  
    }else {
      // Xcp =  Rcpp::as<Eigen::MatrixXd> (rcpp_parallel_CrossProd( Rcpp::wrap(X)));
      Xcp =  bdcrossproduct(X);
    }  

    Spectra::DenseSymMatProd<double> opv(Xcp);
    Spectra::SymEigsSolver< double, Spectra::LARGEST_ALGE, Spectra::DenseSymMatProd<double> > eigsv(&opv, k, ncv);
    
    // Initialize and compute
    eigsv.init();
    nconv = eigsv.compute();
    
    // Retrieve results
    if(eigsv.info() == Spectra::SUCCESSFUL)
    {
      retsvd.v = eigsv.eigenvectors();
    } else {
      retsvd.bokd = false;
    }
    
  }
  
  return retsvd;
}




// Lapack SVD decomposition
svdeig RcppbdSVD_lapack( Eigen::MatrixXd& X,  bool bcenter, bool bscale )
{
  
  svdeig retsvd;
  
  char Schar='S';
  int info = 0;

  if(bcenter ==true || bscale == true)
    X = RcppNormalize_Data(X, bcenter, bscale);

  int m = X.rows();
  int n = X.cols();
  int lda = std::max(1,m);
  int ldu = std::max(1,m);
  int ldvt = std::min(m, n);
  int k = std::min(m,n);
  int lwork = std::max( 1, 4*std::min(m,n)* std::min(m,n) + 7*std::min(m, n) );
  
  Eigen::VectorXd s = Eigen::VectorXd::Zero(k);
  Eigen::VectorXd work = Eigen::VectorXd::Zero(lwork);
  Eigen::MatrixXd u = Eigen::MatrixXd::Zero(ldu,k);
  Eigen::MatrixXd vt = Eigen::MatrixXd::Zero(ldvt,n);

  dgesvd_( &Schar, &Schar, &m, &n, X.data(), &lda, s.data(), u.data(), &ldu, vt.data(), &ldvt, work.data(), &lwork, &info);
  
  retsvd.d = s;
  retsvd.u = u;
  retsvd.v = vt.transpose();
  
  return retsvd;
}





// ##' @param k number of local SVDs to concatenate at each level 
// ##' @param q number of levels
svdeig RcppbdSVD_hdf5_Block( H5File* file, DataSet* dataset, int k, int q, int nev, bool bcenter, bool bscale, 
                             int irows, int icols, Rcpp::Nullable<int> threads = R_NilValue )
{
  
  IntegerVector stride = IntegerVector::create(1, 1);
  IntegerVector block = IntegerVector::create(1, 1);
  svdeig retsvd;
  Eigen::MatrixXd nX;
  // int nconv, M, p, n;
  // int maxsizetoread;
  bool transp = false;
  std::string strGroupName  = "tmpgroup";
  std::string strPrefix;
  
  CharacterVector strvmatnames = {"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z"};
  strPrefix = strvmatnames[q-1];
  
  try{
    
    if(irows > icols)
      transp = true;
    
    // Rcpp::Rcout<<"\n First level\n";

    First_level_SvdBlock_decomposition_hdf5( file, dataset, k, q, nev, bcenter, bscale, irows, icols, threads);

    for(int j = 1; j < q; j++) // For each decomposition level : 
    {
      Next_level_SvdBlock_decomposition_hdf5(file, strGroupName, k, j, bcenter, bscale, threads);
    }
    
    // Rcpp::Rcout<<"\n Després First level\n";
    
    // Get dataset names
    StringVector joindata =  get_dataset_names_from_group(file, strGroupName, strPrefix);
    // Rcpp::Rcout<<"\n Dades a joinar : \n"<<joindata<<"\n";

    // 1.- Join matrix and remove parts from file
    std::string strnewdataset = std::string((joindata[0])).substr(0,1);
    join_datasets(file, strGroupName, joindata, strnewdataset);
    remove_HDF5_multiple_elements_ptr(file, strGroupName, joindata);

    // 2.- Get SVD from Blocks full mattrix

    DataSet datasetlast = file->openDataSet(strGroupName + "/" + strnewdataset);
    IntegerVector dims_out = get_HDF5_dataset_size(datasetlast);

    Eigen::MatrixXd matlast = GetCurrentBlock_hdf5(file, &datasetlast, 0, 0, dims_out[0],dims_out[1]);
    retsvd = RcppbdSVD_lapack(matlast, false, false);

    // 3.- crossprod initial matrix and svdA$u
    IntegerVector dims_out_first = get_HDF5_dataset_size(*dataset);
    // Get initial matrix
    Eigen::MatrixXd A = GetCurrentBlock_hdf5(file, dataset, 0, 0,dims_out_first[0], dims_out_first[1] );
    Eigen::MatrixXd v = Bblock_matrix_mul(A.transpose(),retsvd.u,128);
    
    // 4.- resuls / svdA$d
    v = v.array().rowwise()/(retsvd.d).transpose().array();
    
    // 5.- Retornem resultat : 
    //        --> Si transposta : u=v i v=u
    //        --> Sino : u=u i v=v
    //    --> Senzillament fer l'assignació final abans de retornar (easy)
    
    if (transp == true)
    {
      retsvd.v = retsvd.u;
      retsvd.u = v;
    } else
    {
      retsvd.v = v;
    }
    
  }catch(std::exception &ex) {
    Rcpp::Rcout<< ex.what();
  }
  
  return retsvd;
}











// SVD decomposition with hdf5 file
//    input data : hdf5 file (object from crossproduct matrix) datagroup = 'strsubgroupIN'
//    output data : hdf5 file svd data in datagroup svd 
//                        svd/d 
//                        svd/u 
//                        svd/v 
//                        
//  https://github.com/isglobal-brge/svdParallel/blob/8b072f79c4b7c44a3f1ca5bb5cba4d0fceb93d5b/R/generalBlockSVD.R
//  @param k number of local SVDs to concatenate at each level 
//  @param q number of levels
//  
svdeig RcppbdSVD_hdf5( std::string filename, std::string strsubgroup, std::string strdataset,  
                       int k, int q, int nev, bool bcenter, bool bscale )
{
  
  svdeig retsvd;
  Eigen::MatrixXd X;
  // int nconv;

  // Open an existing file and dataset.
  H5File file(filename, H5F_ACC_RDWR);
  DataSet dataset = file.openDataSet(strsubgroup + "/" + strdataset);

  // Get dataset dims
  //..// hsize_t * dims_out = get_HDF5_dataset_size(dataset);
  IntegerVector dims_out = get_HDF5_dataset_size(dataset);
  
  hsize_t offset[2] = {0,0};
  //..// hsize_t count[2] = {as<hsize_t>(dims_out[0]), as<hsize_t>(dims_out[1])};
  hsize_t count[2] = { (unsigned long long)dims_out[0], (unsigned long long)dims_out[1]};
  
  // Rcpp::Rcout<<"Que ens envia count ... "<<count[0]<<" , "<<count[1]<<"\n";
  
  // In memory computation for small matrices (rows or columns<5000)
  // Block decomposition for big mattrix
  if( std::max(dims_out[0], dims_out[1])<25 )
  {
    Rcpp::Rcout<<"Small matrix in memory process...\n";
    
    X = GetCurrentBlock_hdf5( &file, &dataset, offset[0], offset[1], count[0], count[1]);
    X.transposeInPlace();

    retsvd = RcppbdSVD(X, k, nev, bcenter, bscale);

  }
  else{
    Rcpp::Rcout<<"Small matrix in file process...\n";
    
    // data stored transposed in hdf5
    int xdim = (unsigned long long)dims_out[1];
    int ydim = (unsigned long long)dims_out[0];
    
    // Rcpp::Rcout<<"Valor k : "<<k<<"...\n";
    // Rcpp::Rcout<<"GO to RcppbdSVD_hdf5_Block...\n";
    
    
    retsvd = RcppbdSVD_hdf5_Block( &file, &dataset, k, q, nev, bcenter, bscale, xdim, ydim);
  }
  

  return retsvd;
   
}









svdeig RcppCholDec(const Eigen::MatrixXd& X)
{
  Eigen::MatrixXd mX = X;
  svdeig decomp;
  
  Eigen::LDLT<Eigen::MatrixXd> cholSolv = mX.ldlt();

  // if symetric + positive definite -> info = Success
  // else no Cholesky Decomposition --> svd decomposition with Spectra
  if(cholSolv.info()==Eigen::Success)
  {
    size_t n = cholSolv.cols();
    decomp.d = cholSolv.vectorD();
    Eigen::MatrixXd preinv = Eigen::MatrixXd::Identity(n, n);
    decomp.v = cholSolv.solve(preinv);
  } else {
    Rcpp::Rcout<<"No symetric positive matrix, Cholesky decomposition not viable.";
    decomp = RcppbdSVD(mX, int(), int(), false);
  }
  return(decomp);
}







//' Inverse Cholesky of Delayed Array
//' 
//' This function get the inverse of a numerical or Delayed Array matrix. If x is hermitian and positive-definite matrix then 
//' performs get the inverse using Cholesky decomposition
//' 
//' 
//' @param x numerical or Delayed Array matrix. If x is Hermitian and positive-definite performs
//' @return inverse matrix of d 
//' @examples
//' 
//' A <- matrix(c(3,4,3,4,8,6,3,6,9), byrow = TRUE, ncol = 3)
//' bdInvCholesky(A)
//' 
//' # with Delayed Array
//' DA <- DelayedArray(A)
//' bdInvCholesky(DA)
//' 
//' @export
// [[Rcpp::export]]
Eigen::MatrixXd bdInvCholesky (const Rcpp::RObject & x )
{
  
  svdeig result;
  Eigen::MatrixXd X;
  
  if ( x.isS4() == true)    
  {
    X = read_DelayedArray(x);
  } else {
    try{  
      X = Rcpp::as<Eigen::Map<Eigen::MatrixXd> >(x);
    }
    catch(std::exception &ex) { }
  }
  
  result = RcppCholDec(X);
  
  return (result.v);
  
}



//' SVD of DelayedArray 
//' 
//' This function performs a svd decomposition of numerical matrix or Delayed Array
//' 
//' @param x numerical or Delayed Array matrix
//' @param k number of eigen values , this should satisfy k = min(n, m) - 1
//' @param nev (optional, default nev = n-1) Number of eigenvalues requested. This should satisfy 1≤ nev ≤ n, where n is the size of matrix. 
//' @param bcenter (optional, defalut = TRUE) . If center is TRUE then centering is done by subtracting the column means (omitting NAs) of x from their corresponding columns, and if center is FALSE, no centering is done.
//' @param bscale (optional, defalut = TRUE) .  If scale is TRUE then scaling is done by dividing the (centered) columns of x by their standard deviations if center is TRUE, and the root mean square otherwise. If scale is FALSE, no scaling is done.
//' @return u eigenvectors of AA^t, mxn and column orthogonal matrix
//' @return v eigenvectors of A^tA, nxn orthogonal matrix
//' @return d singular values, nxn diagonal matrix (non-negative real values)
//' @examples
//' n <- 500
//' A <- matrix(rnorm(n*n), nrow=n, ncol=n)
//' AD <- DelayedArray(A)
//' 
//' # svd without normalization
//' bdSVD( A, bscale = FALSE, bcenter = FALSE ), # No matrix normalization
//' decsvd$d
//' decsvd$u
//' 
//' # svd with normalization
//' decvsd <- bdSVD( A, bscale = TRUE, bcenter = TRUE), # Matrix normalization
//' 
//' decsvd$d
//' decsvd$u
//' 
//' # svd with scaled matrix (sd)
//' decvsd <- bdSVD( A, bscale = TRUE, bcenter = FALSE), # Scaled matrix
//' 
//' decsvd$d
//' decsvd$u
//' # svd with centered matrix (sd)
//' decvsd <- bdSVD( A, bscale = FALSE, bcenter = TRUE), # Centered matrix
//' decsvd$d
//' decsvd$u
//' 
//' @export
// [[Rcpp::export]]
Rcpp::RObject bdSVD (const Rcpp::RObject & x, Rcpp::Nullable<int> k=0, Rcpp::Nullable<int> nev=0,
                     Rcpp::Nullable<bool> bcenter=true, Rcpp::Nullable<bool> bscale=true)
{
  
  auto dmtype = beachmat::find_sexp_type(x);
  int ks, nvs;
  bool bcent, bscal;
  
  if(k.isNull())  ks = 0 ;
  else    ks = Rcpp::as<int>(k);
  
  if(nev.isNull())  nvs = 0 ;
  else    nvs = Rcpp::as<int>(nev);
  
  
  if(bcenter.isNull())  bcent = true ;
  else    bcent = Rcpp::as<bool>(bcenter);
  
  if(bscale.isNull())  bscal = true ;
  else    bscal = Rcpp::as<bool>(bscale);
  
  // size_t ncols = 0, nrows=0;
  Eigen::MatrixXd X;
  Rcpp::List ret;
  
  if ( dmtype == INTSXP || dmtype==REALSXP ) {
    if ( x.isS4() == true){
      X = read_DelayedArray(x);
    }else {
      try{
        X = Rcpp::as<Eigen::MatrixXd >(x);
      }catch(std::exception &ex) {
        X = Rcpp::as<Eigen::VectorXd >(x);
      }
    }
    
  } else {
    throw std::runtime_error("unacceptable matrix type");
  }
  
  svdeig retsvd;
  retsvd = RcppbdSVD(X,ks,nvs, bcent, bscal);
  
  ret["u"] = retsvd.u;
  ret["v"] = retsvd.v;
  ret["d"] = retsvd.d;
  
  return Rcpp::wrap(ret);
  
}





//' @export
// [[Rcpp::export]]
Rcpp::RObject bdSVD_hdf5 (const Rcpp::RObject & x, Rcpp::Nullable<CharacterVector> group = R_NilValue, 
                                Rcpp::Nullable<CharacterVector> dataset = R_NilValue,
                                Rcpp::Nullable<int> k=2, Rcpp::Nullable<int> q=1, Rcpp::Nullable<int> nev=0,
                                Rcpp::Nullable<bool> bcenter=true, Rcpp::Nullable<bool> bscale=true,
                                Rcpp::Nullable<int> threads = R_NilValue)
{
  
  int ks, qs, nvs;
  bool bcent, bscal;
  CharacterVector strgroup, strdataset;
  std::string filename;
  // int ithreads;
  
  if(k.isNull())  ks = 2 ;
  else    ks = Rcpp::as<int>(k);
  
  if(q.isNull())  qs = 1 ;
  else    qs = Rcpp::as<int>(q);

  if(nev.isNull())  nvs = 0 ;
  else    nvs = Rcpp::as<int>(nev);
  
  
  if(bcenter.isNull())  bcent = true ;
  else    bcent = Rcpp::as<bool>(bcenter);
  
  if(bscale.isNull())  bscal = true ;
  else    bscal = Rcpp::as<bool>(bscale);
  
  if(group.isNull())  strgroup = "" ;
  else    strgroup = Rcpp::as<CharacterVector>(group);
  
  if(dataset.isNull())  strdataset = "";
  else    strdataset = Rcpp::as<CharacterVector>(dataset);
  
  if(is<CharacterVector>(x))
  {
    filename = as<std::string>(x);
  }
  
  // Rcpp::Rcout<<"Abans de cridar el procés del svd... k val : "<<ks<<"\n";
  svdeig retsvd;
  
  retsvd = RcppbdSVD_hdf5( filename, as<std::string>(strgroup), as<std::string>(strdataset), ks, qs, nvs, bcent, bscal );
  
  return List::create(Named("d") = retsvd.d,
                      Named("u") = retsvd.u,
                      Named("v") = retsvd.v);
  
}



//' @export
// [[Rcpp::export]]
Rcpp::RObject bdSVD_lapack ( Rcpp::RObject X, Rcpp::Nullable<bool> bcenter=true, Rcpp::Nullable<bool> bscale=true)
{
  bool bcent, bscal;
  Eigen::MatrixXd eX = as<Eigen::MatrixXd>(X);
  
  if(bcenter.isNull())  bcent = true ;
  else    bcent = Rcpp::as<bool>(bcenter);
  
  if(bscale.isNull())  bscal = true ;
  else    bscal = Rcpp::as<bool>(bscale);
  
  
  svdeig retsvd =  RcppbdSVD_lapack( eX, bcent, bscal);
  
  return List::create(Named("d") = retsvd.d,
                      Named("u") = retsvd.u,
                      Named("v") = retsvd.v);
}




/***R

library(microbenchmark)
library(DelayedArray)
library(BigDataStatMeth)
library(rhdf5)
setwd("~/Library/Mobile Documents/com~apple~CloudDocs/PROJECTES/Treballant/BigDataStatMeth/tmp")

# Proves svd hdf5
 

# if our data is not accessible but presents more rows than columns (more individuals than variables), 
# then, we are going to apply the algorithm to the transpose of the matrices and, therefore,
# we will obtain the right singular vectors instead of the left singular vectors.


# Rows --> Individuals  (small)
# Cols --> Variables (SNP's or ...) (very big)

dades <- BigDataStatMeth::prova_bdSVD_hdf5("tmp_blockmult.hdf5", group = "INPUT", dataset = "A", k=4, q=1) 


fprova <- H5Fopen("tmp_blockmult.hdf5")
fprova
fprova$INPUT$A[1:25,1:25]
csvd <- bdSVD_lapack(fprova$INPUT$A[1:25,1:25], bcenter = FALSE, bscale = FALSE)

csvd3 <- bdSVD_lapack(fprova$INPUT$A[1:25,1:25], bcenter = FALSE, bscale = FALSE)
csvd3$u[1:5,1:5]
csvd3$v
csvd$d
csvd$u
csvd$v

csvd2 <- svd(fprova$INPUT$A[1:25,1:25])
csvd2$v[1:5,1:5]



csvd$u %*% diag(csvd$d)

(csvd$u %*% diag(csvd$d))[1:5,1:5]
t(fprova$tmpgroup$A0)[1:5,1:5]


svd( scale(fprova$INPUT$A))$
bdSVD_lapack(fprova$INPUT$A)$d

h5closeAll()
# 



























# Proves : Matriu Inversa - Cholesky
set.seed(12)
n <- 10
p <- 10
Z <- matrix(rnorm(n*p), nrow=n, ncol=p)
X <- Z
Z[lower.tri(Z)] <- t(Z)[lower.tri(Z)]

# Z <- DelayedArray(Z);Z

A <- matrix(c(5,0,2,5,0,5,-1,-1,2,-1,5,-1,5,-1,-1,5),byrow = TRUE, nrow = 4)

A <- Posdef(n=100, ev=1:100)
AD <- DelayedArray(A)
invCpp <- bdInvCholesky_LDL_eigen(A); invCpp[1:5,1:5]
invR <- solve(A); invR[1:5,1:5]
invP <- inversechol_par(A);invP[1:5,1:5]

stopifnot(all.equal(invCpp,invR ))
stopifnot(all.equal(invCpp,invR ))

invCpp <- bdInvCholesky_LDL_eigen(AD); invCpp[1:5,1:5]
invP <- inversechol_par(AD);invP[1:5,1:5]



invCpp <- bdInvCholesky_LDL(A); invCpp$v[1:4,1:4]
invR <- solve(A); invR[1:4,1:4]
invP <- inversechol(A);invP[1:4,1:4]

stopifnot(all.equal(solve(Z),bdInvCholesky_LDL(Z)$v ))






  results <- microbenchmark(invR <- solve(Z),  # Inversa amb Solve
                            invCpp <- bdInvCholesky_LDL(Z),
                            invCpppar <- inversechol(Z),
                            times = 5L)  # Inversa amb Cholesky
                            
  print(summary(results)[, c(1:7)],digits=3)
  
  all.equal(invR, invCpppar)  
  
  
  cc <- eigen(tcrossprod(X))
  sqrt(cc$values[1:10])
  sqrt(invCpp$d)
  invCpp$v  
  cc$vectors
  
  
  
  A <- matrix(c(5,-3,4,-3,3,-4,4,-4,6), byrow = TRUE, ncol = 3)
  A <- matrix(c(2,-1,0,-1,2,-1,0,-1,1), byrow = TRUE, ncol = 3)
  A <- matrix(c(3,4,3,4,8,6,3,6,9), byrow = TRUE, ncol = 3)
  
  invR <- solve(A) ; invR
  eigen(tcrossprod(A))
  invCpp <- bdInvCholesky(A); invCpp
  
  
  LOOE_BLAST()
  
  n <- 10
  p <- 20
  Z <- matrix(rnorm(n*p), nrow=n, ncol=p)
  Zn <- scale(Z,center = TRUE, scale = TRUE)
  
  library(BigDataStatMeth)
  a <- bdSVD(Z,8,10, TRUE, TRUE)
  b <- eigen(tcrossprod(Z))
  
  svd(Zn)$d
  a$d
  
  svd(Z)$d^2
  a$d^2
  b$values
  
  
  
  a$d
  a$okd
  
  a$d^2
  b$values
  
  ;
  
  n <- 1000
  A <- matrix(rnorm(n*n), nrow=n, ncol=n)
  AD <- DelayedArray(A)
  
  dim(A)
  
  
  res <- microbenchmark( bdsvd <- bdSVD( A, n-1, n, FALSE), # No normalitza la matriu
                         bdsvdD <- bdSVD( AD, n-1, n, FALSE), # No normalitza la matriu
                         sbd <- svd(tcrossprod(A)),
                         times = 5, unit = "s")
  
  print(summary(res)[, c(1:7)],digits=3)
  
  sqrt(sbd$d[1:10])
  bdsvd$d[1:10]
  rsv$d[1:10]
  rsv <- rsvd::rsvd(A)
  
  bdsvd$u[1:5,1:5]
  
  svd(tcrossprod(A))$d[1:10]
  
  A <- matrix(c(5,-3,4,-3,3,-4,4,-4,6,7,2,3), byrow = TRUE, ncol = 3)
  svd(A)$u
  
  bdSVD(A)$u
  
  
*/