/* 
 * File:   esolver.h
 * Author: wsttiger
 *
 * Created on April 17, 2009, 12:12 PM
 */

#ifndef _ESOLVER_H

typedef SharedPtr< WorldDCPmapInterface< Key<3> > > pmapT;
typedef Vector<double,3> coordT;
typedef SharedPtr< FunctionFunctorInterface<std::complex<double>,3> > functorT;
typedef SharedPtr< FunctionFunctorInterface<double,3> > rfunctorT;
typedef Function<std::complex<double>,3> functionT;
typedef Function<std::complex<double>,3> cfunctionT;
typedef Function<double,3> rfunctionT;
typedef vector<functionT> vecfuncT;
typedef vector<rfunctionT> rvecfuncT;
typedef vector<cfunctionT> cvecfuncT;
typedef Tensor< std::complex<double> > ctensorT;
typedef Tensor<double> rtensorT;
typedef FunctionFactory<std::complex<double>,3> factoryT;
typedef FunctionFactory<double,3> rfactoryT;
typedef SeparatedConvolution<double,3> operatorT;
typedef SharedPtr<operatorT> poperatorT;

void print_cube(World& world, const Function<double,3>& f, int npts)
{
  f.reconstruct();
  if (world.rank() == 0)  printf("\n");
  Tensor<double> csize = FunctionDefaults<3>::get_cell_width();

  for (int i = 0; i < npts; i++)
  {
    for (int j = 0; j < npts; j++)
    {
      for (int k = 0; k < npts; k++)
      {
        double x = (i+0.5) * (csize[0]/npts) - csize[0]/2;
        double y = (j+0.5) * (csize[1]/npts) - csize[1]/2;
        double z = (k+0.5) * (csize[2]/npts) - csize[2]/2;
        coordT p(0.0);
        p[0] = x; p[1] = y; p[2] = z;
        if (world.rank() == 0)
          printf("%10.2f%10.2f%10.2f%15.8f\n", x, y, z, f(p));
      }
    }
  }
}

void print_cube(World& world, const Function<double,3>& f1, const Function<double,3>& f2, int npts)
{
  f1.reconstruct();
  f2.reconstruct();
  if (world.rank() == 0)  printf("\n");
  Tensor<double> csize = FunctionDefaults<3>::get_cell_width();

  for (int i = 0; i < npts; i++)
  {
    for (int j = 0; j < npts; j++)
    {
      for (int k = 0; k < npts; k++)
      {
        double x = (i+0.5) * (csize[0]/npts) - csize[0]/2;
        double y = (j+0.5) * (csize[1]/npts) - csize[1]/2;
        double z = (k+0.5) * (csize[2]/npts) - csize[2]/2;
        coordT p(0.0);
        p[0] = x; p[1] = y; p[2] = z;
        if (world.rank() == 0)
          printf("%10.2f%10.2f%10.2f%15.8f%15.8f\n", x, y, z, f1(p), f2(p));
      }
    }
  }
}

void print_cube(World& world, const Function<double,3>& f1, const Function<double,3>& f2,
    const Function<double,3>& f3, int npts)
{
  f1.reconstruct();
  f2.reconstruct();
  f3.reconstruct();
  if (world.rank() == 0)  printf("\n");
  Tensor<double> csize = FunctionDefaults<3>::get_cell_width();

  for (int i = 0; i < npts; i++)
  {
    for (int j = 0; j < npts; j++)
    {
      for (int k = 0; k < npts; k++)
      {
        double x = (i+0.5) * (csize[0]/npts) - csize[0]/2;
        double y = (j+0.5) * (csize[1]/npts) - csize[1]/2;
        double z = (k+0.5) * (csize[2]/npts) - csize[2]/2;
        coordT p(0.0);
        p[0] = x; p[1] = y; p[2] = z;
        if (world.rank() == 0)
          printf("%10.2f%10.2f%10.2f%15.8f%15.8f%15.8f\n", x, y, z, f1(p), f2(p), f3(p));
      }
    }
  }
}

struct KPoint
{
  coordT k;
  double weight;
  unsigned int begin;
  unsigned int end;

  KPoint()
  {
    k[0] = 0.0; k[1] = 0.0; k[2] = 0.0;
    weight = 0.0;
    begin = -1;
    end = -1;
  }

  KPoint(const coordT& k, const double& weight, const int& begin,
         const int& end)
   : k(k), weight(weight), begin(begin), end(end) {}

  KPoint(const coordT& k, const double& weight)
   : k(k), weight(weight), begin(-1), end(-1) {}

  template <typename Archive>
  void serialize(Archive& ar) {
      ar & k & weight & begin & end;
  }

};

