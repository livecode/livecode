#include "foundation.h"
#include "foundation-auto.h"

#include "script.h"
#include "script-private.h"

#define CHECK(a) __Check(a)

static void __Check(bool p_value)
{
    MCAssert(p_value);
}

void build_library_module(MCStreamRef stream)
{
    MCScriptModuleBuilderRef t_builder;
    MCScriptBeginModule(kMCScriptModuleKindLibrary, MCNAME("com.livecode.testlibrary"), t_builder);
    
    MCHandlerTypeFieldInfo t_fields[2] =
    {
        { MCNAME("useDefault"), kMCBooleanTypeInfo, kMCHandlerTypeFieldModeIn },
        { MCNAME("default"), kMCNumberTypeInfo, kMCHandlerTypeFieldModeIn },
    };
    MCAutoTypeInfoRef t_compute_value_sig;
    CHECK(MCHandlerTypeInfoCreate(t_fields, 2, kMCNumberTypeInfo, &t_compute_value_sig));
    uindex_t t_handler_index;
    MCScriptBeginHandlerInModule(t_builder, MCNAME("computeValue"), *t_compute_value_sig, t_handler_index);
    uindex_t t_alternate, t_endif;
    MCScriptDeferLabelForBytecodeInModule(t_builder, t_alternate);
    MCScriptDeferLabelForBytecodeInModule(t_builder, t_endif);
    MCScriptEmitJumpIfFalseInModule(t_builder, 1, t_alternate);
    MCScriptEmitAssignInModule(t_builder, 0, 2);
    MCScriptEmitJumpInModule(t_builder, t_endif);
    MCScriptResolveLabelForBytecodeInModule(t_builder, t_alternate);
    MCScriptEmitAssignConstantInModule(t_builder, 0, kMCZero);
    MCScriptResolveLabelForBytecodeInModule(t_builder, t_endif);
    MCScriptEmitReturnInModule(t_builder);
    MCScriptEndHandlerInModule(t_builder);
    
    MCScriptAddExportToModule(t_builder, t_handler_index);
    
    CHECK(MCScriptEndModule(t_builder, stream));
}

void build_widget_module(MCStreamRef stream)
{
    MCScriptModuleBuilderRef t_builder;
    MCScriptBeginModule(kMCScriptModuleKindWidget, MCNAME("com.livecode.testwidget"), t_builder);
    
    uindex_t t_dependency;
    MCScriptAddDependencyToModule(t_builder, MCNAME("com.livecode.testlibrary"), t_dependency);
    
    MCHandlerTypeFieldInfo t_fields[2] =
    {
        { MCNAME("useDefault"), kMCBooleanTypeInfo, kMCHandlerTypeFieldModeIn },
        { MCNAME("default"), kMCNumberTypeInfo, kMCHandlerTypeFieldModeIn },
    };
    MCAutoTypeInfoRef t_compute_value_sig;
    CHECK(MCHandlerTypeInfoCreate(t_fields, 2, kMCNumberTypeInfo, &t_compute_value_sig));
    uindex_t t_compute_value_def;
    MCScriptAddImportToModule(t_builder, t_dependency, MCNAME("computeValue"), kMCScriptDefinitionKindHandler, *t_compute_value_sig, t_compute_value_def);
    
    uindex_t t_number_value_var_index;
    MCScriptAddVariableToModule(t_builder, MCNAME("sNumberValue"), kMCNumberTypeInfo, t_number_value_var_index);
    uindex_t t_number_value_prop_index;
    MCScriptAddPropertyToModule(t_builder, MCNAME("numberValue"), t_number_value_var_index, t_number_value_var_index, t_number_value_prop_index);
    MCScriptAddExportToModule(t_builder, t_number_value_prop_index);
    
    uindex_t t_usedef_var_index;
    MCScriptAddVariableToModule(t_builder, MCNAME("sUseDefault"), kMCBooleanTypeInfo, t_usedef_var_index);
    uindex_t t_usedef_prop_index;
    MCScriptAddPropertyToModule(t_builder, MCNAME("useDefault"), t_usedef_var_index, t_usedef_var_index, t_usedef_prop_index);
    MCScriptAddExportToModule(t_builder, t_usedef_prop_index);
    
    MCAutoTypeInfoRef t_virt_number_value_getter_sig;
    CHECK(MCHandlerTypeInfoCreate(nil, 0, kMCNumberTypeInfo, &t_virt_number_value_getter_sig));
    uindex_t t_virt_number_value_getter;
    MCScriptBeginHandlerInModule(t_builder, MCNAME("virtualNumberValue"), *t_virt_number_value_getter_sig, t_virt_number_value_getter);
    MCScriptEmitFetchGlobalInModule(t_builder, 1, t_usedef_var_index);
    MCScriptEmitFetchGlobalInModule(t_builder, 2, t_number_value_var_index);
    MCScriptBeginInvokeInModule(t_builder, t_compute_value_def);
    MCScriptContinueInvokeInModule(t_builder, 0);
    MCScriptContinueInvokeInModule(t_builder, 1);
    MCScriptContinueInvokeInModule(t_builder, 2);
    MCScriptEndInvokeInModule(t_builder);
    MCScriptEmitReturnInModule(t_builder);
    MCScriptEndHandlerInModule(t_builder);
    
    uindex_t t_virt_number_value_prop_index;
    MCScriptAddPropertyToModule(t_builder, MCNAME("virtualNumberValue"), t_virt_number_value_getter, 0, t_virt_number_value_prop_index);
    MCScriptAddExportToModule(t_builder, t_virt_number_value_prop_index);
    
    CHECK(MCScriptEndModule(t_builder, stream));
}

