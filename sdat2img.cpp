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

using namespace std;

constexpr static int block_size = 4096;
vector<string> valid_cmds = {"erase", "new", "zero"};

struct Data {
  int version;
  int new_blocks;
  vector<pair<string, vector<pair<int, int>>>> commands;
};

bool file_exists(const string &filename) {
  ifstream f(filename.data());
  return f.good();
}

void check_cmd(const string &cmd) {
  for (auto i : valid_cmds) {
    if (i == cmd)
      return;
  }
  cerr << "'" << cmd << "' Is not a valid command." << endl;
  exit(1);
}

vector<int> vector_string2int(vector<string> src) {
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

vector<string> split(const string &str, char delimiter) {
  vector<string> tokens;
  stringstream ss(str);
  string token;
  while (getline(ss, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

vector<int> rangeset(string src) {
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

Data parse_transfer_list(const string &transfer_list_file) {
  ifstream file(transfer_list_file);
  if (!file.is_open()) {
    perror("open");
    exit(1);
  }
  Data result;

  int version, new_blocks;
  string hold;

  // First line is the version
  getline(file, hold);
  try {
    version = stoi(hold);
  } catch (...) {
    cerr << "Could not determine transfer list version." << endl;
    exit(1);
  }

  // Second line is total number of blocks
  getline(file, hold);
  try {
    new_blocks = stoi(hold);
  } catch (...) {
    cerr << "Could not determine total number of blocks." << endl;
    exit(1);
  }

  // Skip those 2 lines if version >= 2
  if (version >= 2) {
    getline(file, hold);
    getline(file, hold);
  }

  string line, cmd;
  vector<int> nums;
  vector<pair<string, vector<pair<int, int>>>> commands;

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
  result.new_blocks = new_blocks;
  result.commands = commands;

  return result;
}

void resize_file(string path, long new_size) {
  fstream file(path.data(), ios::in | ios::out | ios::binary);
  if (!file) {
    cerr << "Error resizing file." << endl;
    exit(1);
  }
  file.seekg(0, ios::end);
  streamsize size = file.tellg();
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
  if (argc != 4) {
    usage(argv[0]);
  }
  string transfer_list_file = argv[1];
  string new_dat_file = argv[2];
  string output_img = argv[3];

  Data parsed = parse_transfer_list(transfer_list_file);
  int version = parsed.version;
  int new_blocks = parsed.new_blocks;
  vector<pair<string, vector<pair<int, int>>>> commands = parsed.commands;

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

  int max_second = 0;
  for (const auto &pair : all_block_sets) {
    if (pair.second > max_second) {
      max_second = pair.second;
    }
  }

  // new_blocks is the one defined on file,
  // max_second is the one we calculated.
  // They both should be the same.
  if (max_second != new_blocks) {
    cerr << "Failed to parse total blocks." << endl;
    exit(1);
  }

  long long max_file_size = max_second * block_size;

  string cmd;
  int block_count, begin, end;
  char buffer[block_size];

  for (auto p : commands) {
    cmd = p.first;
    if (cmd == "new") {
      for (auto block : p.second) {
        begin = block.first;
        end = block.second;
        block_count = end - begin;
        cout << "Copying " << block_count << " blocks into position " << begin
             << "..." << endl;
        output.seekp(begin * block_size, ios::beg);
        while (block_count > 0) {
          input_dat.read(buffer, block_size);
          output.write(buffer, block_size);
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
