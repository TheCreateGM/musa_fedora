// CUDA driver API (cuda.h) source, unmodified.
#include <cuda.h>
#include <stdio.h>
int main(void)
{
    CUresult r = cuInit(0);
    if (r != CUDA_SUCCESS) { printf("DRV_FAIL init=%d\n", (int)r); return 1; }
    int n = 0;
    if ((r = cuDeviceGetCount(&n)) != CUDA_SUCCESS || n < 1) {
        printf("DRV_FAIL count=%d\n", (int)r); return 1;
    }
    CUdevice dev;
    if ((r = cuDeviceGet(&dev, 0)) != CUDA_SUCCESS) { printf("DRV_FAIL get=%d\n", (int)r); return 1; }
    char name[128] = {0};
    cuDeviceGetName(name, sizeof name, dev);
    int major = 0, minor = 0, sms = 0;
    cuDeviceGetAttribute(&major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dev);
    cuDeviceGetAttribute(&minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dev);
    cuDeviceGetAttribute(&sms, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, dev);
    printf("DRV name=%s cc=%d.%d sms=%d\n", name, major, minor, sms);
    int ver = 0; cuDriverGetVersion(&ver);
    printf("DRV driverVersion=%d\n", ver);
    CUcontext ctx;
    if ((r = cuCtxCreate(&ctx, 0, dev)) != CUDA_SUCCESS) { printf("DRV_FAIL ctx=%d\n", (int)r); return 1; }
    CUdeviceptr p;
    if ((r = cuMemAlloc(&p, 1 << 20)) != CUDA_SUCCESS) { printf("DRV_FAIL alloc=%d\n", (int)r); return 1; }
    int val = 12345;
    cuMemcpyHtoD(p, &val, sizeof val);
    int back = 0;
    cuMemcpyDtoH(&back, p, sizeof back);
    size_t freeb = 0, totalb = 0;
    cuMemGetInfo(&freeb, &totalb);
    printf("DRV roundtrip=%d free=%zuMiB total=%zuMiB\n", back, freeb >> 20, totalb >> 20);
    cuMemFree(p); cuCtxDestroy(ctx);
    int ok = back == val;
    printf(ok ? "DRV_STATUS: PASS\n" : "DRV_STATUS: FAIL\n");
    return ok ? 0 : 1;
}
