#include <memory>
#ifdef _WIN32
#include <io.h>
#define access    _access_s
#else
#include <unistd.h>
#endif
#include <iostream>
#include <fstream>
#include <memory>
#include <map>
#include <cstring>
#include <algorithm>
#include <regex>
#include "dirent.h"
#include <sys/stat.h>
#include "getopt.h"
#include <direct.h>
#define SORT_BY_ALPHA 1
#define SORT_BY_FREQUENCY 0

using namespace std;

void print_usage();

unique_ptr<map<string, int>> compose_words_map(const string& input_file_path);

template <class T>
void serialize_map(T* map, const string& file_path);
template <class T>
void serialize_map2(T* map, const string& file_path);
void split(char* input_string, const char* delimiters, vector<string>* output_tokens);

void split(const string& input_string, const char* delimiters,
	vector<string>* output_tokens);

void take_torn_word(ifstream& is, const unique_ptr<char>& buffer, int chunk_size, const char* delimiters);

void merge_files(const vector<string>& filenames, const string& result_file_path,
	int sorting_mode);

pair<string, int> parse_pair(const string& basic_string);


void split_file(const string& input_file_name, int sorting_mode, const string& output_path);

void read_map_from_file(const string& file_name, map<string, int>* result_map);

void read_map_from_file(const string& file_name, vector<pair<string, int>>* result_map);

void process_file(const string& input_file_path, const string& output_file_path, int sorting_mode);

string get_new_map_fullpath(const string& output_dir);
string get_map_fullpath(const string& output_dir, int n);


bool file_exists(const std::string& filename)
{
	return access(filename.c_str(), 0) == 0;
}

bool is_string_alpha(const string& str) {
	for (char i : str)
		if (!isalpha(i)) {
			return false;
		}
	return true;
}

string strip_quotes(const string& str) {
	if (str.at(0) == '\'') {
		return str.substr(1);
	}
	return str;
}

int main(int argc, char* argv[]) {
	if (argc == 1) {
		print_usage();
		return 1;
	}
	int option;
	int sorting_mode = SORT_BY_ALPHA;
	int split_mode = 0;
	int merge_mode = 0;
	vector<string> files_for_merging;
	vector<string> numbers;
	string input_file_path;
	string output_file_path;

	

	while ((option = getopt(argc, argv, "fai:o:m:s:")) != -1) {
		switch (option) {
		case 'f':
			sorting_mode = SORT_BY_FREQUENCY;
			break;
		case 'a':
			sorting_mode = SORT_BY_ALPHA;
			break;
		case 'i':
			if (!optarg) {
				print_usage();
				return 1;
			}
			input_file_path = regex_replace(string(optarg), regex("'"), "");
			if (!file_exists(input_file_path)) {
				cout << "Error: couldn't find the file: " << input_file_path << "\n";
				return 1;
			}
			break;
		case 'm':
			if (!optarg) {
				print_usage();
				return 1;
			}
			merge_mode = 1;
			numbers.push_back(string(optarg));
			break;
		/*case 's':
			if (!optarg) {
				print_usage();
				return 1;
			}
			split_mode = 1;
			input_file_path = string(optarg);
			break;*/
		default:
			std::cout << "Unrecognized option: '" << char(option) << "'\n";
			print_usage();
			system("pause");
			break;
		}
	}

	char current_dir[128];
	_getcwd(current_dir, 128);
	if (!argv[optind]) {
		output_file_path = current_dir;
	}
	else {
		output_file_path = string(argv[optind]);
		string absolute_path_prefix("/");
		if (output_file_path.compare(0, absolute_path_prefix.size(), absolute_path_prefix)) {
			output_file_path = string(current_dir) + "/" + output_file_path;
		}
	}
	if (!numbers.empty()) {
		for (int i = 0; i < numbers.size(); i++) {
			//cout << numbers.at(i) << endl;
			files_for_merging.push_back(get_map_fullpath(output_file_path, std::stoi(numbers[i])));
		}
		
	}


	if (merge_mode) {
		if (!input_file_path.empty()) {
			cout << "Warning: input will be ignored while in merge mode.\n";
		}
		if (files_for_merging.empty()) {
			cout << "No files specified for merge.\n";
			return 1;
		}
		bool error = false;
		for (auto& filename : files_for_merging) {
			if (!file_exists(filename)) {
				cout << "Error: couldn't find: " << filename << "\n";
				error = true;
			}
		}
		if (error) { return 1; }
		try {
			merge_files(files_for_merging, output_file_path, sorting_mode);
		}
		catch (invalid_argument& e) {
			cout << e.what() << "\n";
		}
		return 0;
	}

	if (split_mode) {
		if (!file_exists(input_file_path)) {
			cout << "Error: couldn't find: " << input_file_path << "\n";
			return 1;
		}
		split_file(input_file_path, sorting_mode, output_file_path);
		return 0;
	}

	process_file(input_file_path, output_file_path, sorting_mode);
	return 0;
	
}

