#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <exception>

class Tools {
public :
	static std::vector<std::string> getAllFilesInFolder(const std::string& folderPath) {
		std::vector<std::string> filePaths;
		try {
			for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
				if (entry.is_regular_file()) {
					filePaths.push_back(entry.path().string());
				}
			}
		}
		catch (std::filesystem::filesystem_error& e) {
			std::cerr << "Filesystem error: " << e.what() << '\n';
		}
		catch (std::exception& e) {
			std::cerr << "General error: " << e.what() << '\n';
		}
		return filePaths;
	}

	static std::string getFileContents(const char* filename)
	{
		std::ifstream in(filename, std::ios::in | std::ios::binary);
		if (in)
		{
			std::string contents;
			in.seekg(0, std::ios::end);
			contents.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&contents[0], contents.size());
			in.close();
			return(contents);
		}
		throw std::exception(__func__);
	}
};