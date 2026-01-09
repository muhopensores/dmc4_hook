#include <vector>
#include <utility>
#include <cstdint>

extern std::vector<std::pair<uint16_t, const char*>> buttonPairs;

extern const char* dante_attack_names[109];
extern const char* directionMappingNames[];
extern const char* attackNames[];
extern const char* weaponNames[];
extern const char* styleNames[];
extern const char* aerialNames[];
extern const char* restrictNames[];
extern const char* dtNames[];

size_t dante_attack_names_arraysize();
size_t directionMappingNamesArraySize();
size_t attackNamesArraySize();
size_t weaponNamesArraySize();
size_t styleNamesArraySize();
size_t aerialNamesArraySize();
size_t restrictNamesArraySize();
size_t dtNamesArraySize();
