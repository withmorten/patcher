#include "patcher.h"

void Method()
{
}

void Method(int)
{
}

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
Patch_VirtualMethod_Init(Struct, VMethod, 0x00000006);

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
	Patch_VirtualMethod(Struct, VMethod, 0x00000006);

	// InjectHook(0x00000005, (void (Struct::*)(int))&Struct::Method);
	InjectHook_Overload_Member(0x00000005, Struct, Method, HOOK_CALL, void, int);

	InjectHook_Overload(0x00000007, Method, HOOK_CALL, void, int);
ENDPATCHES
