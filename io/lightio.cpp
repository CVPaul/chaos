// lightio.cpp: 定义应用程序的入口点。
//

#include <regex>
#include <iomanip>
#include <fstream>
#include <streambuf>
#include "lightio.h"
#include <google/protobuf/util/json_util.h>


using namespace std;

namespace pb = google::protobuf;

void lower(std::string& word) {
	std::transform(word.begin(), word.end(), word.begin(),
		[](unsigned char c) {return std::tolower(c); });
}

std::vector<std::string> split(const std::string& text, const std::string& delimiter) {
	std::regex sep(delimiter);
	return std::vector<std::string>(
		std::sregex_token_iterator(text.begin(), text.end(), sep, -1),
		std::sregex_token_iterator());
}


dat::TickData io::line2tick(const std::string& line) {
	dat::TickData t;
	auto tokens = split(line, "\\\|");
	auto reflec = t.GetReflection();
	auto descriptor = t.GetDescriptor();
	for (auto&& token : tokens) {
		auto kv = split(token, "=");
		if (kv.size() == 1)
			kv.push_back("");
		if(kv.size() != 2) {
			std::cerr << "invalid filed:" << token << std::endl;
			continue;
		}
		if (kv[0] == "instrument_id") {
			kv[0] = "InstrumentID";
		}
		/*else {
			lower(kv[0]);
		}*/
		auto field = descriptor->FindFieldByName(kv[0]);
		switch (field->cpp_type()) {
		case pb::FieldDescriptor::CPPTYPE_INT32:
			reflec->SetInt32(&t, field, std::stoi(kv[1]));
			break;
		case pb::FieldDescriptor::CPPTYPE_FLOAT:
			if (kv[1].find("+308") != std::string::npos)
				reflec->SetFloat(&t, field, 0);
			else
				reflec->SetFloat(&t, field, std::stof(kv[1]));
			break;
		case pb::FieldDescriptor::CPPTYPE_DOUBLE:
			if (kv[1].find("+308") != std::string::npos)
				reflec->SetDouble(&t, field, 0);
			else
				reflec->SetDouble(&t, field, std::stod(kv[1]));
			break;
		case pb::FieldDescriptor::CPPTYPE_STRING:
			reflec->SetString(&t, field, kv[1]);
			break;
		default:
			std::cerr << "unknow new type:" << field->cpp_type() << std::endl;
		}
	}
	return t;
}


int io::bin2tick(const std::string& filename, std::list<dat::TickData>& ticks) {
	int count(0);
	std::ifstream fin(filename);
	dat::TickData tick;
	while (tick.ParseFromIstream(&fin)) {
		ticks.push_back(tick);
		count += 1;
	}
	return count;
}


int io::log2tick(const std::string& filename, std::list<dat::TickData>& ticks) {
	int count(0);
	std::ifstream fin(filename);
	std::string line, mark("PUBLIC_MARKET_DATA|");
	// std::string content((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
	// auto mline = split(content, "\n");
	while (getline(fin, line)) {
	// for(auto&& line:mline){
		size_t pos = line.find(mark);
		if (pos == std::string::npos)
			continue;
		ticks.push_back(line2tick(line.substr(pos + mark.size())));
		count += 1;
		if (count % 1000 == 0)
			std::cout << "deal with " << count << " items." << std::endl;
	}
	return count;
}


int io::tick2bin(const std::list<dat::TickData>& ticks, const std::string& filename) {
	std::ofstream fout(filename);
	for (auto&& tick : ticks) {
		tick.SerializeToOstream(&fout);
	}
	return ticks.size();
}


int io::log2bin(const std::string& flog, const std::string& fbin) {
	int count(0);
	std::ifstream fin(flog);
	std::ofstream fout(fbin, ios::binary);
	std::string line, mark("PUBLIC_MARKET_DATA|");
	while (getline(fin, line)) {
		size_t pos = line.find(mark);
		if (pos == std::string::npos)
			continue;
		auto tick = line2tick(line.substr(pos + mark.size()));
		tick.SerializeToOstream(&fout);
		count += 1;
		if (count % 1000 == 0) {
			std::cout << "dealed with " << count << " items." << std::endl;
			pb::util::MessageToJsonString(tick, &line);
			std::cout << "data examaple:" << line << std::endl;
		}
	}
	return count;
}


int main() {
	cout << "start parsing..." << endl;
	std::list<dat::TickData> ticks;
	io::log2bin(
		"D:\\WorkSpace\\quant\\data\\output\\logs\\ogre.20211221.log",
		"D:\\WorkSpace\\quant\\data\\output\\logs\\ogre.20211221.bin");
	return 0;
}
