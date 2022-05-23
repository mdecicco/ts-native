#pragma once
#include <gs/common/types.h>
#include <gs/compiler/types.h>
#include <gs/utils/SourceLocation.h>

#include <utils/String.h>

#include <xtr1common>

namespace gs {
    class DataType;
    namespace compiler {
        class ICodeHolder;

        class Value {
            public:
                struct flags {
                    unsigned is_pointer  : 1;
                    unsigned is_const    : 1;

                    // If it's an argument, the argument index will be stored in imm.u
                    unsigned is_argument : 1;
                };

                Value(const Value& o);

                template <typename T>
                std::enable_if_t<std::is_integral_v<T>, T> getImm() const;
                vreg_id getRegId() const;
                DataType* getType() const;
                const utils::String& getName() const;
                const SourceLocation& getSource() const;
                const flags& getFlags() const;
                flags& getFlags();

                // Only one of these can be true
                bool isReg() const;
                bool isImm() const;
                bool isStack() const;

            protected:
                friend class ICodeHolder;
                Value(ICodeHolder* o);

                utils::String m_name;
                SourceLocation m_src;
                DataType* m_tp;
                vreg_id m_regId;
                alloc_id m_allocId;
                flags m_flags;
                union imm {
                    u64 u;
                    i64 i;
                    f64 f;
                };
        };
    };
};