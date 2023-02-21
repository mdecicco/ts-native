#include <tsn/optimize/LabelMap.h>
#include <tsn/optimize/CodeHolder.h>
#include <tsn/interfaces/IOptimizationStep.h>
#include <tsn/compiler/IR.h>

namespace tsn {
    namespace optimize {
        LabelMap::LabelMap() {}
        
        LabelMap::LabelMap(CodeHolder* ch) {
            rebuild(ch);
        }

        address LabelMap::get(compiler::label_id label) const {
            return m_map.at(label);
        }

        void LabelMap::rebuild(CodeHolder* ch) {
            for (address i = 0;i < ch->code.size();i++) {
                if (ch->code[i].op == compiler::ir_label) {
                    compiler::label_id lbl = ch->code[i].operands[0].getImm<compiler::label_id>();
                    m_map[lbl] = i;
                }
            }
        }
    };
};