/*************************************************************************************
Grid physics library, www.github.com/paboyle/Grid

Minimal Wilson Flow program:
Reads parameters from an XML file using Grid's XmlReader (no external XML library).
*************************************************************************************/

#include <Grid/Grid.h>
#include <fstream>
#include <iomanip>

using namespace Grid;

// Serializable struct for Wilson Flow parameters
struct WilsonFlowPar : Serializable
{
  GRID_SERIALIZABLE_CLASS_MEMBERS(WilsonFlowPar,
                                  int, N,
                                  double, eps,
                                  int, cfg_start,
                                  int, cfg_end,
                                  int, cfg_step,
                                  int, meas_int,
                                  std::string, cfg_prefix);
};

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <parameter file> [Grid options]" << std::endl;
    return EXIT_FAILURE;
  }

  std::string parameterFileName = argv[1];

  // Initialize Grid
  Grid_init(&argc, &argv);
  GridLogLayout();

  // Read XML parameters
  WilsonFlowPar wfPar;
  {
    XmlReader reader(parameterFileName);
    read(reader, "wilsonflow", wfPar);
  }

  // Extract parameters
  int Nstep          = wfPar.N;
  RealD epsilon      = wfPar.eps;
  int ckpoint_start  = wfPar.cfg_start;
  int ckpoint_end    = wfPar.cfg_end;
  int ckpoint_step   = wfPar.cfg_step;
  int meas_int       = wfPar.meas_int;
  std::string prefix = wfPar.cfg_prefix + ".";

  std::cout << GridLogMessage << "Reading Wilson flow parameters:" << std::endl;
  std::cout << GridLogMessage << "  N          = " << Nstep << std::endl;
  std::cout << GridLogMessage << "  epsilon    = " << epsilon << std::endl;
  std::cout << GridLogMessage << "  cfg_start   = " << ckpoint_start << std::endl;
  std::cout << GridLogMessage << "  cfg_end     = " << ckpoint_end << std::endl;
  std::cout << GridLogMessage << "  cfg_step    = " << ckpoint_step << std::endl;
  std::cout << GridLogMessage << "  meas_int   = " << meas_int << std::endl;
  std::cout << GridLogMessage << "  cfg_prefix = " << prefix << std::endl;

  // Build Grid Cartesian geometry
  auto latt_size   = GridDefaultLatt();
  auto simd_layout = GridDefaultSimd(Nd, vComplex::Nsimd());
  auto mpi_layout  = GridDefaultMpi();

  GridCartesian         Grid(latt_size, simd_layout, mpi_layout);
  GridRedBlackCartesian RBGrid(&Grid);

  FieldMetaData header;
  GridCartesian *UGrid = SpaceTimeGrid::makeFourDimGrid(
      GridDefaultLatt(),
      GridDefaultSimd(Nd, vComplex::Nsimd()),
      GridDefaultMpi());

  RealD maxTau = Nstep * epsilon;

  // Gauge fields
  LatticeGaugeField Umu(&Grid), Uflow(&Grid);

  // Loop over configurations
  for (int conf = ckpoint_start; conf <= ckpoint_end; conf += ckpoint_step)
  {
    std::string filename = prefix + std::to_string(conf);
    std::cout << GridLogMessage << "Reading configuration: " << filename << std::endl;

    // Read configuration from file
    NerscIO::readConfiguration(Umu, header, filename);

    std::cout << std::setprecision(15);
    std::cout << GridLogMessage << "Initial plaquette: "
              << WilsonLoops<PeriodicGimplR>::avgPlaquette(Umu) << std::endl;

    // Run Wilson Flow
    WilsonFlow<PeriodicGimplR> WF(epsilon, Nstep, meas_int);
    WF.smear(Uflow, Umu);
  }

  std::cout << GridLogMessage << "Done" << std::endl;

  Grid_finalize();
  return EXIT_SUCCESS;
}