std::istream& operator >> (std::istream& is, KPoint& kpt)
{
  for (int i = 0; i < kpt.k.size(); i++)
    is >> kpt.k[i];
  is >> kpt.weight;
  is >> kpt.begin;
  is >> kpt.end;

  return is;
}

  //***************************************************************************
  template <typename Q, int NDIM>
  Function<Q,NDIM> pdiff(const Function<Q,NDIM>& f, int axis, bool fence = true)
  {
    Function<Q,NDIM>& g = const_cast< Function<Q,NDIM>& >(f);
    // Check for periodic boundary conditions
    Tensor<int> oldbc = g.get_bc();
    Tensor<int> bc(NDIM,2);
    bc(___) = 1;
    g.set_bc(bc);
    // Do calculation
    Function<Q,NDIM> rf = diff(g,axis,fence);
    // Restore previous boundary conditions
    g.set_bc(oldbc);
    return rf;
  }
  //***************************************************************************

  //***************************************************************************
  template <typename Q, int NDIM>
  ctensorT kinetic_energy_matrix(World& world,
                                 const std::vector< Function<std::complex<Q>,NDIM> >& v,
                                 const bool periodic,
                                 const KPoint k = KPoint(coordT(0.0), 0.0))
  {
    reconstruct(world, v);
    int n = v.size();
    ctensorT c(n, n);
    const std::complex<double> I = std::complex<double>(0.0, 1.0);
    double k0 = k.k[0];
    double k1 = k.k[1];
    double k2 = k.k[2];
    if (periodic)
    {
      for (int i = 0; i < n; i++)
      {
        functionT dv_i_0 = pdiff(v[i], 0) - I * k0 * v[i];
        functionT dv_i_1 = pdiff(v[i], 1) - I * k1 * v[i];
        functionT dv_i_2 = pdiff(v[i], 2) - I * k2 * v[i];
        for (int j = 0; j <= i; j++)
        {
          functionT dv_j_0 = pdiff(v[j], 0) + I * k0 * v[j];
          functionT dv_j_1 = pdiff(v[j], 1) + I * k1 * v[j];
          functionT dv_j_2 = pdiff(v[j], 2) + I * k2 * v[j];
          c(i, j) = inner(dv_i_0, dv_j_0) + inner(dv_i_1, dv_j_1) + inner(dv_i_2, dv_j_2);
          c(j, i) = conj(c(i, j));
        }
      }
    }
    else
    {
      for (int axis = 0; axis < 3; axis++)
      {
        std::vector< Function<std::complex<Q>,NDIM> > dv = diff(world, v, axis);
        c += matrix_inner(world, dv, dv, true);
        dv.clear(); // Allow function memory to be freed
      }
    }
    return c.scale(0.5);
  }
  //***************************************************************************

  //***************************************************************************
  template <typename Q, int NDIM>
  ctensorT kinetic_energy_matrix(World& world,
                                 const std::vector< Function<Q,NDIM> >& v,
                                 const bool periodic,
                                 const KPoint k = KPoint(coordT(0.0), 0.0))
  {
    reconstruct(world, v);
    int n = v.size();
    ctensorT c(n, n);
    const std::complex<double> I = std::complex<double>(0.0, 1.0);
    double k0 = k.k[0];
    double k1 = k.k[1];
    double k2 = k.k[2];
    if (periodic)
    {
      for (int i = 0; i < n; i++)
      {
        functionT dv_i_0 = function_real2complex(pdiff(v[i], 0)) - I * k0 * v[i];
        functionT dv_i_1 = function_real2complex(pdiff(v[i], 1)) - I * k1 * v[i];
        functionT dv_i_2 = function_real2complex(pdiff(v[i], 2)) - I * k2 * v[i];
        for (int j = 0; j <= i; j++)
        {
          functionT dv_j_0 = function_real2complex(pdiff(v[j], 0)) + I * k0 * v[j];
          functionT dv_j_1 = function_real2complex(pdiff(v[j], 1)) + I * k1 * v[j];
          functionT dv_j_2 = function_real2complex(pdiff(v[j], 2)) + I * k2 * v[j];
          c(i, j) = inner(dv_i_0, dv_j_0) + inner(dv_i_1, dv_j_1) + inner(dv_i_2, dv_j_2);
          c(j, i) = conj(c(i, j));
        }
      }
    }
    else
    {
      rtensorT r(n, n);
      for (int axis = 0; axis < 3; axis++)
      {
        rvecfuncT dv = diff(world, v, axis);
        r += matrix_inner(world, dv, dv, true);
        dv.clear(); // Allow function memory to be freed
      }
      c = ctensorT(r);
    }
    return c.scale(0.5);
  }
  //***************************************************************************

#define	_ESOLVER_H



#endif	/* _ESOLVER_H */

