#include <test_common.h>

//
// standard hoook
//

static int before_standard_hook(const char* str)
{
    spdlog::info("before_standard_hook {}", str);
    return 1;
}

static int after_standard_hook(const char* str)
{
    spdlog::info("after_standard_hook : {}", str);
    return 2;
}

TEST(wxbox_hook, standard_hook)
{
    wb_hook::HookMetaInfo hookMetaInfo;

    EXPECT_EQ(true, wb_hook::InProcessHook(before_standard_hook, after_standard_hook));
    EXPECT_EQ(2, before_standard_hook("hello hook"));
    EXPECT_EQ(true, wb_hook::ObtainHookMetaInfo(before_standard_hook, hookMetaInfo));
    EXPECT_EQ(true, wb_hook::RevokeInProcessHook(before_standard_hook));
    EXPECT_EQ(1, before_standard_hook("hello hook"));
}

//
// relocate intercept hook
//

int before_relocate_intercept_hook(const char* str)
{
    spdlog::info("before_relocate_intercept_hook : {}", str);
    return 1;
}

void after_relocate_intercept_hook(const char* str)
{
    spdlog::info("after_relocate_intercept_hook : {}", str);
}

BEGIN_NAKED_STD_FUNCTION(relocate_intercept_hook_stub)
{
    spdlog::info("relocate_intercept_hook_stub");

    __asm {
		push edx
		mov edx, esp
		add edx, 4
		add edx, 8
		mov edx, [edx]
		push edx
		call after_relocate_intercept_hook
		add esp, 4
		pop edx
		ret
    }
}
END_NAKED_STD_FUNCTION(relocate_intercept_hook_stub)

TEST(wxbox_hook, relocate_intercept_hook)
{
	wb_hook::HookMetaInfo hookMetaInfo;

    EXPECT_EQ(true, wb_hook::InProcessIntercept(before_relocate_intercept_hook, relocate_intercept_hook_stub));
    EXPECT_EQ(1, before_relocate_intercept_hook("hello relocate_intercept_hook"));
    EXPECT_EQ(true, wb_hook::ObtainHookMetaInfo(before_relocate_intercept_hook, hookMetaInfo));
    EXPECT_EQ(true, wb_hook::RevokeInProcessHook(before_relocate_intercept_hook));
    EXPECT_EQ(1, before_relocate_intercept_hook("hello relocate_intercept_hook"));
}

//
// intercept hook
//

static int before_intercept_hook(const char* str)
{
    spdlog::info("before_intercept_hook : {}", str);
    return 1;
}

static void after_intercept_hook(const char* str)
{
    spdlog::info("after_intercept_hook : {}", str);
}

BEGIN_NAKED_STD_FUNCTION(intercept_hook_stub)
{
    spdlog::info("intercept_hook_stub");

    __asm {
		push edx
		mov edx, esp
		add edx, 4
		add edx, 8
		mov edx, [edx]
		push edx
		call after_intercept_hook
		add esp, 4
		pop edx
		ret
    }
}
END_NAKED_STD_FUNCTION(intercept_hook_stub)

TEST(wxbox_hook, intercept_hook)
{
    wb_hook::HookMetaInfo hookMetaInfo;

	void* actualEntry = wb_traits::GetActualEntryAddress(before_relocate_intercept_hook);
    EXPECT_NE(nullptr, actualEntry);

    EXPECT_EQ(true, wb_hook::InProcessIntercept(actualEntry, intercept_hook_stub));
    EXPECT_EQ(1, before_relocate_intercept_hook("hello intercept_hook"));
    EXPECT_EQ(true, wb_hook::ObtainHookMetaInfo(actualEntry, hookMetaInfo));
    EXPECT_EQ(true, wb_hook::RevokeInProcessHook(actualEntry));
    EXPECT_EQ(1, before_relocate_intercept_hook("hello intercept_hook"));
}