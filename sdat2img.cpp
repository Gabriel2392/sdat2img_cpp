/*
 * This is a C++ equivalent version of the original sdat2img, which was originally written
 * in Python by xpirt, luxi78, and howellzhu.
 *
*/

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#define DEFAULT_OUTPUT "system.img"
#define BLOCK_SIZE 4096

using namespace std;
using ll = long long;
using command_vector = vector<pair<string, vector<pair<int, int>>>>;

vector<string> valid_cmds = {"erase", "new", "zero"};

struct Data {
  int version;
  command_vector commands;
};

bool file_exists(const string& filename) {
  ifstream f(filename.data());
  return f.good();
}

void check_cmd(const string& cmd) {
  for (auto i : valid_cmds) {
    if (i == cmd)
      return;
  }
  cerr << "'" << cmd << "' Is not a valid command." << endl;
  exit(1);
}

vector<int> vector_string2int(const vector<string>& src) {
  vector<int> tokens;
  int token;
  for (auto i : src) {
    try {
      token = stoi(i);
    } catch (...) {
      cerr << "Failed to convert string to int." << endl;
      exit(1);
    }
    tokens.push_back(token);
  }
  return tokens;
}

vector<string> split(const string &str, const char& delimiter) {
  vector<string> tokens;
  stringstream ss(str);
  string token;
  while (getline(ss, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

vector<int> rangeset(const string& src) {
  vector<string> src_set = split(src, ',');
  vector<int> num_set = vector_string2int(src_set);
  vector<int> ret;
  for (size_t i = 1; i < num_set.size(); i++) {
    ret.push_back(num_set[i]);
  }
  if (num_set.size() != static_cast<size_t>(num_set[0] + 1) || ret.size() % 2 != 0) {
    cerr << "Error on parsing following data to rangeset: " << src << endl;
    exit(1);
  }
  return ret;
}

Data parse_transfer_list(const string& transfer_list_file) {
  int version;
  string hold, line, cmd;
  vector<int> nums;
  command_vector commands;
  Data result;

  ifstream file(transfer_list_file);
  if (!file.is_open()) {
    perror("open");
    exit(1);
  }

  // First line is the version
  getline(file, hold);
  try {
    version = stoi(hold);
  } catch (...) {
    cerr << "Could not determine transfer list version." << endl;
    exit(1);
  }

  // Second line is total number of blocks. Ignore it though.
  // We are going to calculate it by ourselves.
  getline(file, hold);

  // Skip those 2 lines if version >= 2
  if (version >= 2) {
    getline(file, hold);
    getline(file, hold);
  }

  // Loop through all lines
  while (file.peek() != EOF) {
    getline(file, line);
    vector<string> split_line = split(line, ' ');
    if (split_line.size() != 2) { // We expect the command and the magic numbers
      cerr << "Failed to parse line '" << line << "'" << endl;
      exit(1);
    }
    cmd = split_line[0];
    check_cmd(cmd);
    nums = rangeset(split_line[1]);
    vector<pair<int, int>> pairs;
    for (size_t i = 0; i < nums.size(); i += 2) {
      pairs.push_back(make_pair(nums[i], nums[i + 1])); // Build our pairs
    }
    commands.push_back(make_pair(cmd, pairs)); // Push the cmd and our pairs to commands
  }

  file.close();

  result.version = version;
  result.commands = commands;

  return result;
}

void resize_file(const string& path, const ll& new_size) {
  ll size;

  fstream file(path.data(), ios::in | ios::out | ios::binary);
  if (!file) {
    cerr << "Error resizing file." << endl;
    exit(1);
  }
  file.seekg(0, ios::end);
  size = file.tellg();
  file.seekg(0, ios::beg);
  if (size < new_size) { // Only resize if needed.
    file.seekp(new_size - 1);
    file.put('\0');
  }
  file.close();
}

void usage(const char *exe) {
  cout << "Usage: " << exe << " <transfer_list> <system_new_file> <system_img>"
       << endl;
  cout << "    <transfer_list>: transfer list file" << endl;
  cout << "    <system_new_file>: system new dat file" << endl;
  cout << "    <system_img>: output system image" << endl;
  cout << "Visit xda thread for more information." << endl;
  exit(1);
}

int main(int argc, const char *argv[]) {
  ll max_file_size;
  string cmd, transfer_list_file, new_dat_file, output_img;
  int block_count, begin, end, version, max_second;
  char buffer[BLOCK_SIZE];

  if (argc != 4 && argc != 3) {
    usage(argv[0]);
  }

  transfer_list_file = argv[1];
  new_dat_file = argv[2];
  if (argc == 3) {
      output_img = DEFAULT_OUTPUT;
  } else {
      output_img = argv[3];
  }

  Data parsed = parse_transfer_list(transfer_list_file);
  command_vector commands = parsed.commands;
  version = parsed.version;

  if (version == 1) {
    cout << "Android 5.0 detected!" << endl;
  } else if (version == 2) {
    cout << "Android 5.1 detected!" << endl;
  } else if (version == 3) {
    cout << "Android 6.x detected!" << endl;
  } else if (version == 4) {
    cout << "Android 7.x or above detected!" << endl;
  } else {
    cout << "Unknown Android version!" << endl;
  }

  if (file_exists(output_img)) {
    cerr << "Error: The output file '" << output_img << "' already exists."
         << endl;
    cerr << "Remove it, rename it, or choose a different file name." << endl;
    return 1;
  }

  ofstream output(output_img, ios::binary);
  if (!output) {
    cerr << "Error: Could not open file " << output_img << endl;
    return 1;
  }

  ifstream input_dat(new_dat_file, ios::binary);
  if (!input_dat) {
    cerr << "Error: Could not open file " << new_dat_file << endl;
    return 1;
  }

  vector<pair<int, int>> all_block_sets;
  vector<pair<int, int>> cur_set;
  for (size_t j = 0; j < commands.size(); j++) {
    cur_set = commands[j].second;
    for (size_t i = 0; i < cur_set.size(); i++) {
      all_block_sets.push_back(cur_set[i]);
    }
  }

  max_second = 0;
  for (const auto &pair : all_block_sets) {
    if (pair.second > max_second) {
      max_second = pair.second;
    }
  }

  max_file_size = max_second * BLOCK_SIZE;

  for (auto p : commands) {
    cmd = p.first;
    if (cmd == "new") {
      for (auto block : p.second) {
        begin = block.first;
        end = block.second;
        block_count = end - begin;
        cout << "Copying " << block_count << " blocks into position " << begin
             << "..." << endl;
        output.seekp(begin * BLOCK_SIZE, ios::beg);
        while (block_count > 0) {
          input_dat.read(buffer, BLOCK_SIZE);
          output.write(buffer, BLOCK_SIZE);
          block_count--;
        }
      }
    } else {
      cout << "Skipping command " << cmd << "..." << endl;
    }
  }

  output.close();
  input_dat.close();

  resize_file(output_img, max_file_size);
  cout << "Done! Output image: " << output_img << endl;
  return 0;
}
