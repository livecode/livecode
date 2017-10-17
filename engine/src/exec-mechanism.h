#ifndef __EXEC_MECHANISM__
#define __EXEC_MECHANISM__

template<typename TypeInfo>
class MCXAuto
{
public:
    ~MCXAuto(void)
    {
        TypeInfo::Drop(m_value);
    }
    
    typename TypeInfo::ValueType& operator & (void)
    {
        return m_value;
    }
    
    typename TypeInfo::ValueType operator * (void) const
    {
        return m_value;
    }
    
private:
    typename TypeInfo::ValueType m_value;
};

/* CLASS REFERENCES */

class MCExecContext;
class MCCompileContext;
class MCScriptPoint;

/* OVERVIEW
 *
 * The implementation of LiveCode Script is split into a collection of syntax
 * node classes (e.g. MCAccept) and implementation methods.
 *
 * Each syntax class has a parse method together with an exec_ctxt/eval_ctxt
 * method, and generally represents multiple distinct pieces of syntax - those
 * which all start with the same keyword.
 *
 * Whilst the parse methods are generally very ad-hoc, the exec/eval methods
 * are not - their action is completely describable abstractly by defining a
 * mapping from the syntactic arguments (represented as fields in the AST
 * node) to the arguments of an exec/eval implementation method.
 *
 * The new structure of the exec mechanism works as follows:
 *
 *   1) The current syntax classes remain, but will become just for parsing. All
 *      execution related methods will be removed, and instead a virtual
 *      compile method will be added. The compile method uses the parsed
 *      information to create a new execution-oriented class instance which
 *      represents a call to a single (multi) execution method.
 *
 *   2) The invocation classes will be defined using a template based
 *      on a constexpr execution method description struct. These will be
 *      instantiated by the syntax class specialize method to build the
 *      'execution oriented' syntax tree.
 *
 *   3) The execution method description structs describe the parameters and
 *      attributes of a execution (multi) method. This struct will contain all
 *      the information to evaluate argument expressions, converting types as
 *      appropriate, as well as properties of the method such as whether it is
 *      constant, pure or async.
 *
 *   4) The parameters of an execution method will be described, similarly, by
 *      (constexpr) parameter description structs. These encode all the details
 *      of how the parameter should be evaluated, the 'high-level' (script-
 *      oriented) type it needs to be and how this maps to the actual parameter
 *      of the execution method.
 *
 *   5) The types used will be described by (costexpr) type description structs.
 *      These structs will define any attributes of the type along with details
 *      of how to evaluate an argument expression for it and conversion to
 *      target types.
 *
 *   6) All type conversion operators will be implemented as explicit non-inlined
 *      functions - this helps ensure that the shim code generated for the actual
 *      calls to the implementation methods is as close to the current hand-coded
 *      versions which exist at the moment.
 *
 * With this structure, it will be possible to completely change how script is
 * executed 'just' by changing the templates which use the constexpr description
 * structures. Indeed, by having a completely mechanised description, there is 
 * not necessarily any need to generate code for dispatching the implementation
 * methods, or doing the type conversions at all. Instead, they could be changed
 * to generate a bytecode VM or similar, with the whole mechanism running
 * dynamically (in particular, the current compile methods which do exist become
 * unnecessary - they could be generated using a variant of the Exec template).
 *
 * Whilst the current structure of the engine does not quite fit into the
 * required model for the above, the changes required to move it to it are
 * all incremental and can be done piece-meal at the granuality of the syntax
 * classes.
 */

/* The MCXAuto class is a unique_ptr-like class which uses the exec mechanism's
 * type descriptors, rather than bare C++ types. */
template<typename T>
class MCXAuto
{
public:
    ~MCXAuto(void) {T::drop(m_value);}

    typename T::value_type& operator & (void) {return m_value;}
    typename T::value_type operator * (void) {return m_value;}
    
private:
    typename T::value_type m_value = {};
};

