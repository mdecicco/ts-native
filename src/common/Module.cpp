#include <tsn/common/Module.h>
#include <tsn/ffi/DataType.h>
#include <tsn/utils/ModuleSource.h>
#include <tsn/bind/calling.hpp>

#include <utils/Array.hpp>

namespace tsn {
    Module::Module(Context* ctx, const utils::String& name, const utils::String& path, const script_metadata* meta) : IDataTypeHolder(ctx), m_name(name), m_path(path) {
        m_id = (u32)std::hash<utils::String>()(path);
        m_meta = meta;
        m_src = nullptr;
        m_didInit = false;
    }

    Module::~Module() {
        for (u32 i = 0;i < m_data.size();i++) {
            if (m_data[i].type) {
                ffi::Function* dtor = m_data[i].type->getDestructor();
                if (dtor) ffi::call_method(m_ctx, dtor, m_data[i].ptr);
            }
            delete [] (u8*)m_data[i].ptr;
        }
    }

    void Module::init() {
        if (m_didInit) return;

        auto initfunc = findFunctions("__init__", nullptr, nullptr, 0, fm_ignore_args);
        if (initfunc.size() == 1) {
            ffi::call(m_ctx, initfunc[0]);
        }
        
        m_didInit = true;
    }
    
    u32 Module::getId() const {
        return m_id;
    }

    const utils::String& Module::getName() const {
        return m_name;
    }

    const utils::String& Module::getPath() const {
        return m_path;
    }

    const script_metadata* Module::getInfo() const {
        return m_meta;
    }
    
    u32 Module::getDataSlotCount() const {
        return m_data.size();
    }

    const module_data& Module::getDataInfo(u32 slot) const {
        return m_data[slot];
    }

    void Module::setDataAccess(u32 slot, access_modifier access) {
        m_data[slot].access = access;
    }

    Object Module::getData(u32 slot) {
        return Object::View(m_ctx, false, m_data[slot].type, m_data[slot].ptr);
    }
    
    const utils::Array<module_data>& Module::getData() const {
        return m_data;
    }
    
    ModuleSource* Module::getSource() const {
        return m_src;
    }

    u32 Module::addData(const utils::String& name, ffi::DataType* tp, access_modifier access) {
        u32 sz = tp->getInfo().size;
        
        m_data.push({
            new u8[tp->getInfo().size],
            sz,
            tp,
            access,
            name
        });

        return m_data.size() - 1;
    }
    
    u32 Module::addData(const utils::String& name, u32 size) {
        m_data.push({
            new u8[size],
            size,
            nullptr,
            private_access,
            name
        });

        return m_data.size() - 1;
    }

    void Module::setSrc(ModuleSource* src) {
        m_src = src;
    }
};