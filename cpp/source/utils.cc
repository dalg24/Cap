/* Copyright (c) 2016, the Cap authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#include <cap/utils.templates.h>
#ifdef WITH_DEAL_II
#include <deal.II/base/types.h>
#endif

namespace cap
{

template std::map<std::string, int> to_map(std::string const &s);
template std::map<std::string, double> to_map(std::string const &s);
template std::map<std::string, std::string> to_map(std::string const &s);
template std::map<std::string, bool> to_map(std::string const &s);

template std::vector<unsigned int> to_vector(std::string const &s);
template std::vector<int> to_vector(std::string const &s);
template std::vector<float> to_vector(std::string const &s);
template std::vector<double> to_vector(std::string const &s);
template std::vector<std::string> to_vector(std::string const &s);
template std::vector<bool> to_vector(std::string const &s);

template std::string to_string(std::vector<int> const &v);
template std::string to_string(std::vector<unsigned int> const &v);
template std::string to_string(std::vector<float> const &v);
template std::string to_string(std::vector<double> const &v);
template std::string to_string(std::vector<std::string> const &v);
template std::string to_string(std::vector<bool> const &v);

} // end namespace cap
