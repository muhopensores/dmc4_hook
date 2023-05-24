#include <tuple>

namespace utility {
	void* decompress_file_from_memory(const void* compressed_file_data, int compressed_file_size);
	std::tuple<void*, const unsigned int>
		decompress_file_from_memory_with_size(const void* compressed_file_data, int compressed_file_size);
	std::tuple<void*, const unsigned int>
		decompress_file_from_memory_base85(const char* compressed_file_data_base85);
}