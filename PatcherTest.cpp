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
	virtual void VMethod(Struct) { }

	void Method() { }
	void Method(int) { }
};

InjectHook_Constructor_Init(0x00000000, Struct, PATCH_CALL);
InjectHook_Constructor_Init(0x00000001, Struct, PATCH_CALL, PATCHER_ARG(int));
InjectHook_Destructor_Init(0x00000002, Struct, PATCH_CALL);
InjectHook_VirtualMethod_Init(0x00000003, Struct, VMethod, PATCH_CALL);
InjectHook_VirtualMethod_Init(0x00000004, Struct, VMethod, PATCH_CALL, PATCHER_ARG(int));
Patch_VirtualMethod_Init(0x00000006, Struct, VMethod);

int main()
{
	return 0;
}

STARTPATCHES
	InjectHook_Constructor(0x00000000, Struct);
	InjectHook_Constructor(0x00000001, Struct);
	InjectHook_Destructor(0x00000002, Struct);
	InjectHook_VirtualMethod(0x00000003, Struct, VMethod);
	InjectHook_VirtualMethod(0x00000004, Struct, VMethod);
	Patch_VirtualMethod(0x00000006, Struct, VMethod);

	InjectHook_Overload_Member(0x00000005, Struct, Method, PATCH_CALL, void, int);

	InjectHook_Overload(0x00000007, Method, PATCH_CALL, void, int);
ENDPATCHES
