/*-----------------------------------------------------------------------------------------------100
compile:
    g++ -O3 nep_data_toolkit.cpp
run:
    ./a.out
--------------------------------------------------------------------------------------------------*/

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

static std::string remove_spaces_step1(const std::string& line)
{
  std::vector<int> indices_for_spaces(line.size(), 0);
  for (int n = 0; n < line.size(); ++n) {
    if (line[n] == '=') {
      for (int k = 1; n - k >= 0; ++k) {
        if (line[n - k] == ' ' || line[n - k] == '\t') {
          indices_for_spaces[n - k] = 1;
        } else {
          break;
        }
      }
      for (int k = 1; n + k < line.size(); ++k) {
        if (line[n + k] == ' ' || line[n + k] == '\t') {
          indices_for_spaces[n + k] = 1;
        } else {
          break;
        }
      }
    }
  }

  std::string new_line;
  for (int n = 0; n < line.size(); ++n) {
    if (!indices_for_spaces[n]) {
      new_line += line[n];
    }
  }

  return new_line;
}

static std::string remove_spaces(const std::string& line_input)
{
  auto line = remove_spaces_step1(line_input);

  std::vector<int> indices_for_spaces(line.size(), 0);
  for (int n = 0; n < line.size(); ++n) {
    if (line[n] == '\"') {
      if (n == 0) {
        std::cout << "The second line of the .xyz file should not begin with \"." << std::endl;
        exit(1);
      } else {
        if (line[n - 1] == '=') {
          for (int k = 1; n + k < line.size(); ++k) {
            if (line[n + k] == ' ' || line[n + k] == '\t') {
              indices_for_spaces[n + k] = 1;
            } else {
              break;
            }
          }
        } else {
          for (int k = 1; n - k >= 0; ++k) {
            if (line[n - k] == ' ' || line[n - k] == '\t') {
              indices_for_spaces[n - k] = 1;
            } else {
              break;
            }
          }
        }
      }
    }
  }

  std::string new_line;
  for (int n = 0; n < line.size(); ++n) {
    if (!indices_for_spaces[n]) {
      new_line += line[n];
    }
  }

  return new_line;
}

