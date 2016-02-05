#include <cap/electrochemical_operator.h>
#include <cap/dof_extractor.h>
#include <deal.II/base/function.h>
#include <deal.II/numerics/vector_tools.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/dofs/dof_tools.h>
#include <deal.II/fe/fe_values.h>
#include <deal.II/lac/block_vector.h>
#include <numeric>

namespace cap
{

template <int dim>
ElectrochemicalOperator<dim>::ElectrochemicalOperator(
    std::shared_ptr<OperatorParameters<dim> const> parameters)
    : Operator<dim>(parameters)
{
  std::shared_ptr<boost::property_tree::ptree const> database =
      parameters->database;

  // clang-format off
  this->solid_potential_component  = database->get<unsigned int>("solid_potential_component");
  this->liquid_potential_component = database->get<unsigned int>("liquid_potential_component");
  this->temperature_component      = database->get<unsigned int>("temperature_component");
  // clang-format on

  dealii::DoFHandler<dim> const &dof_handler = *(this->dof_handler);
  unsigned int const n_components = dealii::DoFTools::n_components(dof_handler);
  std::vector<dealii::types::global_dof_index> dofs_per_component(n_components);
  dealii::DoFTools::count_dofs_per_component(dof_handler, dofs_per_component);
  this->dof_shift = std::accumulate(
      &(dofs_per_component[0]),
      &(dofs_per_component[std::min(this->solid_potential_component,
                                    this->liquid_potential_component)]),
      0);

  // clang-format off
  this->alpha               = database->get<double>("material_properties.alpha", 0.0); // going to regret this!

  this->anode_boundary_id   = database->get<dealii::types::boundary_id>("boundary_values.anode_boundary_id");
  this->cathode_boundary_id = database->get<dealii::types::boundary_id>("boundary_values.cathode_boundary_id");
  // clang-format on

  std::shared_ptr<
      ElectrochemicalOperatorParameters<dim> const> electrochemical_parameters =
      std::dynamic_pointer_cast<ElectrochemicalOperatorParameters<dim> const>(
          parameters);
}

template <int dim>
void ElectrochemicalOperator<dim>::reset(
    std::shared_ptr<OperatorParameters<dim> const> parameters)
{
  // std::cout<<"### reset electrical ###\n";
  std::shared_ptr<
      ElectrochemicalOperatorParameters<dim> const> electrochemical_parameters =
      std::dynamic_pointer_cast<ElectrochemicalOperatorParameters<dim> const>(
          parameters);

  this->capacitor_state = electrochemical_parameters->capacitor_state;

  this->stiffness_matrix = 0.0;
  this->mass_matrix      = 0.0;
  this->load_vector = 0.0;
  this->boundary_values.clear();

  this->compute_electrical_operator_contribution();

  if (this->capacitor_state == GalvanostaticCharge)
  {
    //        this->compute_neumann_boundary_contribution(this->charge_current_density);
    throw std::runtime_error("deprecated stuff");
  }
  else if (this->capacitor_state == GalvanostaticDischarge)
  {
    //        this->compute_neumann_boundary_contribution(this->discharge_current_density);
    throw std::runtime_error("deprecated stuff");
  }
  else if (this->capacitor_state == Relaxation)
  {
    //        this->compute_neumann_boundary_contribution(0.0);
    throw std::runtime_error("deprecated stuff");
  }
  else if (this->capacitor_state == PotentiostaticCharge)
  {
    //        this->compute_dirichlet_boundary_values(this->charge_potential);
    throw std::runtime_error("deprecated stuff");
  }
  else if (this->capacitor_state == PotentiostaticDischarge)
  {
    //        this->compute_dirichlet_boundary_values(this->discharge_potential);
    throw std::runtime_error("deprecated stuff");
  }
  else if (this->capacitor_state == Initialize)
  {
    //        this->compute_dirichlet_boundary_values(this->initial_potential);
    throw std::runtime_error("deprecated stuff");
  }
  else if (this->capacitor_state == CustomConstantCurrent)
  {
    this->compute_neumann_boundary_contribution(
        electrochemical_parameters->custom_constant_current_density);
  }
  else if (this->capacitor_state == CustomConstantVoltage)
  {
    this->compute_dirichlet_boundary_values(
        electrochemical_parameters->custom_constant_voltage);
  }
  else if (this->capacitor_state == CustomConstantLoad)
  {
    this->compute_robin_boundary_contribution(
        electrochemical_parameters->custom_constant_load_density);
  }
  else
  {
    throw std::runtime_error("State of the capacitor undetermined");
  } // end if

  // std::cout<<std::setprecision(15)
  //    <<"stiffness="<<this->stiffness_matrix.l1_norm()<<"  "
  //    <<"mass="<<this->mass_matrix.l1_norm()<<"  "
  //    <<"load="<<this->load_vector.l2_norm()<<"\n";
}

template <int dim>
void ElectrochemicalOperator<dim>::compute_dirichlet_boundary_values(
    double const potential)
{
  dealii::DoFHandler<dim> const &dof_handler = *(this->dof_handler);
  unsigned int const n_components = dealii::DoFTools::n_components(dof_handler);
  std::vector<bool> mask(n_components, false);
  mask[this->solid_potential_component] = true;
  dealii::ComponentMask component_mask(mask);
  typename dealii::FunctionMap<dim>::type dirichlet_boundary_condition;
  std::vector<dealii::std_cxx11::shared_ptr<dealii::Function<dim>>>
      dirichlet_functions;
  dirichlet_functions.push_back(
      dealii::std_cxx11::shared_ptr<dealii::Function<dim>>(
          new dealii::ConstantFunction<dim>(0.0, n_components)));
  dirichlet_functions.push_back(
      dealii::std_cxx11::shared_ptr<dealii::Function<dim>>(
          new dealii::ConstantFunction<dim>(potential, n_components)));
  dirichlet_boundary_condition[this->anode_boundary_id] =
      dirichlet_functions[0].get();
  dirichlet_boundary_condition[this->cathode_boundary_id] =
      dirichlet_functions[1].get();
  dealii::VectorTools::interpolate_boundary_values(
      dof_handler, dirichlet_boundary_condition, this->boundary_values,
      component_mask);
  dealii::types::global_dof_index const dof_shift = this->dof_shift;
  if (dof_shift != 0)
  {
    std::map<dealii::types::global_dof_index, double> tmp_boundary_values;
    std::for_each(
        (this->boundary_values).begin(), (this->boundary_values).end(),
        [&tmp_boundary_values, &dof_shift](
            std::pair<dealii::types::global_dof_index, double> const &p)
        {
          tmp_boundary_values[p.first - dof_shift] = p.second;
        });
    (this->boundary_values).swap(tmp_boundary_values);
  }
}

template <int dim>
void ElectrochemicalOperator<dim>::compute_robin_boundary_contribution(
    double const load_density)
{
  dealii::DoFHandler<dim> const &dof_handler = *(this->dof_handler);
  dealii::ConstraintMatrix const &constraint_matrix =
      *(this->constraint_matrix);
  {
    unsigned int const n_components =
        dealii::DoFTools::n_components(dof_handler);
    std::vector<bool> mask(n_components, false);
    mask[this->solid_potential_component] = true;
    dealii::ComponentMask component_mask(mask);
    dealii::VectorTools::interpolate_boundary_values(
        dof_handler, this->anode_boundary_id,
        dealii::ConstantFunction<dim>(0.0, n_components), this->boundary_values,
        component_mask);
    dealii::types::global_dof_index const dof_shift = this->dof_shift;
    if (dof_shift != 0)
    {
      std::map<dealii::types::global_dof_index, double> tmp_boundary_values;
      std::for_each(
          (this->boundary_values).begin(), (this->boundary_values).end(),
          [&tmp_boundary_values, &dof_shift](
              std::pair<dealii::types::global_dof_index, double> const &p)
          {
            tmp_boundary_values[p.first - dof_shift] = p.second;
          });
      (this->boundary_values).swap(tmp_boundary_values);
    }
  }

  dealii::FEValuesExtractors::Scalar const solid_potential(
      this->solid_potential_component);
  dealii::FiniteElement<dim> const &fe = dof_handler.get_fe();
  dealii::QGauss<dim - 1> face_quadrature_rule(
      fe.degree + 1); // TODO: maybe use fe extractor
  dealii::FEFaceValues<dim> fe_face_values(fe, face_quadrature_rule,
                                           dealii::update_values |
                                               dealii::update_JxW_values);
  unsigned int const dofs_per_cell   = fe.dofs_per_cell;
  unsigned int const n_face_q_points = face_quadrature_rule.size();
  dealii::FullMatrix<double> cell_stiffness_matrix(dofs_per_cell,
                                                   dofs_per_cell);
  std::vector<double> load_electrical_conductance_values(n_face_q_points);
  std::vector<dealii::types::global_dof_index> local_dof_indices(dofs_per_cell);
  unsigned int const n_components = dealii::DoFTools::n_components(dof_handler);
  dealii::ComponentMask mask(n_components, false);
  mask.set(this->solid_potential_component, true);
  mask.set(this->liquid_potential_component, true);
  DoFExtractor dof_extractor(mask, mask, dofs_per_cell);
  typename dealii::DoFHandler<dim>::active_cell_iterator
      cell     = dof_handler.begin_active(),
      end_cell = dof_handler.end();
  for (; cell != end_cell; ++cell)
  {
    cell_stiffness_matrix = 0.0;
    if (cell->at_boundary())
    {
      for (unsigned int face = 0;
           face < dealii::GeometryInfo<dim>::faces_per_cell; ++face)
      {
        if (cell->face(face)->at_boundary())
        {
          fe_face_values.reinit(cell, face);
          std::fill(
              load_electrical_conductance_values.begin(),
              load_electrical_conductance_values.end(),
              ((cell->face(face)->boundary_id() == this->cathode_boundary_id)
                   ? 1.0 / load_density
                   : 0.0));
          for (unsigned int q_point = 0; q_point < n_face_q_points; ++q_point)
          {
            for (unsigned int i = 0; i < dofs_per_cell; ++i)
            {
              for (unsigned int j = 0; j < dofs_per_cell; ++j)
              {
                cell_stiffness_matrix(i, j) +=
                    load_electrical_conductance_values[q_point] *
                    fe_face_values[solid_potential].value(i, q_point) *
                    fe_face_values[solid_potential].value(j, q_point) *
                    fe_face_values.JxW(q_point);
              } // end for j
            }   // end for i
          }     // end for face quadrature point
        }       // end if face at boundary
      }         // end for face
    }           // end if cell at boundary
    cell->get_dof_indices(local_dof_indices);
    std::vector<dealii::types::global_dof_index> tmp_indices =
        dof_extractor.extract_row_indices(local_dof_indices);
    dealii::FullMatrix<double> tmp_stiffness_matrix =
        dof_extractor.extract_matrix(cell_stiffness_matrix);
    std::transform(tmp_indices.begin(), tmp_indices.end(), tmp_indices.begin(),
                   std::bind2nd(std::minus<dealii::types::global_dof_index>(),
                                this->dof_shift));
    constraint_matrix.distribute_local_to_global(
        tmp_stiffness_matrix, tmp_indices, this->stiffness_matrix);
  } // end for cell
}

template <int dim>
void ElectrochemicalOperator<dim>::compute_neumann_boundary_contribution(
    double const current_density)
{
  dealii::DoFHandler<dim> const &dof_handler = *(this->dof_handler);
  dealii::ConstraintMatrix const &constraint_matrix =
      *(this->constraint_matrix);
  {
    unsigned int const n_components =
        dealii::DoFTools::n_components(dof_handler);
    std::vector<bool> mask(n_components, false);
    mask[this->solid_potential_component] = true;
    dealii::ComponentMask component_mask(mask);
    dealii::VectorTools::interpolate_boundary_values(
        dof_handler, this->anode_boundary_id,
        dealii::ConstantFunction<dim>(0.0, n_components), this->boundary_values,
        component_mask);
    dealii::types::global_dof_index const dof_shift = this->dof_shift;
    if (dof_shift != 0)
    {
      std::map<dealii::types::global_dof_index, double> tmp_boundary_values;
      std::for_each(
          (this->boundary_values).begin(), (this->boundary_values).end(),
          [&tmp_boundary_values, &dof_shift](
              std::pair<dealii::types::global_dof_index, double> const &p)
          {
            tmp_boundary_values[p.first - dof_shift] = p.second;
          });
      (this->boundary_values).swap(tmp_boundary_values);
    }
  }

  dealii::FEValuesExtractors::Scalar const solid_potential(
      this->solid_potential_component);
  dealii::FiniteElement<dim> const &fe = dof_handler.get_fe();
  dealii::QGauss<dim - 1> face_quadrature_rule(
      fe.degree + 1); // TODO: maybe use fe extractor
  dealii::FEFaceValues<dim> fe_face_values(fe, face_quadrature_rule,
                                           dealii::update_values |
                                               dealii::update_JxW_values);
  unsigned int const dofs_per_cell   = fe.dofs_per_cell;
  unsigned int const n_face_q_points = face_quadrature_rule.size();
  dealii::Vector<double> cell_load_vector(dofs_per_cell);
  std::vector<double> current_density_values(n_face_q_points);
  std::vector<dealii::types::global_dof_index> local_dof_indices(dofs_per_cell);
  unsigned int const n_components = dealii::DoFTools::n_components(dof_handler);
  dealii::ComponentMask mask(n_components, false);
  mask.set(this->solid_potential_component, true);
  mask.set(this->liquid_potential_component, true);
  DoFExtractor dof_extractor(mask, mask, dofs_per_cell);
  typename dealii::DoFHandler<dim>::active_cell_iterator
      cell     = dof_handler.begin_active(),
      end_cell = dof_handler.end();
  for (; cell != end_cell; ++cell)
  {
    cell_load_vector = 0.0;
    if (cell->at_boundary())
    {
      for (unsigned int face = 0;
           face < dealii::GeometryInfo<dim>::faces_per_cell; ++face)
      {
        if (cell->face(face)->at_boundary())
        {
          fe_face_values.reinit(cell, face);
          std::fill(
              current_density_values.begin(), current_density_values.end(),
              ((cell->face(face)->boundary_id() == this->cathode_boundary_id)
                   ? current_density
                   : 0.0));
          for (unsigned int q_point = 0; q_point < n_face_q_points; ++q_point)
          {
            for (unsigned int i = 0; i < dofs_per_cell; ++i)
            {
              cell_load_vector(i) +=
                  (current_density_values[q_point] *
                   fe_face_values[solid_potential].value(i, q_point)) *
                  fe_face_values.JxW(q_point);
            } // end for i
          }   // end for face quadrature point
        }     // end if face at boundary
      }       // end for face
    }         // end if cell at boundary
    cell->get_dof_indices(local_dof_indices);
    std::vector<dealii::types::global_dof_index> tmp_indices =
        dof_extractor.extract_row_indices(local_dof_indices);
    dealii::Vector<double> tmp_load_vector =
        dof_extractor.extract_vector(cell_load_vector);
    std::transform(tmp_indices.begin(), tmp_indices.end(), tmp_indices.begin(),
                   std::bind2nd(std::minus<dealii::types::global_dof_index>(),
                                this->dof_shift));
    constraint_matrix.distribute_local_to_global(tmp_load_vector, tmp_indices,
                                                 this->load_vector);
    //        this->constraint_matrix.distribute_local_to_global(cell_load_vector,
    //        local_dof_indices, this->load_vector);
  } // end for cell
}

template <int dim>
void ElectrochemicalOperator<dim>::compute_electrical_operator_contribution()
{
  // clang-format off
  dealii::DoFHandler<dim> const &dof_handler        = *(this->dof_handler);
  dealii::ConstraintMatrix const &constraint_matrix = *(this->constraint_matrix);
  dealii::FiniteElement<dim> const &fe              = dof_handler.get_fe();
  // clang-format on
  dealii::FEValuesExtractors::Scalar const solid_potential(
      this->solid_potential_component);
  dealii::FEValuesExtractors::Scalar const liquid_potential(
      this->liquid_potential_component);
  dealii::QGauss<dim> quadrature_rule(fe.degree +
                                      1); // TODO: map to component...
  dealii::FEValues<dim> fe_values(
      fe, quadrature_rule, dealii::update_values | dealii::update_gradients |
                               dealii::update_JxW_values);
  unsigned int const dofs_per_cell = fe.dofs_per_cell;
  unsigned int const n_q_points = quadrature_rule.size();
  dealii::FullMatrix<double> cell_stiffness_matrix(dofs_per_cell,
                                                   dofs_per_cell);
  dealii::FullMatrix<double> cell_mass_matrix(dofs_per_cell, dofs_per_cell);
  std::vector<double> solid_phase_diffusion_coefficient_values(n_q_points);
  std::vector<double> liquid_phase_diffusion_coefficient_values(n_q_points);
  std::vector<double> specific_capacitance_values(n_q_points);
  std::vector<double> faradaic_reaction_coefficient_values(n_q_points);
  std::vector<dealii::types::global_dof_index> local_dof_indices(dofs_per_cell);
  unsigned int const n_components = dealii::DoFTools::n_components(dof_handler);
  dealii::ComponentMask mask(n_components, false);
  mask.set(this->solid_potential_component, true);
  mask.set(this->liquid_potential_component, true);
  DoFExtractor dof_extractor(mask, mask, dofs_per_cell);
  typename dealii::DoFHandler<dim>::active_cell_iterator
      cell     = dof_handler.begin_active(),
      end_cell = dof_handler.end();
  for (; cell != end_cell; ++cell)
  {
    cell_stiffness_matrix = 0.0;
    cell_mass_matrix = 0.0;
    fe_values.reinit(cell);
    // clang-format off
    (this->mp_values)->get_values("specific_capacitance", cell, specific_capacitance_values);
    (this->mp_values)->get_values("solid_electrical_conductivity", cell, solid_phase_diffusion_coefficient_values);
    (this->mp_values)->get_values("liquid_electrical_conductivity", cell, liquid_phase_diffusion_coefficient_values);
    (this->mp_values)->get_values("faradaic_reaction_coefficient", cell, faradaic_reaction_coefficient_values);
    // clang-format on
    for (unsigned int q_point = 0; q_point < n_q_points; ++q_point)
    {
      for (unsigned int i = 0; i < dofs_per_cell; ++i)
      {
        for (unsigned int j = 0; j < dofs_per_cell; ++j)
        {
          cell_stiffness_matrix(i, j) +=
              (+solid_phase_diffusion_coefficient_values[q_point] *
                   (fe_values[solid_potential].gradient(i, q_point) *
                    fe_values[solid_potential].gradient(j, q_point)) +
               liquid_phase_diffusion_coefficient_values[q_point] *
                   (fe_values[liquid_potential].gradient(i, q_point) *
                    fe_values[liquid_potential].gradient(j, q_point)) +
               faradaic_reaction_coefficient_values[q_point] *
                   (fe_values[solid_potential].value(i, q_point) *
                    fe_values[solid_potential].value(j, q_point)) -
               faradaic_reaction_coefficient_values[q_point] *
                   (fe_values[liquid_potential].value(i, q_point) *
                    fe_values[solid_potential].value(j, q_point)) -
               faradaic_reaction_coefficient_values[q_point] *
                   (fe_values[solid_potential].value(i, q_point) *
                    fe_values[liquid_potential].value(j, q_point)) +
               faradaic_reaction_coefficient_values[q_point] *
                   (fe_values[liquid_potential].value(i, q_point) *
                    fe_values[liquid_potential].value(j, q_point))) *
              fe_values.JxW(q_point);
          cell_mass_matrix(i, j) +=
              (+specific_capacitance_values[q_point] *
                   (fe_values[solid_potential].value(i, q_point) *
                    fe_values[solid_potential].value(j, q_point)) -
               specific_capacitance_values[q_point] *
                   (fe_values[solid_potential].value(i, q_point) *
                    fe_values[liquid_potential].value(j, q_point)) -
               specific_capacitance_values[q_point] *
                   (fe_values[liquid_potential].value(i, q_point) *
                    fe_values[solid_potential].value(j, q_point)) +
               specific_capacitance_values[q_point] *
                   (fe_values[liquid_potential].value(i, q_point) *
                    fe_values[liquid_potential].value(j, q_point))) *
              fe_values.JxW(q_point);
        } // end for j
      }   // end for i
    }     // end for quadrature point
    cell->get_dof_indices(local_dof_indices);
    std::vector<dealii::types::global_dof_index> tmp_indices =
        dof_extractor.extract_row_indices(local_dof_indices);
    dealii::FullMatrix<double> tmp_mass_matrix =
        dof_extractor.extract_matrix(cell_mass_matrix);
    dealii::FullMatrix<double> tmp_stiffness_matrix =
        dof_extractor.extract_matrix(cell_stiffness_matrix);
    std::transform(tmp_indices.begin(), tmp_indices.end(), tmp_indices.begin(),
                   std::bind2nd(std::minus<dealii::types::global_dof_index>(),
                                this->dof_shift));
    constraint_matrix.distribute_local_to_global(
        tmp_stiffness_matrix, tmp_indices, this->stiffness_matrix);
    constraint_matrix.distribute_local_to_global(tmp_mass_matrix, tmp_indices,
                                                 this->mass_matrix);

    //        this->constraint_matrix.distribute_local_to_global(cell_stiffness_matrix,
    //        local_dof_indices, this->stiffness_matrix);
    //        this->constraint_matrix.distribute_local_to_global(cell_mass_matrix,
    //        local_dof_indices, this->mass_matrix);
  } // end for cell
}
template <int dim>
void ElectrochemicalOperator<dim>::compute_heat_source(
    dealii::BlockVector<double> const &potential_solution_vector,
    dealii::Vector<double> &thermal_load_vector) const
{
  dealii::DoFHandler<dim> const &dof_handler = *(this->dof_handler);
  dealii::ConstraintMatrix const &constraint_matrix =
      *(this->constraint_matrix);
  double coeff;
  if ((this->capacitor_state == GalvanostaticCharge) ||
      (this->capacitor_state == PotentiostaticCharge))
  {
    coeff = this->alpha;
  }
  else if ((this->capacitor_state == GalvanostaticDischarge) ||
           (this->capacitor_state == PotentiostaticDischarge))
  {
    coeff = -this->alpha;
  }
  else if (this->capacitor_state == Relaxation)
  {
    coeff = 0.0;
  }
  else
  {
    throw std::runtime_error(
        "compute_heat_source irreversible component not implemented yet");
  } // end if
  dealii::FiniteElement<dim> const &fe = dof_handler.get_fe();
  dealii::FEValuesExtractors::Scalar const solid_potential(
      this->solid_potential_component);
  dealii::FEValuesExtractors::Scalar const liquid_potential(
      this->liquid_potential_component);
  dealii::FEValuesExtractors::Scalar const temperature(
      this->temperature_component);
  dealii::QGauss<dim> quadrature_rule(fe.degree +
                                      1); // TODO: map to component...
  dealii::FEValues<dim> fe_values(
      fe, quadrature_rule, dealii::update_values | dealii::update_gradients |
                               dealii::update_JxW_values);
  unsigned int const dofs_per_cell = fe.dofs_per_cell;
  unsigned int const n_q_points = quadrature_rule.size();
  dealii::Vector<double> cell_load_vector(dofs_per_cell);
  std::vector<double> solid_phase_diffusion_coefficient_values(n_q_points);
  std::vector<double> liquid_phase_diffusion_coefficient_values(n_q_points);
  std::vector<dealii::Tensor<1, dim>> solid_potential_gradients(n_q_points);
  std::vector<dealii::Tensor<1, dim>> liquid_potential_gradients(n_q_points);
  std::vector<dealii::types::global_dof_index> local_dof_indices(dofs_per_cell);
  unsigned int const n_components = dealii::DoFTools::n_components(dof_handler);
  dealii::ComponentMask mask(n_components, false);
  mask.set(this->temperature_component, true);
  DoFExtractor dof_extractor(mask, mask, dofs_per_cell);
  std::vector<dealii::types::global_dof_index> dofs_per_component(n_components);
  dealii::DoFTools::count_dofs_per_component(dof_handler, dofs_per_component);
  dealii::types::global_dof_index const dof_shift =
      std::accumulate(&(dofs_per_component[0]),
                      &(dofs_per_component[this->temperature_component]), 0);
  typename dealii::DoFHandler<dim>::active_cell_iterator
      cell     = dof_handler.begin_active(),
      end_cell = dof_handler.end();
  for (; cell != end_cell; ++cell)
  {
    cell_load_vector = 0.0;
    fe_values.reinit(cell);
    (this->mp_values)
        ->get_values("solid_electrical_conductivity", cell,
                     solid_phase_diffusion_coefficient_values);
    (this->mp_values)
        ->get_values("liquid_electrical_conductivity", cell,
                     liquid_phase_diffusion_coefficient_values);
    fe_values[solid_potential].get_function_gradients(
        potential_solution_vector, solid_potential_gradients);
    fe_values[liquid_potential].get_function_gradients(
        potential_solution_vector, liquid_potential_gradients);
    for (unsigned int q_point = 0; q_point < n_q_points; ++q_point)
    {
      for (unsigned int i = 0; i < dofs_per_cell; ++i)
      {
        cell_load_vector(i) +=
            (+solid_phase_diffusion_coefficient_values[q_point] *
                 (solid_potential_gradients[q_point] *
                  solid_potential_gradients[q_point]) +
             liquid_phase_diffusion_coefficient_values[q_point] *
                 (liquid_potential_gradients[q_point] *
                  liquid_potential_gradients[q_point]) +
             coeff *
                 (solid_phase_diffusion_coefficient_values[q_point] *
                      solid_potential_gradients[q_point] +
                  liquid_phase_diffusion_coefficient_values[q_point] *
                      liquid_potential_gradients[q_point])
                     .norm()) *
            fe_values[temperature].value(i, q_point) * fe_values.JxW(q_point);
      } // end for i
    }   // end for quadrature point
    cell->get_dof_indices(local_dof_indices);
    std::vector<dealii::types::global_dof_index> tmp_indices =
        dof_extractor.extract_row_indices(local_dof_indices);
    dealii::Vector<double> tmp_load_vector =
        dof_extractor.extract_vector(cell_load_vector);
    if (dof_shift != 0)
      std::transform(tmp_indices.begin(), tmp_indices.end(),
                     tmp_indices.begin(),
                     std::bind2nd(std::minus<dealii::types::global_dof_index>(),
                                  dof_shift));
    constraint_matrix.distribute_local_to_global(tmp_load_vector, tmp_indices,
                                                 thermal_load_vector);
  } // end for cell
}

} // end namespace cap