void а(const string& input_file_path, const string& output_file_path,
	int sorting_mode) {
	unique_ptr<map<string, int>> map_ptr = compose_words_map(input_file_path);
	switch (sorting_mode) {
	case SORT_BY_ALPHA:
		serialize_map(map_ptr.get(),
			output_file_path);
		break;
	case SORT_BY_FREQUENCY:
		vector<pair<string, int>> map_by_freq(map_ptr->begin(), map_ptr->end());
		map_ptr.reset();
		sort(map_by_freq.begin(), map_by_freq.end(),
			[](const pair<string, int>& l, const pair<string, int>& r) {
				return l.second < r.second;
			});
		serialize_map(&map_by_freq, output_file_path);
		break;
	}
}

void split_file(const string& input_file_name, int sorting_mode, const string& output_path) {
	vector<pair<string, int>> map_input;
	read_map_from_file(input_file_name, &map_input);
	if (map_input.empty()) {
		return;
	}
	vector<pair<string, int>> map_first_half;
	vector<pair<string, int>> map_second_half;
	int half_index = static_cast<int>(map_input.size() / 2);
	for (auto iter = map_input.begin(); iter < (map_input.begin() + half_index); ++iter) {
		map_first_half.push_back(*iter);
	}
	for (auto iter = map_input.begin() + half_index; iter < map_input.end(); ++iter) {
		map_second_half.push_back(*iter);
	}
	switch (sorting_mode) {
	case SORT_BY_ALPHA:
		sort(map_first_half.begin(), map_first_half.end(),
			[](const pair<string, int>& l, const pair<string, int>& r) {
				return l.first < r.first;
			});
		sort(map_second_half.begin(), map_second_half.end(),
			[](const pair<string, int>& l, const pair<string, int>& r) {
				return l.first < r.first;
			});
		break;
	case SORT_BY_FREQUENCY:
		sort(map_first_half.begin(), map_first_half.end(),
			[](const pair<string, int>& l, const pair<string, int>& r) {
				return l.second < r.second;
			});
		sort(map_second_half.begin(), map_second_half.end(),
			[](const pair<string, int>& l, const pair<string, int>& r) {
				return l.second < r.second;
			});
		break;
	default:
		break;
	}
	serialize_map(&map_first_half, output_path);
	serialize_map(&map_second_half, output_path);
}

