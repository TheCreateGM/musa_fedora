/*
 * Load the Moore Threads Vulkan ICD and exercise a headless path:
 * instance, physical device, logical device, buffer, and host-visible
 * memory map. No swapchain and no display change.
 *
 * Run with VK_DRIVER_FILES and VK_ICD_FILENAMES set to
 * /etc/vulkan/icd.d/musaicdconf.json.
 *
 * Struct layouts are Vulkan 1.0. VKAPI_CALL is the System V ABI on Linux.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t VkFlags;
typedef uint32_t VkResult;
typedef uint64_t VkDeviceSize;
typedef void *VkInstance;
typedef void *VkPhysicalDevice;
typedef void *VkDevice;
typedef void *VkQueue;
typedef void *VkBuffer;
typedef void *VkDeviceMemory;

#define VK_SUCCESS 0
#define VK_STRUCTURE_TYPE_APPLICATION_INFO 0
#define VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO 1
#define VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO 2
#define VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO 3
#define VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO 5
#define VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO 12
#define VK_API_VERSION_1_0 0x00400000u
#define VK_SHARING_MODE_EXCLUSIVE 0
#define VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 0x00000020u
#define VK_QUEUE_COMPUTE_BIT 0x00000002u
#define VK_QUEUE_GRAPHICS_BIT 0x00000001u
#define VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 0x00000001u
#define VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 0x00000002u
#define VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 0x00000004u
#define MT_VENDOR 0x1ed5u

typedef struct VkApplicationInfo {
    uint32_t sType;
    const void *pNext;
    const char *pApplicationName;
    uint32_t applicationVersion;
    const char *pEngineName;
    uint32_t engineVersion;
    uint32_t apiVersion;
} VkApplicationInfo;

typedef struct VkInstanceCreateInfo {
    uint32_t sType;
    const void *pNext;
    VkFlags flags;
    const VkApplicationInfo *pApplicationInfo;
    uint32_t enabledLayerCount;
    const char *const *ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char *const *ppEnabledExtensionNames;
} VkInstanceCreateInfo;

typedef struct VkDeviceQueueCreateInfo {
    uint32_t sType;
    const void *pNext;
    VkFlags flags;
    uint32_t queueFamilyIndex;
    uint32_t queueCount;
    const float *pQueuePriorities;
} VkDeviceQueueCreateInfo;

typedef struct VkDeviceCreateInfo {
    uint32_t sType;
    const void *pNext;
    VkFlags flags;
    uint32_t queueCreateInfoCount;
    const VkDeviceQueueCreateInfo *pQueueCreateInfos;
    uint32_t enabledLayerCount;
    const char *const *ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char *const *ppEnabledExtensionNames;
    const void *pEnabledFeatures;
} VkDeviceCreateInfo;

typedef struct VkExtent3D {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} VkExtent3D;

typedef struct VkQueueFamilyProperties {
    VkFlags queueFlags;
    uint32_t queueCount;
    uint32_t timestampValidBits;
    VkExtent3D minImageTransferGranularity;
} VkQueueFamilyProperties;

typedef struct VkBufferCreateInfo {
    uint32_t sType;
    const void *pNext;
    VkFlags flags;
    VkDeviceSize size;
    VkFlags usage;
    uint32_t sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t *pQueueFamilyIndices;
} VkBufferCreateInfo;

typedef struct VkMemoryRequirements {
    VkDeviceSize size;
    VkDeviceSize alignment;
    uint32_t memoryTypeBits;
} VkMemoryRequirements;

typedef struct VkMemoryType {
    VkFlags propertyFlags;
    uint32_t heapIndex;
} VkMemoryType;

typedef struct VkMemoryHeap {
    VkDeviceSize size;
    VkFlags flags;
} VkMemoryHeap;

typedef struct VkPhysicalDeviceMemoryProperties {
    uint32_t memoryTypeCount;
    VkMemoryType memoryTypes[32];
    uint32_t memoryHeapCount;
    VkMemoryHeap memoryHeaps[16];
} VkPhysicalDeviceMemoryProperties;

typedef struct VkMemoryAllocateInfo {
    uint32_t sType;
    const void *pNext;
    VkDeviceSize allocationSize;
    uint32_t memoryTypeIndex;
} VkMemoryAllocateInfo;

struct vk_props_prefix {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    uint32_t deviceType;
    char deviceName[256];
};

typedef VkResult (*PFN_vkCreateInstance)(const VkInstanceCreateInfo *, const void *, VkInstance *);
typedef VkResult (*PFN_vkEnumeratePhysicalDevices)(VkInstance, uint32_t *, VkPhysicalDevice *);
typedef void (*PFN_vkGetPhysicalDeviceProperties)(VkPhysicalDevice, void *);
typedef void (*PFN_vkGetPhysicalDeviceQueueFamilyProperties)(VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties *);
typedef void (*PFN_vkGetPhysicalDeviceMemoryProperties)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties *);
typedef VkResult (*PFN_vkCreateDevice)(VkPhysicalDevice, const VkDeviceCreateInfo *, const void *, VkDevice *);
typedef VkResult (*PFN_vkCreateBuffer)(VkDevice, const VkBufferCreateInfo *, const void *, VkBuffer *);
typedef void (*PFN_vkGetBufferMemoryRequirements)(VkDevice, VkBuffer, VkMemoryRequirements *);
typedef VkResult (*PFN_vkAllocateMemory)(VkDevice, const VkMemoryAllocateInfo *, const void *, VkDeviceMemory *);
typedef VkResult (*PFN_vkBindBufferMemory)(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize);
typedef VkResult (*PFN_vkMapMemory)(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkFlags, void **);
typedef void (*PFN_vkUnmapMemory)(VkDevice, VkDeviceMemory);
typedef void (*PFN_vkDestroyBuffer)(VkDevice, VkBuffer, const void *);
typedef void (*PFN_vkFreeMemory)(VkDevice, VkDeviceMemory, const void *);
typedef void (*PFN_vkDestroyDevice)(VkDevice, const void *);
typedef void (*PFN_vkDestroyInstance)(VkInstance, const void *);

#define LOAD(name)                                                          \
    do {                                                                    \
        name = (PFN_##name)dlsym(lib, #name);                              \
        if (!name) {                                                        \
            fprintf(stderr, "missing %s\n", #name);                         \
            return 2;                                                       \
        }                                                                   \
    } while (0)

int main(void)
{
    void *lib;
    PFN_vkCreateInstance vkCreateInstance;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
    PFN_vkCreateDevice vkCreateDevice;
    PFN_vkCreateBuffer vkCreateBuffer;
    PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
    PFN_vkAllocateMemory vkAllocateMemory;
    PFN_vkBindBufferMemory vkBindBufferMemory;
    PFN_vkMapMemory vkMapMemory;
    PFN_vkUnmapMemory vkUnmapMemory;
    PFN_vkDestroyBuffer vkDestroyBuffer;
    PFN_vkFreeMemory vkFreeMemory;
    PFN_vkDestroyDevice vkDestroyDevice;
    PFN_vkDestroyInstance vkDestroyInstance;

    VkApplicationInfo app;
    VkInstanceCreateInfo ici;
    VkInstance instance = NULL;
    VkPhysicalDevice phys[8];
    uint32_t count = 0;
    uint32_t i;
    VkResult rc;
    int mt_index = -1;
    unsigned char props_buf[1024];
    VkQueueFamilyProperties families[16];
    uint32_t family_count = 0;
    int family = -1;
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci;
    VkDeviceCreateInfo dci;
    VkDevice device = NULL;
    VkBufferCreateInfo bci;
    VkBuffer buffer = NULL;
    VkMemoryRequirements req;
    VkPhysicalDeviceMemoryProperties memprops;
    VkMemoryAllocateInfo mai;
    VkDeviceMemory memory = NULL;
    int type_index = -1;
    uint32_t want;
    void *mapped = NULL;
    uint32_t *words;

    lib = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!lib) {
        fprintf(stderr, "dlopen libvulkan.so.1: %s\n", dlerror());
        return 2;
    }
    LOAD(vkCreateInstance);
    LOAD(vkEnumeratePhysicalDevices);
    LOAD(vkGetPhysicalDeviceProperties);
    LOAD(vkGetPhysicalDeviceQueueFamilyProperties);
    LOAD(vkGetPhysicalDeviceMemoryProperties);
    LOAD(vkCreateDevice);
    LOAD(vkCreateBuffer);
    LOAD(vkGetBufferMemoryRequirements);
    LOAD(vkAllocateMemory);
    LOAD(vkBindBufferMemory);
    LOAD(vkMapMemory);
    LOAD(vkUnmapMemory);
    LOAD(vkDestroyBuffer);
    LOAD(vkFreeMemory);
    LOAD(vkDestroyDevice);
    LOAD(vkDestroyInstance);

    memset(&app, 0, sizeof(app));
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "musa-fedora-vk-enum";
    app.apiVersion = VK_API_VERSION_1_0;
    memset(&ici, 0, sizeof(ici));
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo = &app;

    rc = vkCreateInstance(&ici, NULL, &instance);
    printf("vkCreateInstance=%d\n", (int)rc);
    if (rc != VK_SUCCESS || !instance)
        return 1;

    rc = vkEnumeratePhysicalDevices(instance, &count, NULL);
    printf("vkEnumeratePhysicalDevices_count=%u rc=%d\n", count, (int)rc);
    if (rc != VK_SUCCESS || count == 0)
        return 1;
    if (count > 8)
        count = 8;
    rc = vkEnumeratePhysicalDevices(instance, &count, phys);
    if (rc != VK_SUCCESS)
        return 1;

    for (i = 0; i < count; i++) {
        struct vk_props_prefix *p;
        memset(props_buf, 0, sizeof(props_buf));
        vkGetPhysicalDeviceProperties(phys[i], props_buf);
        p = (struct vk_props_prefix *)props_buf;
        p->deviceName[255] = '\0';
        printf("device%u vendor=0x%04x id=0x%04x type=%u name=%s\n",
               i, p->vendorID, p->deviceID, p->deviceType, p->deviceName);
        if (p->vendorID == MT_VENDOR && p->deviceID == 0x0102u)
            mt_index = (int)i;
    }
    if (mt_index < 0) {
        printf("VULKAN_ENUM: FAIL no MTT S30\n");
        return 1;
    }
    printf("VULKAN_ENUM: PASS\n");

    family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys[mt_index], &family_count, NULL);
    if (family_count > 16)
        family_count = 16;
    vkGetPhysicalDeviceQueueFamilyProperties(phys[mt_index], &family_count, families);
    for (i = 0; i < family_count; i++) {
        printf("queue_family%u flags=0x%x count=%u\n",
               i, families[i].queueFlags, families[i].queueCount);
        if (family < 0 && (families[i].queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)))
            family = (int)i;
    }
    if (family < 0) {
        printf("VULKAN_DEVICE: FAIL no graphics or compute queue\n");
        return 1;
    }

    memset(&qci, 0, sizeof(qci));
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = (uint32_t)family;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;
    memset(&dci, 0, sizeof(dci));
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;

    rc = vkCreateDevice(phys[mt_index], &dci, NULL, &device);
    printf("vkCreateDevice=%d family=%d\n", (int)rc, family);
    if (rc != VK_SUCCESS || !device) {
        printf("VULKAN_DEVICE: FAIL\n");
        return 1;
    }

    memset(&bci, 0, sizeof(bci));
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = 4096;
    bci.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    rc = vkCreateBuffer(device, &bci, NULL, &buffer);
    printf("vkCreateBuffer=%d\n", (int)rc);
    if (rc != VK_SUCCESS || !buffer) {
        printf("VULKAN_MEMORY: FAIL\n");
        vkDestroyDevice(device, NULL);
        return 1;
    }

    memset(&req, 0, sizeof(req));
    vkGetBufferMemoryRequirements(device, buffer, &req);
    printf("buffer_bytes=%llu alignment=%llu type_bits=0x%x\n",
           (unsigned long long)req.size, (unsigned long long)req.alignment, req.memoryTypeBits);

    memset(&memprops, 0, sizeof(memprops));
    vkGetPhysicalDeviceMemoryProperties(phys[mt_index], &memprops);
    want = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (i = 0; i < memprops.memoryTypeCount; i++) {
        if ((req.memoryTypeBits & (1u << i)) &&
            (memprops.memoryTypes[i].propertyFlags & want) == want) {
            type_index = (int)i;
            break;
        }
    }
    if (type_index < 0) {
        for (i = 0; i < memprops.memoryTypeCount; i++) {
            if ((req.memoryTypeBits & (1u << i)) &&
                (memprops.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
                type_index = (int)i;
                break;
            }
        }
    }
    if (type_index < 0) {
        printf("VULKAN_MEMORY: FAIL no host-visible type\n");
        vkDestroyBuffer(device, buffer, NULL);
        vkDestroyDevice(device, NULL);
        return 1;
    }
    printf("memory_type=%d flags=0x%x heap=%u\n", type_index,
           memprops.memoryTypes[type_index].propertyFlags,
           memprops.memoryTypes[type_index].heapIndex);

    memset(&mai, 0, sizeof(mai));
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = (uint32_t)type_index;
    rc = vkAllocateMemory(device, &mai, NULL, &memory);
    printf("vkAllocateMemory=%d\n", (int)rc);
    if (rc != VK_SUCCESS || !memory) {
        printf("VULKAN_MEMORY: FAIL\n");
        vkDestroyBuffer(device, buffer, NULL);
        vkDestroyDevice(device, NULL);
        return 1;
    }
    rc = vkBindBufferMemory(device, buffer, memory, 0);
    printf("vkBindBufferMemory=%d\n", (int)rc);
    if (rc != VK_SUCCESS) {
        printf("VULKAN_MEMORY: FAIL\n");
        vkFreeMemory(device, memory, NULL);
        vkDestroyBuffer(device, buffer, NULL);
        vkDestroyDevice(device, NULL);
        return 1;
    }
    rc = vkMapMemory(device, memory, 0, req.size, 0, &mapped);
    printf("vkMapMemory=%d\n", (int)rc);
    if (rc != VK_SUCCESS || !mapped) {
        printf("VULKAN_MEMORY: FAIL\n");
        vkFreeMemory(device, memory, NULL);
        vkDestroyBuffer(device, buffer, NULL);
        vkDestroyDevice(device, NULL);
        return 1;
    }
    words = (uint32_t *)mapped;
    words[0] = 0x5300102u;
    words[1] = 0x1ed5u;
    if (words[0] != 0x5300102u || words[1] != 0x1ed5u) {
        printf("VULKAN_MEMORY: FAIL readback\n");
        vkUnmapMemory(device, memory);
        vkFreeMemory(device, memory, NULL);
        vkDestroyBuffer(device, buffer, NULL);
        vkDestroyDevice(device, NULL);
        return 1;
    }
    vkUnmapMemory(device, memory);
    vkFreeMemory(device, memory, NULL);
    vkDestroyBuffer(device, buffer, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
    printf("VULKAN_MEMORY: PASS\n");
    return 0;
}
