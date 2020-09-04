#pragma once

#include <new.h>
#include <string.h>

class StaticPatcher
{
private:
	static StaticPatcher *ms_head;

	using Patcher = void(*)();

	Patcher m_func;
	StaticPatcher *m_next;

	void Run() { m_func(); }

public:

	static void Apply();

	StaticPatcher(Patcher func);
};

#define STARTPATCHES	static StaticPatcher Patcher([]() {
#define ENDPATCHES		});

#define NAKED __declspec(naked)
#define WRAPPER NAKED
#define DEPRECATED __declspec(deprecated)
#define EAXJMP(a) { _asm mov eax, a _asm jmp eax }
#define VARJMP(a) { _asm jmp a }
#define WRAPARG(a) UNREFERENCED_PARAMETER(a)

#define RET(n) { __asm push n __asm retn }
#define ASM(name) void __declspec(naked) name(void)

#define asm(...) __asm { __VA_ARGS__ }

#define NOVMT __declspec(novtable)
#define SETVMT(a) *((uintptr *)this) = (uintptr)a

#define FIELD(type, var, offset) *(type *)((byte *)var + offset)

#pragma warning(disable:4731) // -- suppress C4731:"frame pointer register 'ebp' modified by inline assembly code"

#define XCALL(uAddr)			\
	__asm { mov esp, ebp	}	\
	__asm { pop ebp			}	\
	__asm { mov eax, uAddr	}	\
	__asm { jmp eax			}

int Unprotect_internal(void *address, size_t size);
int Protect_internal(void *address, size_t);

template<typename T> __forceinline void Patch(uintptr_t address, T value)
{
	Unprotect_internal((void *)address, sizeof(T));
	*(T *)address = value;
	Protect_internal((void *)address, sizeof(T));
}

__forceinline void PatchByte(uintptr_t address, unsigned char value)
{
	Patch(address, value);
}

__forceinline void PatchBytes(uintptr_t address, void *value, size_t size)
{
	Unprotect_internal((void *)address, size);
	memcpy((void *)address, value, size);
	Protect_internal((void *)address, size);
}

__forceinline void ReadBytes(uintptr_t address, void *out, size_t size)
{
	memcpy(out, (void *)address, size);
}

__forceinline void SetBytes(uintptr_t address, int value, size_t size)
{
	Unprotect_internal((void *)address, size);
	memset((void *)address, value, size);
	Protect_internal((void *)address, size);
}

__forceinline void Nop(uintptr_t address, size_t count = 1)
{
	SetBytes(address, 0x90, count);
}

__forceinline void NopTo(uintptr_t address, uintptr_t to)
{
	Nop(address, to - address);
}

enum
{
	PATCH_EXISTING,
	PATCH_CALL,
	PATCH_JUMP,
	HOOK_SIZE = 5,
};

template<typename T> __forceinline void InjectHook(uintptr_t address, T hook, int type = PATCH_EXISTING)
{
	switch (type)
	{
	case PATCH_EXISTING:
		Unprotect_internal((void *)(address + 1), HOOK_SIZE - 1);

		break;

	case PATCH_CALL:
		Unprotect_internal((void *)address, HOOK_SIZE);
		*(unsigned char *)address = 0xE8;

		break;

	case PATCH_JUMP:
		Unprotect_internal((void *)address, HOOK_SIZE);
		*(unsigned char *)address = 0xE9;

		break;
	}

	*(ptrdiff_t *)(address + 1) = (uintptr_t)(void *&)hook - address - HOOK_SIZE;

	switch (type)
	{
		case PATCH_EXISTING:
			Protect_internal((void *)(address + 1), HOOK_SIZE - 1);

			break;

		case PATCH_CALL:
		case PATCH_JUMP:
			Protect_internal((void *)address, HOOK_SIZE);

			break;
	}
}

__forceinline void PatchJump(uintptr_t address, uintptr_t to)
{
	InjectHook(address, to, PATCH_JUMP);
}

__forceinline void ExtractCall(void *dst, uintptr_t a)
{
	*(uintptr_t *)dst = (uintptr_t)(*(uintptr_t *)(a + 1) + a + 5);
}

template<typename T> __forceinline void InterceptCall(void *dst, T func, uintptr_t a)
{
	ExtractCall(dst, a);
	InjectHook(a, func);
}

