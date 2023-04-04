#include <tuple>

namespace utility {
	void* DecompressFileFromMemory(const void* compressed_file_data, int compressed_file_size);
	std::tuple<void*, const unsigned int>
		DecompressFileFromMemoryWithSize(const void* compressed_file_data, int compressed_file_size);
	void* DecompressFileFromMemoryBase85(const char* compressed_file_data_base85);
}