void load_constructed_module(void (*builder)(MCStreamRef stream), MCScriptModuleRef& r_module)
{
    MCStreamRef t_stream;
    void *t_module_buffer;
    size_t t_module_buffer_size;
    CHECK(MCMemoryOutputStreamCreate(t_stream));
    builder(t_stream);
    CHECK(MCMemoryOutputStreamFinish(t_stream, t_module_buffer, t_module_buffer_size));
    MCValueRelease(t_stream);
    
    MCScriptModuleRef t_module;
    CHECK(MCMemoryInputStreamCreate(t_module_buffer, t_module_buffer_size, t_stream));
    CHECK(MCScriptCreateModuleFromStream(t_stream, t_module));
    MCValueRelease(t_stream);
    
    CHECK(MCScriptEnsureModuleIsUsable(t_module));

    r_module = t_module;
}

int main(int argc, char *argv[])
{
    MCInitialize();
    
    MCScriptModuleRef t_widget_module, t_library_module;
    load_constructed_module(build_library_module, t_library_module);
    load_constructed_module(build_widget_module, t_widget_module);
    
    MCScriptInstanceRef t_widget_instance, t_library_instance;
    CHECK(MCScriptCreateInstanceOfModule(t_library_module, t_library_instance));
    CHECK(MCScriptCreateInstanceOfModule(t_widget_module, t_widget_instance));
    
    // TEST LIBRARY
    
    MCValueRef t_arguments[2];
    t_arguments[0] = kMCFalse;
    t_arguments[1] = kMCOne;
    MCAutoValueRef t_result_false;
    CHECK(MCScriptCallHandlerOfInstance(t_library_instance, MCNAME("computeValue"), t_arguments, 2, &t_result_false));
    t_arguments[0] = kMCTrue;
    t_arguments[1] = kMCOne;
    MCAutoValueRef t_result_true;
    CHECK(MCScriptCallHandlerOfInstance(t_library_instance, MCNAME("computeValue"), t_arguments, 2, &t_result_true));
    MCLog("computeValue(false, 1) = %@", *t_result_false);
    MCLog("computeValue(true, 1) = %@", *t_result_true);
    
    // TEST WIDGET
    
    MCValueRef t_before_value, t_after_value;
    CHECK(MCScriptGetPropertyOfInstance(t_widget_instance, MCNAME("numberValue"), t_before_value));
    CHECK(MCScriptSetPropertyOfInstance(t_widget_instance, MCNAME("numberValue"), kMCOne));
    CHECK(MCScriptGetPropertyOfInstance(t_widget_instance, MCNAME("numberValue"), t_after_value));
    
    MCLog("numberValue: before = %@, after = %@", t_before_value, t_after_value);
    
    MCValueRef t_virtual_value;
    CHECK(MCScriptSetPropertyOfInstance(t_widget_instance, MCNAME("useDefault"), kMCFalse));
    CHECK(MCScriptGetPropertyOfInstance(t_widget_instance, MCNAME("virtualNumberValue"), t_virtual_value));
    MCLog("virtualNumberValue: %@", t_virtual_value);
    CHECK(MCScriptSetPropertyOfInstance(t_widget_instance, MCNAME("useDefault"), kMCTrue));
    CHECK(MCScriptGetPropertyOfInstance(t_widget_instance, MCNAME("virtualNumberValue"), t_virtual_value));
    MCLog("virtualNumberValue: %@", t_virtual_value);
    
    MCFinalize();
    
    return 0;
}
