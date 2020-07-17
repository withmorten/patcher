#include "patcher.h"

struct Struct
{
	Struct() { }
	Struct(int) { }
	~Struct() { }

	virtual void VMethod() { }
	virtual void VMethod(int) { }

	void Method() { }
	void Method(int) { }
};

InjectHook_Constructor_Init(Struct, 0x00000000, HOOK_CALL);
InjectHook_Constructor_Init(Struct, 0x00000001, HOOK_CALL, HOOK_ARG(int));
InjectHook_Destructor_Init(Struct, 0x00000002, HOOK_CALL);
InjectHook_VirtualMethod_Init(Struct, VMethod, 0x00000003, HOOK_CALL);
InjectHook_VirtualMethod_Init(Struct, VMethod, 0x00000004, HOOK_CALL, HOOK_ARG(int));

int main()
{
	return 0;
}

STARTPATCHES
	InjectHook_Constructor(Struct, 0x00000000);
	InjectHook_Constructor(Struct, 0x00000001);
	InjectHook_Destructor(Struct, 0x00000002);
	InjectHook_VirtualMethod(Struct, VMethod, 0x00000003);
	InjectHook_VirtualMethod(Struct, VMethod, 0x00000004);

	InjectHook(0x00000005, (void (Struct::*)(int))&Struct::Method);
ENDPATCHES