/* EVALUATION ORDER AND SIDE EFFECTS
 *
 * Whenever a phrase has side-effects, what the side-effects are can be greatly
 * affected by evaluation order of the required sub-phrases.
 *
 * Being a high-level language, it is important that all execution follows a
 * strict and obvious evaluation order, so that side-effects can be properly
 * understood.
 *
 * The initial proposed evaluation order for LiveCode is as follows:
 *
 *   - left to right for parameters: parameters for a phrase (as a whole) are
 *     evaluated left to right
 *
 *   - container to containee for slicing: chunk expressions are evaluated
 *     starting at the 'largest' container and proceed to the 'smallest
 *
 *   - depth-first: all effects of the completion of any sub-expression
 *     evaluation are seen before the expression using them
 *
 * This order is natural when considered from 'the most obvious way to evaluate
 * assuming no side-effects' and essentially corresponds to a left-to-right-
 * depth-first ordering when chunk expressions are rewritten in dot notation
 * form, e.g:
 *
 *    word 5 to -1 of line 3 of field 1 of card 2
 *
 * Can be rewritten as:
 *
 *    card(2).field(1).line(3).word(5, -1)
 *
 * Note: Other evaluation orders can be enforced (e.g. strict left-to-right
 *       source order) by rewriting code to evaluate sub-phrases into
 *       temporaries.
 */
 
/* SYNTAX CLASSES
 *
 * The current script parsing code is entirely hand-coded, the result being an
 * entirely ad-hoc parser. Although it should be possible to mechanise the
 * parsing of an ideal form of the LiveCode script syntax, the existing parse
 * methods need to be preserved in order to provide backwards-compatibility with
 * existing scripts.
 *
 * In the new exec mechanism, what are currently the combined parsing and
 * evaluation classes will be split into two pieces. The existing MCStatement
 * and MCExpression base-classes will be retained, but become purely for parsing,
 * with all execution related methods removed. Instead, they will be augmented
 * with a 'compile' virtual method which will take the information gathered
 * during the parse pharse, and generate invocation instances containing the
 * information required for execution.
 *
 * The details of the compilation phase are 'hidden' in an MCCompileContext
 * class which is passed through to all compile methods. This allows easier
 * error management, and also the ability to change the approach taken to
 * compile the parse nodes in the future.
 *
 * Whilst MCStatement parse nodes can be compiled directly after they have
 * been parsed, MCExpression parse nodes will be compiled after a complete
 * expression tree has been parsed. This approach means that the current
 * implementation of precedence-based parsing can be retained, and all syntax
 * related details need only appear in the parsing classes. */

/* The MCStatement class is the base class of all parsing classes which
 * operate in statement context. */
class MCStatement
{
public:
    virtual ~MCStatement(void) = 0;
    
    virtual Parse_stat parse(MCScriptPoint& sp) = 0;
    virtual void compile(MCCompileContext& ctxt) = 0;
    
protected:
    uint2 line;
    uint2 pos;
};

/* The MCExpression class is the base class of all parsing classes which
 * operate in an expression context. */
class MCExpression
{
public:
    virtual ~MCExpression(void) = 0;
    
    virtual Parse_stat parse(MCScriptPoint& sp, Boolean doingthe) = 0;
    virtual void compile(MCCompileContext& ctxt) = 0;
    
protected:
    uint2 line;
    uint2 pos;
    Factor_rank rank;
    MCExpression* root;
    MCExpression* left;
    MCExpression* right;
};

/* TYPE DESCRIPTION STRUCTS
 */

/* TYPE CONVERSION FUNCTIONS
 */