template<typename T> __forceinline void InterceptVmethod(void *dst, T func, uintptr_t a)
{
	*(uintptr_t *)dst = *(uintptr_t *)a;
	Patch(a, func);
}

#define HOOK_ARG(type) (type)0

#define HOOK_CALL PATCH_CALL
#define HOOK_JUMP PATCH_JUMP

#define InjectHook_Constructor_Init(classname, offset, type, ...) \
NAKED void classname ## _Constructor_ ## offset (classname *_) \
{ \
	__asm { mov eax, end } \
	::new (_) classname(__VA_ARGS__); \
	__asm { end: } \
} \
\
void InjectHook_ ## classname ## _Constructor_ ## offset (void) \
{ \
	uintptr_t calladdr = (*(uintptr_t *)((uintptr_t)& classname ## _Constructor_ ## offset + 1)) - 5; \
	uintptr_t ctoraddr; \
	ExtractCall(&ctoraddr, calladdr); \
	InjectHook(offset, ctoraddr, type); \
\
}

#define InjectHook_Constructor(classname, offset) InjectHook_ ## classname ## _Constructor_ ## offset()

#define InjectHook_Destructor_Init(classname, offset, type) \
static NAKED void classname ## _Destructor_ ## offset (classname *_) \
{ \
	__asm { mov eax, end } \
	_->~classname(); \
	__asm { end: } \
} \
\
static uintptr_t classname ## _Destructor_ ## offset ## _Address; \
\
static ASM(classname ## _Destructor_ ## offset ## _HOOK_CALL) \
{ \
	__asm { push 0 } \
	__asm { push offset + HOOK_SIZE } \
	__asm { jmp classname ## _Destructor_ ## offset ## _Address } \
} \
\
static ASM(classname ## _Destructor_ ## offset ## _HOOK_JUMP) \
{ \
	__asm { pop eax } \
	__asm { push 0 } \
	__asm { push eax } \
	__asm { jmp classname ## _Destructor_ ## offset ## _Address } \
} \
\
static void InjectHook_ ## classname ## _Destructor_ ## offset (void) \
{ \
	uintptr_t calladdr = (*(uintptr_t *)((uintptr_t)& classname ## _Destructor_ ## offset + 1)) - 5; \
	uintptr_t dtoraddr; \
	ExtractCall(&dtoraddr, calladdr); \
	classname ## _Destructor_ ## offset ## _Address = dtoraddr; \
	InjectHook(offset, &classname ## _Destructor_ ## offset ## _ ## type, PATCH_JUMP); \
\
}

#define InjectHook_Destructor(classname, offset) InjectHook_ ## classname ## _Destructor_ ## offset()

#define InjectHook_VirtualMethod_Init(classname, funcname, offset, type, ...) \
static NAKED void classname ## __ ## funcname ## _ ## offset (classname *_) \
{ \
	__asm { mov eax, end } \
	_->classname::funcname(__VA_ARGS__); \
	__asm { end: } \
} \
\
static void InjectHook_ ## classname ## __ ## funcname ## _ ## offset (void) \
{ \
	uintptr_t calladdr = (*(uintptr_t *)((uintptr_t)& classname ## __ ## funcname ## _ ## offset + 1)) - 5; \
	uintptr_t vmthdaddr; \
	ExtractCall(&vmthdaddr, calladdr); \
	InjectHook(offset, vmthdaddr, type); \
\
}

#define InjectHook_VirtualMethod(classname, funcname, offset) InjectHook_ ## classname ## __ ## funcname ## _ ## offset()

#define Patch_VirtualMethod_Init(classname, funcname, offset, ...) \
static NAKED void classname ## __ ## funcname ## _ ## offset (classname *_) \
{ \
	__asm { mov eax, end } \
	_->classname::funcname(__VA_ARGS__); \
	__asm { end: } \
} \
\
static void Patch_ ## classname ## __ ## funcname ## _ ## offset (void) \
{ \
	uintptr_t calladdr = (*(uintptr_t *)((uintptr_t)& classname ## __ ## funcname ## _ ## offset + 1)) - 5; \
	uintptr_t vmthdaddr; \
	ExtractCall(&vmthdaddr, calladdr); \
	Patch(offset, vmthdaddr); \
\
}

#define Patch_VirtualMethod(classname, funcname, offset) Patch_ ## classname ## __ ## funcname ## _ ## offset()
