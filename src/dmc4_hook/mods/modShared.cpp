extern "C" {
	static const float limitadjust = 0.0f;
};

namespace modShared {
	_declspec(naked) void limitadjust_patch(void)
	{
		_asm {
			fld dword ptr[limitadjust]
		}
	}
}