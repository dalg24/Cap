/* Copyright (c) 2016, the Cap authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#include <cap/energy_storage_device.h>

#include <omp.h>

namespace cap
{

EnergyStorageDeviceInspector::~EnergyStorageDeviceInspector() = default;

EnergyStorageDeviceBuilder::~EnergyStorageDeviceBuilder() = default;

void EnergyStorageDeviceBuilder::register_energy_storage_device(
    std::string const &type, EnergyStorageDeviceBuilder *builder)
{
  EnergyStorageDevice::_builders()[type] = builder;
  std::ignore = omp_get_num_threads();
}

std::map<std::string, EnergyStorageDeviceBuilder *> &
EnergyStorageDevice::_builders()
{
  static std::map<std::string, EnergyStorageDeviceBuilder *> *_builder =
      new std::map<std::string, EnergyStorageDeviceBuilder *>();
  return *_builder;
}

std::unique_ptr<EnergyStorageDevice>
EnergyStorageDevice::build(boost::property_tree::ptree const &ptree,
                           boost::mpi::communicator const &comm)
{
  auto const type = ptree.get<std::string>("type");
  auto const it = _builders().find(type);
  if (it != _builders().end())
    return (it->second)->build(ptree, comm);
  else
    throw std::runtime_error("invalid EnergyStorageDevice type `" + type +
                             "`\n");
}

EnergyStorageDevice::EnergyStorageDevice(
    boost::mpi::communicator const &communicator)
    : _communicator(communicator)
{
}

EnergyStorageDevice::~EnergyStorageDevice() = default;

boost::mpi::communicator EnergyStorageDevice::get_mpi_communicator() const
{
  return _communicator;
}

} // end namespace cap
