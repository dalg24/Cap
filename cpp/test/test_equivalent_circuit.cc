/* Copyright (c) 2016, the Cap authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#define BOOST_TEST_MODULE EquivalentCircuit

#include "main.cc"

#include <cap/energy_storage_device.h>
#include <cap/equivalent_circuit.h>
#include <boost/property_tree/info_parser.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/range/combine.hpp>
#include <boost/algorithm/cxx11/is_sorted.hpp>

BOOST_AUTO_TEST_CASE(build_equivalent_circuit)
{
  boost::mpi::communicator world;

  // read the property tree for building a supercapacitor`
  boost::property_tree::ptree ptree;
  boost::property_tree::info_parser::read_info("super_capacitor.info", ptree);
  boost::property_tree::ptree geometry_database;
  boost::property_tree::info_parser::read_info("read_mesh.info",
                                               geometry_database);
  ptree.put_child("geometry", geometry_database);

  // this is the new prefered way to make an equivalent circuit
  //
  ptree.put("type", "EquivalentCircuit");
  auto device = cap::EnergyStorageDevice::build(ptree, world);

  // database passed to compute_equivalent_circuit(...) must be empty
  boost::property_tree::ptree not_empty_database;
  not_empty_database.put("something", "not_empty");
  BOOST_CHECK_THROW(cap::compute_equivalent_circuit(ptree, not_empty_database),
                    std::runtime_error);
}

BOOST_DATA_TEST_CASE(test_equivalent_circuit,
                     boost::unit_test::data::make({true, false}),
                     with_faradaic_processes)
{
  boost::mpi::communicator world;

  // read the property tree for building the supercapacitor
  boost::property_tree::ptree super_capacitor_database;
  boost::property_tree::info_parser::read_info("super_capacitor.info",
                                               super_capacitor_database);
  boost::property_tree::ptree geometry_database;
  boost::property_tree::info_parser::read_info("read_mesh.info",
                                               geometry_database);
  super_capacitor_database.put_child("geometry", geometry_database);

  if (!with_faradaic_processes)
    super_capacitor_database.put("material_properties.electrode_material"
                                 ".exchange_current_density",
                                 0.0);

  // get the property tree to build the equivalent circuit
  boost::property_tree::ptree equivalent_circuit_database;
  cap::compute_equivalent_circuit(super_capacitor_database,
                                  equivalent_circuit_database);

  if (with_faradaic_processes)
    BOOST_TEST(equivalent_circuit_database.get<std::string>("type")
                   .compare("ParallelRC") == 0);
  else
    BOOST_TEST(equivalent_circuit_database.get<std::string>("type")
                   .compare("SeriesRC") == 0);

  std::shared_ptr<cap::EnergyStorageDevice> super_capacitor =
      cap::EnergyStorageDevice::build(super_capacitor_database, world);
  std::shared_ptr<cap::EnergyStorageDevice> equivalent_circuit =
      cap::EnergyStorageDevice::build(equivalent_circuit_database, world);

  double time_step = 0.1;                // [second]
  double const maximum_duration = 15.0;  // [second]
  double const charge_current = 10.0e-3; // [ampere]
  double const charge_stop_at = 2.0;     // [volt]
  auto charge_done =
      [charge_stop_at](std::shared_ptr<cap::EnergyStorageDevice const> dev)
  {
    double voltage;
    dev->get_voltage(voltage);
    return voltage >= charge_stop_at;
  };
  auto report = [](std::shared_ptr<cap::EnergyStorageDevice const> dev,
                   std::vector<double> &data)
  {
    double voltage;
    dev->get_voltage(voltage);
    data.push_back(voltage);
  };

  std::map<std::string, std::shared_ptr<cap::EnergyStorageDevice>> device;
  device.emplace("super_capacitor", cap::EnergyStorageDevice::build(
                                        super_capacitor_database, world));
  device.emplace("equivalent_circuit", cap::EnergyStorageDevice::build(
                                           equivalent_circuit_database, world));

  std::map<std::string, std::vector<double>> data;
  data.emplace("super_capacitor", std::vector<double>());
  data.emplace("equivalent_circuit", std::vector<double>());

  double time = 0.0;
  while (time < maximum_duration)
  {
    time += time_step;
    for (auto type : {"super_capacitor", "equivalent_circuit"})
    {
      device[type]->evolve_one_time_step_constant_current(time_step,
                                                          charge_current);
      report(device[type], data[type]);
    }
    if (charge_done(device["super_capacitor"]) &&
        charge_done(device["equivalent_circuit"]))
      break;
  }

  auto absolute_difference = [](boost::tuple<double, double> const &a)
  {
    double a1, a2;
    boost::tie(a1, a2) = a;
    return std::abs(a1 - a2);
  };

  // a = (a1, a2) and b = (b1, b2)
  // check that |a1 - a2| >= |b1 - b2|
  auto compare = [absolute_difference](
      boost::tuple<double, double> const &a,
      boost::tuple<double, double> const &b) -> bool
  {
    return absolute_difference(a) >= absolute_difference(b);
  };

  // check that the voltage difference between the two model is a decreasing
  // sequence
  BOOST_TEST(boost::algorithm::is_sorted(
      boost::combine(data["super_capacitor"], data["equivalent_circuit"]),
      compare));

  // check that the voltage is the same asymptotically (say one percent)
  BOOST_TEST(data["super_capacitor"].back() ==
                 data["equivalent_circuit"].back(),
             1.0 % boost::test_tools::tolerance());
}
