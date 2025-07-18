#pragma once

#include <vector>
#include <cstdint>
#include "WaylandProtocol.hpp"
#include "xwayland-shell-v1.hpp"
#include "../helpers/signal/Signal.hpp"

class CWLSurfaceResource;

class CXWaylandSurfaceResource {
  public:
    CXWaylandSurfaceResource(SP<CXwaylandSurfaceV1> resource_, SP<CWLSurfaceResource> surface_);
    ~CXWaylandSurfaceResource();

    bool       good();
    wl_client* client();

    struct {
        CSignalT<> destroy;
    } events;

    uint64_t                     m_serial = 0;
    WP<CWLSurfaceResource>       m_surface;

    WP<CXWaylandSurfaceResource> m_self;

  private:
    SP<CXwaylandSurfaceV1> m_resource;
    wl_client*             m_client = nullptr;
};

class CXWaylandShellResource {
  public:
    CXWaylandShellResource(SP<CXwaylandShellV1> resource_);

    bool good();

  private:
    SP<CXwaylandShellV1> m_resource;
};

class CXWaylandShellProtocol : public IWaylandProtocol {
  public:
    CXWaylandShellProtocol(const wl_interface* iface, const int& ver, const std::string& name);

    virtual void bindManager(wl_client* client, void* data, uint32_t ver, uint32_t id);

    struct {
        CSignalT<SP<CXWaylandSurfaceResource>> newSurface; // Fired when it sets a serial, otherwise it's useless
    } m_events;

  private:
    void destroyResource(CXWaylandSurfaceResource* resource);
    void destroyResource(CXWaylandShellResource* resource);

    //
    std::vector<SP<CXWaylandShellResource>>   m_managers;
    std::vector<SP<CXWaylandSurfaceResource>> m_surfaces;

    friend class CXWaylandSurfaceResource;
    friend class CXWaylandShellResource;
};

namespace PROTO {
    inline UP<CXWaylandShellProtocol> xwaylandShell;
};
