/* Copyright (c) 2016, the Cap authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license. 
 */

#include <pycap/energy_storage_device_wrappers.h>
#include <cap/default_inspector.h>
#ifdef WITH_DEAL_II
#include <cap/supercapacitor.h>
#endif
#include <mpi4py/mpi4py.h>

namespace pycap {

double get_current(cap::EnergyStorageDevice const & dev)
{
    double current;
    dev.get_current(current);
    return current;
}

double get_voltage(cap::EnergyStorageDevice const & dev)
{
    double voltage;
    dev.get_voltage(voltage);
    return voltage;
}

boost::python::dict inspect(cap::EnergyStorageDevice & dev,
                            const std::string & type)
{
    // For now there are only two different inspectors so using if ... else ...
    // is fine. However, if we add more inspectors we should use a factory.
    boost::python::dict data;
    if (type.compare("default") == 0)
    {
      cap::DefaultInspector inspector;
      dev.inspect(&inspector);
      for (auto x : inspector.get_data())
        data[x.first] = x.second;
    }
    else if (type.compare("postprocessor") == 0)
    {
      // SuperCapacitorInspector only works if the underlying dev is a
      // SuperCapacitor. SuperCapacitorInspector is templated on the dimension
      // but the EnergyStorageDevice is dimension-independent. So we first try
      // with SuperCapacitorInspector<2> and if the inspect function throws then
      // we try SuperCapacitorInspector<3>.
      bool success = false;
      #ifdef WITH_DEAL_II
      std::vector<std::shared_ptr<cap::EnergyStorageDeviceInspector>> inspectors;
      inspectors.push_back(std::shared_ptr<cap::EnergyStorageDeviceInspector> (
          new cap::SuperCapacitorInspector<2>()));
      inspectors.push_back(std::shared_ptr<cap::EnergyStorageDeviceInspector> (
          new cap::SuperCapacitorInspector<3>()));
      for (auto inspector : inspectors)
      {
        try
        {
          dev.inspect(inspector.get());
          success = true;
          break;
        }
        catch (std::bad_cast const &)
        {
          // Do nothing. If inspector was of dim = 2, we will try dim = 3 next.
          // If dim = 3, we will throw a runtime_error when we leave the loop.
        }
      }
      #endif
      if (!success)
        throw std::runtime_error("The postprocessor inspector can only be used "
            "with a supercapacitor device");
    }
    else
      throw std::runtime_error("Unknown inspector type");

    return data;
}

std::shared_ptr<cap::EnergyStorageDevice>
build_energy_storage_device(boost::python::object & py_ptree,
                            boost::python::object & py_comm)
{
    if (import_mpi4py() < 0) throw std::runtime_error("Failed to import mpi4py");
    boost::property_tree::ptree const & ptree =
        boost::python::extract<boost::property_tree::ptree const &>(py_ptree);
    PyObject* py_obj = py_comm.ptr();
    MPI_Comm *comm_p = PyMPIComm_Get(py_obj);
    if (comm_p == nullptr) boost::python::throw_error_already_set();
    boost::mpi::communicator comm(*comm_p, boost::mpi::comm_attach);
    return cap::EnergyStorageDevice::build(ptree, comm);
}

} // end namespace pycap