/* PARAMETER DESCRIPTION STRUCTS
 *
 * The implementation methods are normal C functions, and as such don't contain
 * sufficient information in their signatures to describe how the argument
 * expressions at the script level should be evaluated to the actual C argument
 * values.
 *
 * In order to provide this information, each parameter is described by a
 * constexpr struct which describes both the type and other ephemeral information
 * about the parameter.
 *
 * The parameter kinds are as follows:
 *
 *    - Constant: A constant parameter's value is provided without any script
 *      evaluation. For example, the 'accept secure connection' implementation
 *      method has a final boolean parameter which is determined by the presence
 *      of 'with verification' or 'without verification'.
 *
 *    - Required: A required parameter must always be present, its value being
 *      computed by evaluation of an expression.
 *
 *    - Optional: An optional parameter may or may not be present, in the case
 *      of it not being present, the parameter takes a fixed default value
 *      specified at engine compile time (e.g. like having p_param = 100 in a
 *      C++ function definition).
 *
 *    - Nullable: A nullable parameter may or may not be present. The type of
 *      a nullable parameter must allow optionality to be specified in some way.
 *      For example, for a nullable ValueRef parameter, nil would be used.
 *
 *    - Return: A return parameter provides a slot into which a value can be
 *      placed.
 *
 *    - Update: An update parameter provides a slot which can be read and
 *      written without the effects of a write being applied until successful
 *      completion of the invocation method.
 *
 *    - Reference: A reference parameter is essentially a pair of closures,
 *      one for reading, and one for writing; abstracting access to the
 *      underlying slot.
 *
 * Note: Constant, Required, Optional and Nullable are variants of 'in' mode
 *       parameters, Return is an 'out' mode parameter and 'Update' is an
 *       'in-out' mode parameter.
 */

/* The MCXParam structs define methods and types for implementing the evaluation
 * and assignment to parameter expressions.
 *
 * Implementation method invocation occurs in three phases:
 *
 *   1) Read - where all the parameters are evaluated / marked as needed
 *
 *   2) Invoke - where the invocation method is actually called
 *
 *   3) Write - where all the parameters are assigned as needed
 *
 * Each kind of parameter implements read / write methods appropriate to their
 * purpose. */

/* The Param structs must be templatized based on the type-descriptor T, and
 * provide all the fields indicated in the MCXExampleParam template. */
template<typename T>
struct MCXExampleParam
{
    /* The type field is the type descriptor describing the value of the
     * parameter. */
    typedef T type;
    /* The value type field is the C++ type of the parameter. */
    typedef typename T::value_type value_type;
    /* The eval_type field is the C++ type to use to evaluate the parameter's
     * value. */
    typedef T eval_type;
    /* The node_type field is the C++ type to use for the field in the
     * invocation class. */
    typedef MCXExpression* node_type;
    /* The read method is called before invoking the implementation method to
     * evaluate/mark the parameter's value. */
    bool read(MCExecContext& ctxt, node_type p_node, eval_type& r_value);
    /* The write method is called after invoking the implementation method to
     * assign the parameter's value. */
    bool write(MCExecContext& ctxt, eval_type& p_value);
    /* The argument method is called to pass the parameter's value/ref to the
     * implementation method. */
    value_type argument(MCExecContext& ctxt, eval_type& p_value);
};

/* A required parameter evaluates the argument on read, and does nothing on
 * write as it corresponds to a normal 'in' parameter. */
template<typename T>
struct MCXRequiredParam
{
    /* The type descriptor of the parameter type. */
    typedef T type;
    /* The type of the value passed to the invocation method is the value_type
     * of the type descriptor. */
    typedef typename T::value_type value_type;
    /* As this is just an in parameter, we only need to store the actual
     * value_type whilst we invoke. */
    typedef typename T::value_type eval_type;
    /* Required parameters arguments are expressed as MCXExpression nodes. */
    typedef MCXExpression* node_type;
    /* The read phase of a required parameter simply evaluates the expression
     * node. */
    bool read(MCExecContext& ctxt, node_type p_expr, eval_type& r_value)
    {
        return p_expr->evaluate(ctxt, r_value);
    }
    /* The write phase of a required parameter does nothing. */
    bool write(MCEXecContext& ctxt, eval_type& p_value)
    {
        return true;
    }
    /* The value passed to the invocation argument is just the evaluated value
     */
    value_type argument(MCExecContext& ctxt, eval_type& p_value)
    {
        return p_value;
    }
};

