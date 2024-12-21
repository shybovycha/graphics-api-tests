#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#define GLFW_EXPOSE_NATIVE_COCOA
//#include <GLFW/glfw3native.h>

//#define VK_USE_PLATFORM_MACOS_MVK
//#include <vulkan/vulkan.h>

#include <vector>
#include <iostream>

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static int findGraphicsQueueFamily(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;

    std::cout << "[findGraphicsQueueFamily] Get queue family count..." << std::endl;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

    std::cout << "[findGraphicsQueueFamily] Get queue families..." << std::endl;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // bool hasGraphicsQueueFamily = queueFamilies.has([](auto& queueFamily) { return queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT; });

    std::cout << "[findGraphicsQueueFamily] Search for queue family..." << std::endl;

    for (auto i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            std::cout << "[findGraphicsQueueFamily] Found queue family at [" << i << "]" << std::endl;

            return i;
        }
    }

    return -1;
}

static bool isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // on OSX this will always be false regardless of the real support for geometry shaders
#ifndef __APPLE__
    if (!deviceFeatures.geometryShader) {
        std::cerr << "[isDeviceSuitable] Device does not support geometry shaders" << std::endl;
        return false;
    }
#endif

    bool hasQueue = findGraphicsQueueFamily(device) > -1;

    if (!hasQueue) {
        std::cerr << "[isDeviceSuitable] Device does not have a compatible queue" << std::endl;
        return false;
    }

    // deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ?

    return true;
}

int main() {
    std::cout << "Initializing GLFW..." << std::endl;

    if (!glfwInit()) {
        std::cerr << "Could not load GLTF" << std::endl;
        return 1;
    }

    const auto WINDOW_WIDTH = 1024;
    const auto WINDOW_HEIGHT = 768;

    std::cout << "Creating GLFW window..." << std::endl;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan with GLFW", NULL, NULL);

    if (!window) {
        std::cerr << "Could not create GLFW window" << std::endl;
        return 1;
    }

    std::cout << "Initializing Vulkan..." << std::endl;

    // initialize Vulkan
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;

    const char **glfwExtensions;

    std::cout << "Obtaining required Vulkan extensions via GLFW..." << std::endl;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> requiredExtensions;

    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        requiredExtensions.emplace_back(glfwExtensions[i]);
    }

#ifdef __APPLE__
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    createInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    createInfo.enabledLayerCount = 0;

    std::cout << "Creating Vulkan instance..." << std::endl;

    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    if (result != VK_SUCCESS) {
        if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
            std::cerr << "Incompatible driver" << std::endl;
            return 1;
        }

        std::cerr << "Failed to create Vulkan instance: " << result << std::endl;
        return 1;
    }

    // physical device
    std::cout << "Obtaining physical devices..." << std::endl;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        std::cerr << "Failed to find GPUs with Vulkan support " << std::endl;
        return 1;
    }

    std::cout << "Picking a physical device (out of " << deviceCount << ")..." << std::endl;

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        std::cerr << "Failed to find a suitable GPU" << std::endl;
        return 1;
    }

    // logical device
    auto queueFamilyIndex = findGraphicsQueueFamily(physicalDevice);

    if (queueFamilyIndex < 0) {
        std::cerr << "Failed to obtain compatible queue" << std::endl;
        return 1;
    }

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    deviceCreateInfo.enabledExtensionCount = 0;
    deviceCreateInfo.enabledLayerCount = 0;

    VkDevice device;

    std::cout << "Creating logical device..." << std::endl;

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        std::cerr << "Failed to create logical device" << std::endl;
        return 1;
    }

    std::cout << "Creating queue..." << std::endl;

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &graphicsQueue);

    // window surface

    std::cout << "Creating window surface..." << std::endl;

    VkSurfaceKHR surface;

    if (glfwCreateWindowSurface(instance, window, NULL, &surface)) {
        std::cerr << "Failed to create GLFW window surface" << std::endl;
        return 1;
    }

//    id windowHandle = glfwGetCocoaWindow(window);
//    id viewHandle = getViewFromNSWindowPointer(windowHandle);
//
//    VkMacOSSurfaceCreateInfoMVK createInfo = {};
//    createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
//    createInfo.pNext = nullptr;
//    createInfo.flags = 0;
//    createInfo.pView = viewHandle;
//
//    PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK;
//    vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)vkGetInstanceProcAddr(instance, "vkCreateMacOSSurfaceMVK");
//
//    if (!vkCreateMacOSSurfaceMVK) {
//        std::cerr << "Unabled to get pointer to function: vkCreateMacOSSurfaceMVK" << std::endl;
//        return 1;
//    }
//
//    if (vkCreateMacOSSurfaceMVK(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
//        std::cerr << "Failed to create surface" << std::endl;
//        return 1;
//    }
    // proceed to main setup

    std::cout << "Running main loop..." << std::endl;

    glfwSetKeyCallback(window, key_callback);

    bool isRunning = true;

    while (isRunning) {
        if (glfwWindowShouldClose(window)) {
            isRunning = false;
        }

        double time = glfwGetTime();

        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Destroying logical device..." << std::endl;

    vkDestroyDevice(device, nullptr);

    std::cout << "Destroying Vulkan instance..." << std::endl;

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