std::vector<std::string> get_tokens(const std::string& line)
{
  std::istringstream iss(line);
  std::vector<std::string> tokens{
    std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
  return tokens;
}

std::vector<std::string> get_tokens_without_unwanted_spaces(std::ifstream& input)
{
  std::string line;
  std::getline(input, line);
  auto line_without_unwanted_spaces = remove_spaces(line);
  std::istringstream iss(line_without_unwanted_spaces);
  std::vector<std::string> tokens{
    std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
  return tokens;
}

std::vector<std::string> get_tokens(std::ifstream& input)
{
  std::string line;
  std::getline(input, line);
  std::istringstream iss(line);
  std::vector<std::string> tokens{
    std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
  return tokens;
}

int get_int_from_token(const std::string& token, const char* filename, const int line)
{
  int value = 0;
  try {
    value = std::stoi(token);
  } catch (const std::exception& e) {
    std::cout << "Standard exception:\n";
    std::cout << "    File:          " << filename << std::endl;
    std::cout << "    Line:          " << line << std::endl;
    std::cout << "    Error message: " << e.what() << std::endl;
    exit(1);
  }
  return value;
}

double get_double_from_token(const std::string& token, const char* filename, const int line)
{
  double value = 0;
  try {
    value = std::stod(token);
  } catch (const std::exception& e) {
    std::cout << "Standard exception:\n";
    std::cout << "    File:          " << filename << std::endl;
    std::cout << "    Line:          " << line << std::endl;
    std::cout << "    Error message: " << e.what() << std::endl;
    exit(1);
  }
  return value;
}

struct Structure {
  int num_atom;
  std::string sid;
  bool has_sid = false;
  bool has_virial = false;
  bool has_stress = false;
  double energy_weight = 1.0;
  double energy;
  double weight;
  double virial[9];
  double stress[9];
  double box[9];
  std::vector<std::string> atom_symbol;
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> z;
  std::vector<double> fx;
  std::vector<double> fy;
  std::vector<double> fz;
  std::vector<double> q;
};

static void read_force(
  const int num_columns,
  const int species_offset,
  const int pos_offset,
  const int force_offset,
  std::ifstream& input,
  Structure& structure)
{
  structure.atom_symbol.resize(structure.num_atom);
  structure.x.resize(structure.num_atom);
  structure.y.resize(structure.num_atom);
  structure.z.resize(structure.num_atom);
  structure.fx.resize(structure.num_atom);
  structure.fy.resize(structure.num_atom);
  structure.fz.resize(structure.num_atom);

  for (int na = 0; na < structure.num_atom; ++na) {
    std::vector<std::string> tokens = get_tokens(input);
    if (tokens.size() != num_columns) {
      std::cout << "Number of items for an atom line mismatches properties." << std::endl;
      exit(1);
    }
    structure.atom_symbol[na] = tokens[0 + species_offset];
    structure.x[na] = get_double_from_token(tokens[0 + pos_offset], __FILE__, __LINE__);
    structure.y[na] = get_double_from_token(tokens[1 + pos_offset], __FILE__, __LINE__);
    structure.z[na] = get_double_from_token(tokens[2 + pos_offset], __FILE__, __LINE__);
    if (num_columns > 4) {
      structure.fx[na] = get_double_from_token(tokens[0 + force_offset], __FILE__, __LINE__);
      structure.fy[na] = get_double_from_token(tokens[1 + force_offset], __FILE__, __LINE__);
      structure.fz[na] = get_double_from_token(tokens[2 + force_offset], __FILE__, __LINE__);
    }
  }
}

static void read_one_structure(std::ifstream& input, Structure& structure)
{
  std::vector<std::string> tokens = get_tokens_without_unwanted_spaces(input);
  for (auto& token : tokens) {
    std::transform(
      token.begin(), token.end(), token.begin(), [](unsigned char c) { return std::tolower(c); });
  }

  if (tokens.size() == 0) {
    std::cout << "The second line for each frame should not be empty." << std::endl;
    exit(1);
  }

  for (const auto& token : tokens) {
    const std::string sid_string = "sid=";
    if (token.substr(0, sid_string.length()) == sid_string) {
      structure.has_sid = true;
      structure.sid = token.substr(sid_string.length(), token.length());
    }
  }

  // get energy_weight (optional)
  for (const auto& token : tokens) {
    const std::string energy_weight_string = "energy_weight=";
    if (token.substr(0, energy_weight_string.length()) == energy_weight_string) {
      structure.energy_weight = get_double_from_token(
        token.substr(energy_weight_string.length(), token.length()), __FILE__, __LINE__);
    }
  }

  bool has_energy_in_exyz = false;
  for (const auto& token : tokens) {
    const std::string energy_string = "energy=";
    if (token.substr(0, energy_string.length()) == energy_string) {
      has_energy_in_exyz = true;
      structure.energy = get_double_from_token(
        token.substr(energy_string.length(), token.length()), __FILE__, __LINE__);
    }
  }
  if (!has_energy_in_exyz) {
    std::cout << "'energy' is missing in the second line of a frame." << std::endl;
    exit(1);
  }

  structure.weight = 1.0f;
  for (const auto& token : tokens) {
    const std::string weight_string = "weight=";
    if (token.substr(0, weight_string.length()) == weight_string) {
      structure.weight = get_double_from_token(
        token.substr(weight_string.length(), token.length()), __FILE__, __LINE__);
      if (structure.weight <= 0.0f || structure.weight > 100.0f) {
        std::cout << "Configuration weight should > 0 and <= 100." << std::endl;
        exit(1);
      }
    }
  }

  bool has_lattice_in_exyz = false;
  for (int n = 0; n < tokens.size(); ++n) {
    const std::string lattice_string = "lattice=";
    if (tokens[n].substr(0, lattice_string.length()) == lattice_string) {
      has_lattice_in_exyz = true;
      for (int m = 0; m < 9; ++m) {
        structure.box[m] = get_double_from_token(
          tokens[n + m].substr(
            (m == 0) ? (lattice_string.length() + 1) : 0,
            (m == 8) ? (tokens[n + m].length() - 1) : tokens[n + m].length()),
          __FILE__, __LINE__);
      }
    }
  }
  if (!has_lattice_in_exyz) {
    std::cout << "'lattice' is missing in the second line of a frame." << std::endl;
    exit(1);
  }

  structure.has_virial = false;
  for (int n = 0; n < tokens.size(); ++n) {
    const std::string virial_string = "virial=";
    if (tokens[n].substr(0, virial_string.length()) == virial_string) {
      structure.has_virial = true;
      for (int m = 0; m < 9; ++m) {
        structure.virial[m] = get_double_from_token(
          tokens[n + m].substr(
            (m == 0) ? (virial_string.length() + 1) : 0,
            (m == 8) ? (tokens[n + m].length() - 1) : tokens[n + m].length()),
          __FILE__, __LINE__);
      }
    }
  }

  if (!structure.has_virial) {
    for (int n = 0; n < tokens.size(); ++n) {
      const std::string stress_string = "stress=";
      if (tokens[n].substr(0, stress_string.length()) == stress_string) {
        structure.has_stress = true;
        for (int m = 0; m < 9; ++m) {
          structure.stress[m] = get_double_from_token(
            tokens[n + m].substr(
              (m == 0) ? (stress_string.length() + 1) : 0,
              (m == 8) ? (tokens[n + m].length() - 1) : tokens[n + m].length()),
            __FILE__, __LINE__);
        }
      }
    }
  }

  int species_offset = 0;
  int pos_offset = 0;
  int force_offset = 0;
  int num_columns = 0;
  for (int n = 0; n < tokens.size(); ++n) {
    const std::string properties_string = "properties=";
    if (tokens[n].substr(0, properties_string.length()) == properties_string) {
      std::string line = tokens[n].substr(properties_string.length(), tokens[n].length());
      for (auto& letter : line) {
        if (letter == ':') {
          letter = ' ';
        }
      }
      std::vector<std::string> sub_tokens = get_tokens(line);
      int species_position = -1;
      int pos_position = -1;
      int force_position = -1;
      for (int k = 0; k < sub_tokens.size() / 3; ++k) {
        if (sub_tokens[k * 3] == "species") {
          species_position = k;
        }
        if (sub_tokens[k * 3] == "pos") {
          pos_position = k;
        }
        if (sub_tokens[k * 3] == "force" || sub_tokens[k * 3] == "forces") {
          force_position = k;
        }
      }
      if (species_position < 0) {
        std::cout << "'species' is missing in properties." << std::endl;
        exit(1);
      }
      if (pos_position < 0) {
        std::cout << "'pos' is missing in properties." << std::endl;
        exit(1);
      }
      if (force_position < 0) {
        std::cout << "'force' or 'forces' is missing in properties." << std::endl;
        exit(1);
      }
      for (int k = 0; k < sub_tokens.size() / 3; ++k) {
        if (k < species_position) {
          species_offset += get_int_from_token(sub_tokens[k * 3 + 2], __FILE__, __LINE__);
        }
        if (k < pos_position) {
          pos_offset += get_int_from_token(sub_tokens[k * 3 + 2], __FILE__, __LINE__);
        }
        if (k < force_position) {
          force_offset += get_int_from_token(sub_tokens[k * 3 + 2], __FILE__, __LINE__);
        }
        num_columns += get_int_from_token(sub_tokens[k * 3 + 2], __FILE__, __LINE__);
      }
    }
  }

  read_force(num_columns, species_offset, pos_offset, force_offset, input, structure);
}

static void read(const std::string& inputfile, std::vector<Structure>& structures)
{
  std::ifstream input(inputfile);
  if (!input.is_open()) {
    std::cout << "Failed to open " << inputfile << std::endl;
    exit(1);
  } else {
    while (true) {
      std::vector<std::string> tokens = get_tokens(input);
      if (tokens.size() == 0) {
        break;
      } else if (tokens.size() > 1) {
        std::cout << "The first line for each frame should have one value." << std::endl;
        exit(1);
      }
      Structure structure;
      structure.num_atom = get_int_from_token(tokens[0], __FILE__, __LINE__);
      if (structure.num_atom < 1) {
        std::cout << "Number of atoms for each frame should >= 1." << std::endl;
        exit(1);
      }
      read_one_structure(input, structure);
      structures.emplace_back(structure);
    }
    input.close();
  }
}

static void write_one_structure(std::ofstream& output, const Structure& structure)
{
  output << structure.num_atom << "\n";

  if (structure.energy_weight != 1.0) {
    output << "energy_weight=" << structure.energy_weight << " ";
  }

  output << "Lattice=\"";
  for (int m = 0; m < 9; ++m) {
    output << structure.box[m];
    if (m != 8) {
      output << " ";
    }
  }
  output << "\" ";

  output << "energy=" << structure.energy << " ";

  if (structure.has_virial) {
    output << "virial=\"";
    for (int m = 0; m < 9; ++m) {
      output << structure.virial[m];
      if (m != 8) {
        output << " ";
      }
    }
    output << "\" ";
  }

  if (structure.has_stress) {
    output << "stress=\"";
    for (int m = 0; m < 9; ++m) {
      output << structure.stress[m];
      if (m != 8) {
        output << " ";
      }
    }
    output << "\" ";
  }

  if (structure.has_sid) {
    output << "sid=" << structure.sid << " ";
  }

  output << "Properties=species:S:1:pos:R:3:force:R:3\n";

  for (int n = 0; n < structure.num_atom; ++n) {
    output << structure.atom_symbol[n] << " " << structure.x[n] << " " << structure.y[n] << " "
           << structure.z[n] << " " << structure.fx[n] << " " << structure.fy[n] << " "
           << structure.fz[n] << "\n";
  }
}

static void write(
  const std::string& outputfile,
  const std::vector<Structure>& structures)
{
  std::ofstream output(outputfile);
  if (!output.is_open()) {
    std::cout << "Failed to open " << outputfile << std::endl;
    exit(1);
  }
  std::cout << outputfile << " is opened." << std::endl;
  for (int nc = 0; nc < structures.size(); ++nc) {
    write_one_structure(output, structures[nc]);
  }
  output.close();
  std::cout << outputfile << " is closed." << std::endl;
}

static void split_into_accurate_and_inaccurate(
  const std::vector<Structure>& structures, 
  double energy_threshold, 
  double force_threshold,
  double virial_threshold)
{
  std::ifstream input_energy("energy_train.out");
  std::ifstream input_force("force_train.out");
  std::ifstream input_virial("virial_train.out");
  std::ofstream output_accurate("accurate.xyz");
  std::ofstream output_inaccurate("inaccurate.xyz");
  int num1 = 0;
  int num2 = 0;
  for (int nc = 0; nc < structures.size(); ++nc) {
    bool energy_is_small = structures[nc].energy < 0.0;
    bool force_is_small = true;
    for (int n = 0; n < structures[nc].num_atom; ++n) {
      double fx = structures[nc].fx[n];
      double fy = structures[nc].fy[n];
      double fz = structures[nc].fz[n];
      if (fx * fx + fy * fy + fz * fz > 400.0) {
        force_is_small = false;
        break;
      }
    }
    bool is_considered = (energy_is_small || structures[nc].energy_weight < 0.5f) && force_is_small;

    bool is_accurate = true;

    double energy_nep = 0.0;
    double energy_ref = 0.0;
    input_energy >> energy_nep >> energy_ref;

    if (structures[nc].energy_weight > 0.5f && energy_threshold > 0) {
      if (std::abs(energy_nep - energy_ref) > energy_threshold) {
        is_accurate = false;
      }
    }

    double force_nep[3];
    double force_ref[3];
    for (int n = 0; n < structures[nc].num_atom; ++n) {
      input_force >> force_nep[0] >> force_nep[1] >> force_nep[2] >> force_ref[0] >> force_ref[1] >> force_ref[2];
      double fx_diff = force_nep[0] - force_ref[0];
      double fy_diff = force_nep[1] - force_ref[1];
      double fz_diff = force_nep[2] - force_ref[2];
      if (fx_diff * fx_diff + fy_diff * fy_diff + fz_diff * fz_diff > force_threshold * force_threshold) {
        is_accurate = false;
      }
    }

    double virial_nep[6];
    double virial_ref[6];
    for (int n = 0; n < 6; ++n) {
      input_virial >> virial_nep[n];
    }
    for (int n = 0; n < 6; ++n) {
      input_virial >> virial_ref[n];
    }
    for (int n = 0; n < 6; ++n) {
      if (std::abs(virial_nep[n] - virial_ref[n]) > virial_threshold) {
        is_accurate = false;
      }
    }

    if (is_considered) {
      if (is_accurate) {
        write_one_structure(output_accurate, structures[nc]);
        num1++;
      } else{
        write_one_structure(output_inaccurate, structures[nc]);
        num2++;
      }
    }
  }
  input_energy.close();
  input_force.close();
  input_virial.close();
  output_accurate.close();
  output_inaccurate.close();
  std::cout << "Number of structures written into accurate.xyz = " << num1 << std::endl;
  std::cout << "Number of structures written into inaccurate.xyz = " << num2 << std::endl;
}

static void fps(std::vector<Structure>& structures, double distance_square_min, int dim)
{
  std::ifstream input_descriptor("descriptor.out");
  std::ofstream output_selected("selected.xyz");
  std::ofstream output_not_selected("not_selected.xyz");
  std::ofstream output_index_selected("indices_selected.txt");
  std::ofstream output_index_not_selected("indices_not_selected.txt");
  std::vector<Structure> structures_selected;

  int num1 = 0;
  int num2 = 0;

  for (int nc = 0; nc < structures.size(); ++nc) {
    structures[nc].q.resize(dim);
    for (int d = 0; d < dim; ++d) {
      input_descriptor >> structures[nc].q[d];
    }
    if (nc == 0) {
      structures_selected.emplace_back(structures[nc]);
      output_index_selected << nc << "\n";
      num1++;
      write_one_structure(output_selected, structures[nc]);
    } else {
      bool to_be_selected = true;
      for (int m = 0; m < structures_selected.size(); ++m) {
        double distance_square = 0.0;
        for (int d = 0; d < dim; ++d) {
          double temp = (structures[nc].q[d] - structures_selected[m].q[d]);
          distance_square += temp * temp;
        }
        if (distance_square < distance_square_min) {
          to_be_selected = false;
          break;
        }
      }
      if (to_be_selected) {
        structures_selected.emplace_back(structures[nc]);
        output_index_selected << nc << "\n";
        num1++;
        if (num1 % 1000 == 0) {
          std::cout << "#selected = " << num1 << ", current structure ID = " << nc << "\n";
        }
        write_one_structure(output_selected, structures[nc]);
      } else {
        output_index_not_selected << nc << "\n";
        num2++;
        write_one_structure(output_not_selected, structures[nc]);
      }
    }
  }

  input_descriptor.close();
  output_selected.close();
  output_not_selected.close();
  output_index_selected.close();
  output_index_not_selected.close();
  std::cout << "Number of structures written into selected.xyz = " << num1 << std::endl;
  std::cout << "Number of structures written into not_selected.xyz = " << num2 << std::endl;
}

int main(int argc, char* argv[])
{
  std::cout << "====================================================\n";
  std::cout << "Welcome to use nep_data_toolkit!" << std::endl;
  std::cout << "Here are the functionalities:" << std::endl;
  std::cout << "----------------------------------------------------\n";
  std::cout << "1: count the number of structures\n";
  std::cout << "2: copy\n";
  std::cout << "3: split into accurate.xyz and inaccurate.xyz\n";
  std::cout << "5: descriptor-space subsampling\n";
  std::cout << "====================================================\n";

  std::cout << "Please choose a number based on your purpose: ";
  int option;
  std::cin >> option;

  if (option == 1) {
    std::cout << "Please enter the input xyz filename: ";
    std::string input_filename;
    std::cin >> input_filename;
    std::vector<Structure> structures_input;
    read(input_filename, structures_input);
    std::cout << "Number of structures read from "
              << input_filename + " = " << structures_input.size() << std::endl;
  } else if (option == 2) {
    std::cout << "Please enter the input xyz filename: ";
    std::string input_filename;
    std::cin >> input_filename;
    std::cout << "Please enter the output xyz filename: ";
    std::string output_filename;
    std::cin >> output_filename;
    std::vector<Structure> structures_input;
    read(input_filename, structures_input);
    std::cout << "Number of structures read from "
              << input_filename + " = " << structures_input.size() << std::endl;
    write(output_filename, structures_input);
  } else if (option == 3) {
    std::cout << "Please enter the input xyz filename: ";
    std::string input_filename;
    std::cin >> input_filename;
    std::cout << "Please enter the energy threshold in units of eV/atom (negative to ignore): ";
    double energy_threshold;
    std::cin >> energy_threshold;
    std::cout << "Please enter the force threshold in units of eV/A: ";
    double force_threshold;
    std::cin >> force_threshold;
    std::cout << "Please enter the virial threshold in units of eV/atom: ";
    double virial_threshold;
    std::cin >> virial_threshold;
    std::vector<Structure> structures_input;
    read(input_filename, structures_input);
    std::cout << "Number of structures read from "
              << input_filename + " = " << structures_input.size() << std::endl;
    split_into_accurate_and_inaccurate(structures_input, energy_threshold, force_threshold, virial_threshold);
  } else if (option == 5) {
    std::cout << "Please enter the input xyz filename: ";
    std::string input_filename;
    std::cin >> input_filename;
    std::cout << "Please enter the minimal distance in descriptor space: ";
    double distance;
    std::cin >> distance;
    std::cout << "Please enter the dimension of descriptor space: ";
    int dim;
    std::cin >> dim;
    std::vector<Structure> structures_input;
    read(input_filename, structures_input);
    std::cout << "Number of structures read from "
              << input_filename + " = " << structures_input.size() << std::endl;

    clock_t time_begin = clock();
    fps(structures_input, distance * distance, dim);
    clock_t time_finish = clock();
    double time_used = (time_finish - time_begin) / double(CLOCKS_PER_SEC);
    std::cout << "Time used for descriptor-space subsampling = " << time_used << " s.\n";
  } else {
    std::cout << "This is an invalid option.";
    exit(1);
  }

  std::cout << "Done." << std::endl;
  return EXIT_SUCCESS;
}