/* METHOD DESCRIPTION STRUCTS
 *
 * All implementation methods for syntax are implemented in MCExec or MCEval
 * functions. On the whole, these functions are generally specific to a single
 * syntax variant (although some need refinement - see later) and follow strict
 * calling conventions with regards the arguments.
 *
 * Each such function will have an associated constexpr description structure
 * which provides higher-level information about the parameters, along with
 * other attributes which are needed to perform execution efficiently.
 *
 * Note: In the future, by changing the C++ signature of the implementation
 *       functions, it should be possible to auto generate the majority of the
 *       description. However, as C++ does not have a reflective attribute
 *       system, an extrinsic description structure will always be needed.
 *
 * An implementation method is described by the following:
 *
 *   - A sequence of parameter descriptions.
 *
 *   - A sequence of 'legacy' error codes, which are used when evaluation
 *      fails for a particular concrete argument.
 *
 *      Note: The current scripting engine uses distinct error codes per
 *            piece of syntax to flag an error on any one parameter. In the
 *            future this could be replace by a mechanically derived approach
 *            using argument names and position.
 *
 *   - A set of attributes:
 *
 *         . is_constant : a constant implementation function is one whose
 *           resut is entirely computed from its arguments (not including
 *           any current execution state as provided via the ExecContext
 *           class). A constant function must have no side-effects.
 *
 *         . is_pure : a pure implementation function is one whose result
 *           is computed from its arguments (including the execution state)
 *           and by reading global state. A pure function must have no side-
 *           effects.
 *
 *         . is_async : an async implementation function is one which does call
 *           or might call wait.
 *
 *   - The implementation function pointer.
 *
 */

/* INVOCATION CLASSES
 *
 * The execution classes describe a call to an implementation method and are
 * generated during the compile step which occurs after parsing.
 *
 * There are four kinds of invocation which can occur:
 *
 *   - execute: used for statement phrases, an execute invocation causes the
 *     corresponding implementation method to be called, producing no explicit
 *     return value.
 *
 *   - fetch: used for evaluating expression phrases, an evaluate invocation
 *     causes the corresponding implementation method to be called, producing
 *     an explicit return value.
 *
 *   - store: used for setting the value of a target expression phrase, an
 *     assign invocation sets the value of the target expression to an explicitly
 *     provided value.
 *
 *   - mark: used for target expression phrases, e.g. for mutating chunk
 *     expressions, a mark invocation causes the (symbolic) location of
 *     the chunk to be computed which can then be evaluated or assigned as
 *     appropriate.
 *
 * Note: The 'mark' invocation kind is important to ensure that sub-expressions
 *       are only evaluated once. There is the classic bug in the current
 *       implementation:
 *
 *            add 1 to any item of tString
 *
 *       Currently, this causes random() to be evaluated both to fetch the
 *       item, and then again to set it - which is entirely non-obvious.
 *
 * Note: In the future, the 'mark' invocation kind could be replaced by
 *       sub-expression assignment to temporaries, and construction of a pair
 *       of closures - one for read, one for write. However, without closures,
 *       the 'mark' concept is required to implement by-ref arguments.
 *
 * In general, invocation proceeds in three phrases:
 *
 *   - fetch: in parameters are read, in-out parameters are marked and the mark
 *           read, out parameters are marked
 *
 *   - invoke: the implemention method is called
 *
 *   - store: the marks attached to in-out/out parameters are written
 *
 * For cases where there are no evaluation side-effects between an out parameter's
 * read and write phrase, the out parameter need not be marked; instead
 * it can be written. This optimization makes statements such as:
 *
 *   put X into Y
 *
 * More efficient, as it becomes:
 *
 *   fetch(X -> t1)
 *   PutInto(t1 -> t2) -- this is just the assignment t2 := t1
 *   store(t2 -> Y)
 *
 */
    
