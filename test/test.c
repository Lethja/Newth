#pragma region CMocka Headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>

#pragma endregion

#pragma region Test Group Headers

#include "mock.h"
#include "platform.h"
#include "sockbufr.h"
#include "request.h"

#pragma endregion

int main(void) {
    int r;

    if ((r = cmocka_run_group_tests(mockTest, NULL, NULL))) return r;
    if ((r = cmocka_run_group_tests(platformTest, NULL, NULL))) return r;
    if ((r = cmocka_run_group_tests(socketTest, NULL, NULL))) return r;
    if ((r = cmocka_run_group_tests(requestTest, NULL, NULL))) return r;

    return 0;
}
