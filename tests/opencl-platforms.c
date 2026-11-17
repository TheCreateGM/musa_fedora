/*
 * List OpenCL platforms visible to the system ICD loader.
 *
 * The MTT S30 3.1.0 package ships OpenCL headers and no vendor ICD.
 * This probe records that fact; it does not invent a platform.
 */
#include <stdio.h>
#include <string.h>

#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#ifndef CL_PLATFORM_NOT_FOUND_KHR
#define CL_PLATFORM_NOT_FOUND_KHR -1001
#endif

static void platform_string(cl_platform_id id, cl_platform_info what, const char *label)
{
    char buf[512];
    size_t n = 0;
    cl_int rc = clGetPlatformInfo(id, what, sizeof(buf), buf, &n);
    if (rc != CL_SUCCESS) {
        printf("  %s: error %d\n", label, (int)rc);
        return;
    }
    buf[sizeof(buf) - 1] = '\0';
    printf("  %s: %s\n", label, buf);
}

int main(void)
{
    cl_platform_id ids[16];
    cl_uint count = 0;
    cl_uint i;
    cl_int rc;
    int saw_mt = 0;

    rc = clGetPlatformIDs(16, ids, &count);
    printf("clGetPlatformIDs=%d count=%u\n", (int)rc, count);
    if (rc == CL_PLATFORM_NOT_FOUND_KHR || rc == -1001) {
        printf("OPENCL_MT: ABSENT no platforms\n");
        return 0;
    }
    if (rc != CL_SUCCESS)
        return 1;

    for (i = 0; i < count; i++) {
        char vendor[256];
        printf("platform%u\n", i);
        platform_string(ids[i], CL_PLATFORM_NAME, "name");
        platform_string(ids[i], CL_PLATFORM_VENDOR, "vendor");
        platform_string(ids[i], CL_PLATFORM_VERSION, "version");
        vendor[0] = '\0';
        clGetPlatformInfo(ids[i], CL_PLATFORM_VENDOR, sizeof(vendor), vendor, NULL);
        if (strstr(vendor, "Moore") || strstr(vendor, "MUSA") || strstr(vendor, "MooreThreads"))
            saw_mt = 1;
    }
    if (saw_mt)
        printf("OPENCL_MT: PRESENT\n");
    else
        printf("OPENCL_MT: ABSENT\n");
    return 0;
}
