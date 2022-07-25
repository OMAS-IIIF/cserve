/*
 * Copyright © 2022 Lukas Rosenthaler
 * This file is part of OMAS/cserve
 * OMAS/cserve is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * OMAS/cserve is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <iostream>
#include <dlfcn.h>

#include "RequestHandlerLoader.h"
#include "Cserve.h"

void cserve::RequestHandlerLoader::load() {
    if (!(handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LAZY))) {
        cserve::Server::logger()->error("Could not load plugin {}: {}", path, dlerror());
    }
}

void cserve::RequestHandlerLoader::close() {
    if (dlclose(handle) != 0) {
        cserve::Server::logger()->error("Could not close plugin {}: {}", path, dlerror());
    }
    handle = nullptr;
}

std::shared_ptr<cserve::RequestHandler> cserve::RequestHandlerLoader::get_instance() {
    if (handle == nullptr) load();
    using allocClass = cserve::RequestHandler *(*)();
    using deleteClass = void (*)(cserve::RequestHandler *);

    auto allocFunc = reinterpret_cast<allocClass>(dlsym(handle, allocatorSymbol.c_str()));
    auto deleteFunc = reinterpret_cast<deleteClass>(dlsym(handle, deleterSymbol.c_str()));

    if (!allocFunc || !deleteFunc) {
        close();
        cserve::Server::logger()->error("Could not find alloc/delete function of plugin {}: {}", path, dlerror());
        std::cerr << dlerror() << std::endl;
    }

    cserve::Server::logger()->info("Loaded request handler plugin \"{}\"", path);
    return std::shared_ptr<cserve::RequestHandler>(allocFunc(), [deleteFunc](cserve::RequestHandler *p){ deleteFunc(p); });
}