/* The compile methods of the syntax classes generate instances of the
 * invocation node, depending on the context of the syntax. All invocation
 * nodes have MCXNode as a base-class which provides position information
 * and the virtual destructor.
 */

/* The MCXNode class is the base class of all invocation nodes, it stores the
 * position information for the syntax giving rise to the invocation so that
 * errors can be reported against the correct source location. */
class MCXNode
{
public:
    virtual ~MCXNode(void) = 0;
    
protected:
    uint16_t m_row;
    uint16_t m_column;
};

/* The MCXStatement class is the base of all invocation nodes which arise from
 * compiling statement phrases. */
class MCXStatement: public MCXNode
{
public:
    virtual bool execute(MCExecContext& ctxt) = 0;
};

/* The MCXExpression class is the base of all invocation nodes which arise from
 * compiling evaluate-only expression phrases */
class MCXExpression: public MCXNode
{
public:
    virtual bool evaluate(MCExecContext& ctxt, MCXValue& r_value) = 0;
};

/* The MCXTarget class is the base of all invocation nodes which arise from
 * compiling assign-only expression phrases */
class MCXTarget: public MCXNode
{
public:
    virtual bool assign(MCExecContext& ctxt, MCXValue&& q_value) = 0;
};

/* The MCXContainer class is the base of all invocation nodes which arise from
 * marking a container expression phrase */
class MCXContainer: public MCXNode
{
public:
    virtual bool mark(MCExecContext& ctxt, MCXContainer*& r_mark) = 0;
    virtual bool fetch(MCExecContext& ctxt, MCXValue& r_value) = 0;
    virtual bool store(MCExecContext& ctxt, MCXValue&& q_value) = 0;
};

/**/

template<typename Method>
class MCXConcreteStatement0: public MCXStatement
{
public:
    virtual bool execute(MCExecContext& ctxt)
    {
        Method::Function(ctxt);
        return !ctxt.HasError();
    }
};

template<typename Method>
class MCXConcreteStatement1: public MCXStatement
{
public:
    virtual void execute(MCExecContext& ctxt)
    {
        MCXAuto<typename Method::Param1::eval_type> t_arg_1;
        Exec_errors t_error;
        if (Method::Param1::read(ctxt, *m_field_1, &t_arg_1))
        {
            Method::Function(ctxt, Method::Param1::argument(*t_arg_1));
            if (!ctxt.HasError() &&
                Method::Param1::write(ctxt, *t_arg_1))
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            t_error = Method::Error1;
        }
        ctxt.LegacyThrow(t_error);
    }

private:
    MCXAuto<typename Method::Param1::node_type> m_field_1;
};

class MCCompileContext
{
public:
};

/* IMPLEMENTATION METHOD REFINEMENT
 *
 * In order for the exec mechanism to work, it is important that each
 * implementation method represent one specific variant of syntax from the
 * point of view of evaluation.
 *
 * In some cases, the implemention methods are not quite fine-grained enough
 * to enable a simple description of the execution action.
 *
 * For example, the 'choose' command can either be static:
 *    choose browse tool
 * Or dynamic:
 *    choose "browse tool"
 *
 * Currently the implementation method for the function takes two parameters:
 *
 *    void MCInterfaceExecChooseTool(MCExecContext& ctxt,
 *                                   MCStringRef p_input,
 *                                   int p_tool)
 *
 * Where p_input is used if p_tool is T_UNDEFINED. This implementation method
 * will need to be split into two:
 *
 *    MCInterfaceExecChooseToolStatic
 *    MCInterfaceExecChooseToolDynamic
 *
 * This will then allow the syntax class to compile to the appropriate method
 * depending on whether the syntax contained a literal, or an expression.
 */

#endif