void merge_files(const vector<string>& filenames, const string& result_file_path,
	int sorting_mode) {
	map<string, int> merge_map;
	for (const string& file_name : filenames) {
		//cout << file_name << endl;
		read_map_from_file(file_name, &merge_map);
	}

	vector<pair<string, int>> merge_map_vec(merge_map.begin(), merge_map.end());
	switch (sorting_mode) {
	case SORT_BY_FREQUENCY:
		sort(merge_map_vec.begin(), merge_map_vec.end(),
			[](const pair<string, int>& l, const pair<string, int>& r) {
				return l.second > r.second;
			});
		break;
	case SORT_BY_ALPHA:
		sort(merge_map_vec.begin(), merge_map_vec.end(),
			[](const pair<string, int>& l, const pair<string, int>& r) {
				return l.first < r.first;
			});
	}
	vector<pair<string, int>> map_first_half;
	vector<pair<string, int>> map_second_half;
	int half_index = static_cast<int>(merge_map_vec.size() / 2);
	for (auto iter = merge_map_vec.begin(); iter < (merge_map_vec.begin() + half_index); ++iter) {
		map_first_half.push_back(*iter);
	}
	for (auto iter = merge_map_vec.begin() + half_index; iter < merge_map_vec.end(); ++iter) {
		map_second_half.push_back(*iter);
	}
	cout << filenames[0] << endl;
	cout << filenames[1] << endl;
	serialize_map2(&map_first_half, filenames[0]);
	serialize_map2(&map_second_half, filenames[1]);
}

void read_map_from_file(const string& file_name, map<string, int>* result_map) {
	try {
		ifstream is(file_name);
		if (!is) { return; }
		while (is) {
			string line;
			getline(is, line);
			if (!line.empty()) {
				(*result_map)[parse_pair(line).first] += parse_pair(line).second;
				//result_map->emplace(parse_pair(line));
			}
		}
	}
	catch (invalid_argument& e) {
		throw invalid_argument("Invalid file format for :" + file_name + "\n" + e.what());
	}
}

void read_map_from_file(const string& file_name, vector<pair<string, int>>* result_map) {
	try {
		ifstream is(file_name);
		if (!is) { return; }
		while (is) {
			string line;
			getline(is, line);
			if (!line.empty()) {
				result_map->emplace_back(parse_pair(line));
			}
		}
	}
	catch (invalid_argument& e) {
		throw invalid_argument("Invalid file format for :" + file_name + "\n" + e.what());
	}
}


pair<string, int> parse_pair(const string& basic_string) {
	vector<string> tokens;
	int frequency;

	split(basic_string, ":", &tokens);
	if (tokens.empty()) {
		throw invalid_argument("Couldn't parse values");
	}
	try {
		frequency = stoi(tokens.at(1));
	}
	catch (invalid_argument& e) {
		throw e;
	}
	return make_pair(tokens[0], frequency);
}

template <class T>
void serialize_map(T* map, const string& file_path) {
	ofstream os;
	os.open(get_new_map_fullpath(file_path));
	for (auto& item : *map) {
		os << item.first << ":" << item.second << " \n";
	}
	os.close();
}
template <class T>
void serialize_map2(T* map, const string& file_path) {
	ofstream os;
	os.open(file_path);
	for (auto& item : *map) {
		os << item.first << ":" << item.second << " \n";
	}
	os.close();
}
unique_ptr<map<string, int>> compose_words_map(const string& input_file_path) {
	constexpr int CHUNK_SIZE = 1024 * 1024 + 20;
	const static char* delimiters = " \n\r\t,./\\[]{}\";:<>*-+=()@#$%^&_!?`¦|~";
	unique_ptr<map<string, int>> word_frequency_map(new map<string, int>());
	unique_ptr<char> buffer{ new char[CHUNK_SIZE] };

	ifstream is{ input_file_path };
	while (is) {
		vector<string> words;
		is.read(buffer.get(), CHUNK_SIZE);
		if (!buffer) {
			break;
		}
		take_torn_word(is, buffer, CHUNK_SIZE, delimiters);
		split(buffer.get(), delimiters, &words);
		for (auto& word : words) {
			(*word_frequency_map)[word]++;
		}
	}
	is.close();
	return word_frequency_map;
}

void take_torn_word(ifstream& is, const unique_ptr<char>& buffer,
	int chunk_size, const char* delimiters) {
	if (!strchr(delimiters, *(buffer.get() + chunk_size - 1))) {
		for (int i = 1; i < 20; ++i) {
			char new_char = static_cast<char>(is.get());
			if (strchr(delimiters, new_char)) {
				break;
			}
			*(buffer.get() + chunk_size + i) = new_char;
		}
	}
}

