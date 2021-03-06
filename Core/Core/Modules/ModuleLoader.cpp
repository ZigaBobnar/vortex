#include <Core/Modules/ModuleLoader.h>
#include <Core/Logging.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace Vortex::Core::Modules {

    ModuleLoader* ModuleLoader::loader() {
        if (_loader == nullptr) {
#ifdef WIN32
            _loader = new WindowsModuleLoader();
#else
            _loader = new LinuxModuleLoader();
#endif
        }

        return _loader;
    }

    ModuleLoader* ModuleLoader::_loader = nullptr;


    Module* DummyModuleLoader::load_module(const std::string& module_name) {
        return nullptr;
    }

    Module* DummyModuleLoader::get_module(const std::string& module_name) {
        return nullptr;
    }

    bool DummyModuleLoader::module_loaded(const std::string& module_name) {
        return false;
    }


#ifdef WIN32

    struct ModuleInstance {
        HINSTANCE lib_handle;
        Module* module;
    };

    WindowsModuleLoader::~WindowsModuleLoader() {
        for (const auto& it : _loaded_modules) {
            delete it.second->module;

            FreeLibrary(it.second->lib_handle);

            delete it.second;
        }
    }

    Module* WindowsModuleLoader::load_module(const std::string& module_name) {
        std::string module_path = ".\\" + module_name + ".dll";

        WIN32_FIND_DATA file_find_data;
        HANDLE file_handle = FindFirstFile(module_path.c_str(), &file_find_data);
        if (file_handle == (void*)ERROR_INVALID_HANDLE ||
            file_handle == (void*)ERROR_FILE_NOT_FOUND) {
            VORTEX_ERROR("Module file not found {0}", module_path);

            return nullptr;
        }

        HINSTANCE lib_handle = LoadLibrary(module_path.c_str());
        if (!lib_handle) {
            VORTEX_ERROR("Unable to load module {0}", module_path);

            return nullptr;
        }

        typedef const char* (__cdecl* ModuleNameProc)(void);
        typedef Module* (__cdecl* ModuleProc)(void);

        ModuleNameProc module_name_func = (ModuleNameProc)GetProcAddress(lib_handle, "get_vortex_module_name");
        ModuleProc get_module_func = (ModuleProc)GetProcAddress(lib_handle, "get_vortex_module");

        if (module_name_func == nullptr) {
            VORTEX_ERROR("Unable to load module. Get module name function (get_vortex_module_name) not found");

            return nullptr;
        }

        if (get_module_func == nullptr) {
            VORTEX_ERROR("Unable to load module '{0}'. Get module instance function (get_vortex_module) not found", module_name_func());

            return nullptr;
        }

        Module* module_ptr = get_module_func();
        if (module_ptr == nullptr) {
            VORTEX_ERROR("Unable to load module '{0}'. Get module instance function returned null pointer", module_name_func());

            return nullptr;
        }

        _loaded_modules[module_name] = new ModuleInstance{
            lib_handle,
            module_ptr,
        };

        VORTEX_TRACE("Module '{0}' loaded", module_name);

        return module_ptr;
    }

    Module* WindowsModuleLoader::get_module(const std::string& module_name) {
        const auto& it = _loaded_modules.find(module_name);
        if (it != _loaded_modules.end()) {
            return it->second->module;
        }

        return nullptr;
    }

    bool WindowsModuleLoader::module_loaded(const std::string& module_name) {
        return _loaded_modules.find(module_name) != _loaded_modules.end();
    }

    #else

    struct ModuleInstance {
        void* lib_handle;
        Module* module;
    };

    LinuxModuleLoader::~LinuxModuleLoader() {
        for (const auto& it : _loaded_modules) {
            delete it.second->module;

            dlclose(it.second->lib_handle);

            delete it.second;
        }
    }

    Module* LinuxModuleLoader::load_module(const std::string& module_name) {
        std::string module_path = "./lib" + module_name + ".so";

        void* lib_handle = dlopen(module_path.c_str(), RTLD_NOW);
        if (!lib_handle) {
            VORTEX_ERROR("Unable to load module '{0}' from '{1}'", module_name, module_path);

            return nullptr;
        }

        typedef const char* (*ModuleNameProc)(void);
        typedef Module* (*ModuleProc)(void);

        ModuleNameProc module_name_func = (ModuleNameProc)dlsym(lib_handle, "get_vortex_module_name");
        const char* error = dlerror();
        if (error || module_name_func == nullptr) {
            VORTEX_ERROR("Unable to load module '{0}'. Get module name function (get_vortex_module_name) not found. Error: {1}", module_name, error);

            return nullptr;
        }
        ModuleProc get_module_func = (ModuleProc)dlsym(lib_handle, "get_vortex_module");
        error = dlerror();
        if (error || get_module_func == nullptr) {
            VORTEX_ERROR("Unable to load module '{0}'. Get module instance function (get_vortex_module) not found. Error: {1}", module_name_func(), error);

            return nullptr;
        }

        Module* module_ptr = get_module_func();
        if (module_ptr == nullptr) {
            VORTEX_ERROR("Unable to load module '{0}'. Get module instance function returned null pointer", module_name_func());

            return nullptr;
        }

        _loaded_modules[module_name] = new ModuleInstance{
            lib_handle,
            module_ptr,
        };

        VORTEX_TRACE("Module '{0}' loaded", module_name);

        return module_ptr;
    }

    Module* LinuxModuleLoader::get_module(const std::string& module_name) {
        const auto& it = _loaded_modules.find(module_name);
        if (it != _loaded_modules.end()) {
            return it->second->module;
        }

        return nullptr;
    }

    bool LinuxModuleLoader::module_loaded(const std::string& module_name) {
        return _loaded_modules.find(module_name) != _loaded_modules.end();
    }

#endif

}
