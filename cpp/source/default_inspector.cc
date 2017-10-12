/* Copyright (c) 2016, the Cap authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#include <cap/default_inspector.h>
#ifdef WITH_DEAL_II
#include <cap/supercapacitor.h>
#endif
#include <cap/utils.h> // to_vector<>

namespace cap
{

DefaultInspector::DefaultInspector() : _data(std::map<std::string, double>()) {}

template <int dim>
std::map<std::string, double>
extract_data_from_super_capacitor(EnergyStorageDevice *device)
{
  std::map<std::string, double> data;
  #ifdef WITH_DEAL_II
  if (auto super_capacitor = dynamic_cast<SuperCapacitor<dim> *>(device))
  {
    // get some values from the post processor
    auto post_processor = super_capacitor->get_post_processor();
    BOOST_ASSERT_MSG(post_processor != nullptr,
                     "The Postprocessor does not exist.");
    double value;
    for (std::string const &key :
         {"anode_electrode_interfacial_surface_area",
          "anode_electrode_mass_of_active_material",
          "cathode_electrode_interfacial_surface_area",
          "cathode_electrode_mass_of_active_material", "n_dofs"})
    {
      post_processor->get(key, value);
      data[key] = value;
    }

    // get other values from the property tree
    boost::property_tree::ptree const *ptree =
        super_capacitor->get_property_tree();
    data["geometric_area"] = ptree->get<double>("geometry.geometric_area");
    data["anode_electrode_thickness"] =
        ptree->get<double>("geometry.anode_electrode_thickness");
    data["cathode_electrode_thickness"] =
        ptree->get<double>("geometry.cathode_electrode_thickness");
    // NOTE: by default let us assume that the electrode materials are called
    // "anode" and "cathode" but allow for the user to change it.
    // We might want to make a SuperCapacitor specific class derived from
    // Geometry that would be aware of the material tags assigned to the
    // electrode/separator/collector regions. In that case we would access the
    // SuperCapacitor::_geometry here and pull this information.
    // Nevertheless, this is needed for debug purposes and I am not going to
    // bother add a test yet. As a reminder there will be this note plus a line
    // with no coverage...
    std::vector<std::string> electrode_material_names = {"anode", "cathode"};
    if (auto names = ptree->get_optional<std::string>("electrodes"))
      electrode_material_names = to_vector<std::string>(names.get());
    for (std::string const &electrode : electrode_material_names)
    {
      std::string tmp = ptree->get<std::string>("material_properties." +
                                                electrode + ".matrix_phase");
      data[electrode + "_electrode_double_layer_capacitance"] =
          ptree->get<double>("material_properties." + tmp +
                             ".differential_capacitance");
    }
  }
  else
  {
    throw std::runtime_error("Downcasting failed");
  }
  #endif
  return data;
}

void DefaultInspector::inspect(EnergyStorageDevice *device)
{
  #ifdef WITH_DEAL_II
  if (dynamic_cast<SuperCapacitor<2> *>(device))
  {
    _data = extract_data_from_super_capacitor<2>(device);
  }
  else if (dynamic_cast<SuperCapacitor<3> *>(device))
  {
    _data = extract_data_from_super_capacitor<3>(device);
  }
  else
  {
    // do nothing
  }
  #endif
}

std::map<std::string, double> DefaultInspector::get_data() const
{
  return _data;
}

} // end namespace cap