void split(const string& input_string, const char* delimiters,
	vector<string>* output_tokens) {
	if (input_string.empty()) { return; }

	char* token = nullptr;
	char* input_string_cpy = new char[input_string.size() + 1];
	copy(input_string.begin(), input_string.end(), input_string_cpy);
	input_string_cpy[input_string.size()] = 0;
	token = strtok(input_string_cpy, delimiters);
	while (token != nullptr) {
		string word = strip_quotes(string(token));
		output_tokens->push_back(word);

		token = strtok(nullptr, delimiters);
	}
}

void split(char* input_string, const char* delimiters,
	vector<string>* output_tokens) {
	char* token = nullptr;
	token = strtok(input_string, delimiters);
	while (token != nullptr) {
		string word = strip_quotes(string(token));
		if (is_string_alpha(word) && word.size() > 1) {
			transform(word.begin(), word.end(), word.begin(), ::tolower);
			output_tokens->push_back(word);
		}
		token = strtok(nullptr, delimiters);
	}

}
string get_map_fullpath(const string& output_dir,int n) {
	int file_number = n;
	//cout << n << endl;
	string number_string = to_string(file_number);
	//cout << number_string << endl;
	if (number_string.length() < 6) {
		number_string = string(6 - number_string.length(), '0')
			.append(number_string);
	}
	char s[100]="";
	strcpy(s, ("file" + number_string + ".csv").c_str());
	DIR* directory = opendir(output_dir.data());
	if (directory) {
		dirent* dp;
		while ((dp = readdir(directory)) != nullptr) {
			if (strstr(dp->d_name, s) == dp->d_name) {
				file_number++;
			}
		}
		closedir(directory);
	}
	else if (ENOENT == errno) {
		mkdir(output_dir.data());
	}

	
	return string(output_dir) + "/file" + number_string + ".csv";
}
string get_new_map_fullpath(const string& output_dir) {
	int file_number = 0;
	DIR* directory = opendir(output_dir.data());
	if (directory) {
		dirent* dp;
		while ((dp = readdir(directory)) != nullptr) {
			if (strstr(dp->d_name, "file") == dp->d_name) {
				file_number++;
			}
		}
		closedir(directory);
	}
	else if (ENOENT == errno) {
		mkdir(output_dir.data());
	}

	string number_string = to_string(file_number);
	if (number_string.length() < 6) {
		number_string = string(6 - number_string.length(), '0')
			.append(number_string);
	}
	return string(output_dir) + "/file" + number_string + ".csv";
}
void process_file(const string& input_file_path, const string& output_file_path,
	int sorting_mode) {
	unique_ptr<map<string, int>> map_ptr = compose_words_map(input_file_path);
	switch (sorting_mode) {
	case SORT_BY_ALPHA:
		serialize_map(map_ptr.get(),
			output_file_path);
		break;
	case SORT_BY_FREQUENCY:
		vector<pair<string, int>> map_by_freq(map_ptr->begin(), map_ptr->end());
		map_ptr.reset();
		sort(map_by_freq.begin(), map_by_freq.end(),
			[](const pair<string, int>& l, const pair<string, int>& r) {
				return l.second < r.second;
			});
		serialize_map(&map_by_freq, output_file_path);
		break;
	}
}
void print_usage() {
	cout << "=========================wunderwaffel===================================\n";
	cout << "usage: fc [options] {-i input_file} {-m file_1 -m file_2 ..} output_path\n";
	cout << "  options:\n";
	cout << "    -i, --input\t\t files to be converted to frequency table\n";
	cout << "    -a, --alphabetical\t default\n";
	cout << "    -f, --frequency\n";
	cout << "    -m, --merge\t\t files, associated with this option,"
		" will be merged into output_file\n";
}
