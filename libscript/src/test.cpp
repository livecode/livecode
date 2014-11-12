#include "foundation.h"
#include "foundation-auto.h"

#include "script.h"
#include "script-private.h"

#define CHECK(a) __Check(a)

static void __Check(bool p_value)
{
    MCAssert(p_value);
}

void build_module(MCStreamRef stream)
{
    MCScriptModuleBuilderRef t_builder;
    MCScriptBeginModule(kMCScriptModuleKindWidget, MCNAME("com.livecode.testwidget"), t_builder);
    
    uindex_t t_number_value_var_index;
    MCScriptAddVariableToModule(t_builder, MCNAME("sNumberValue"), kMCNumberTypeInfo, t_number_value_var_index);
    uindex_t t_number_value_prop_index;
    MCScriptAddPropertyToModule(t_builder, MCNAME("numberValue"), t_number_value_var_index, t_number_value_var_index, t_number_value_prop_index);
    MCScriptAddExportToModule(t_builder, t_number_value_prop_index);
    
    MCAutoTypeInfoRef t_virt_number_value_getter_sig;
    CHECK(MCHandlerTypeInfoCreate(nil, 0, kMCNumberTypeInfo, &t_virt_number_value_getter_sig));
    uindex_t t_virt_number_value_getter;
    MCScriptBeginHandlerInModule(t_builder, MCNAME("virtualNumberValue"), *t_virt_number_value_getter_sig, t_virt_number_value_getter);
    MCScriptEmitAssignConstantInModule(t_builder, 0, kMCMinusOne);
    MCScriptEmitReturnInModule(t_builder);
    MCScriptEndHandlerInModule(t_builder);
    
    uindex_t t_virt_number_value_prop_index;
    MCScriptAddPropertyToModule(t_builder, MCNAME("virtualNumberValue"), t_virt_number_value_getter, 0, t_virt_number_value_prop_index);
    MCScriptAddExportToModule(t_builder, t_virt_number_value_prop_index);
    
    CHECK(MCScriptEndModule(t_builder, stream));
}

int main(int argc, char *argv[])
{
    MCInitialize();
    
    MCStreamRef t_stream;
    void *t_module_buffer;
    size_t t_module_buffer_size;
    CHECK(MCMemoryOutputStreamCreate(t_stream));
    build_module(t_stream);
    CHECK(MCMemoryOutputStreamFinish(t_stream, t_module_buffer, t_module_buffer_size));
    MCValueRelease(t_stream);
    
    MCScriptModuleRef t_module;
    CHECK(MCMemoryInputStreamCreate(t_module_buffer, t_module_buffer_size, t_stream));
    CHECK(MCScriptCreateModuleFromStream(t_stream, t_module));
    MCValueRelease(t_stream);
    
    MCScriptInstanceRef t_instance;
    CHECK(MCScriptCreateInstanceOfModule(t_module, t_instance));
    
    MCValueRef t_before_value, t_after_value;
    CHECK(MCScriptGetPropertyOfInstance(t_instance, MCNAME("numberValue"), t_before_value));
    CHECK(MCScriptSetPropertyOfInstance(t_instance, MCNAME("numberValue"), kMCOne));
    CHECK(MCScriptGetPropertyOfInstance(t_instance, MCNAME("numberValue"), t_after_value));
    
    MCLog("numberValue: before = %@, after = %@", t_before_value, t_after_value);
    
    MCValueRef t_virtual_value;
    CHECK(MCScriptGetPropertyOfInstance(t_instance, MCNAME("virtualNumberValue"), t_virtual_value));
    
    MCLog("virtualNumberValue: %@", t_virtual_value);
    
    MCFinalize();
    
    return 0;
}
