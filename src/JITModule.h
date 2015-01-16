#ifndef HALIDE_JIT_MODULE_H
#define HALIDE_JIT_MODULE_H

/** \file
 * Defines the struct representing a JIT compiled halide pipeline
 */

#include "IntrusivePtr.h"
#include "runtime/HalideRuntime.h"

#include <map>

namespace llvm {
class Module;
class Type;
}

namespace Halide {

struct Target;

namespace Internal {

class JITModuleContents;
class CodeGen;

struct JITModule {
    IntrusivePtr<JITModuleContents> jit_module;

    struct Symbol {
        void *address;
        llvm::Type *llvm_type;
        Symbol() : address(NULL), llvm_type(NULL) {}
        Symbol(void *address, llvm::Type *llvm_type) : address(address), llvm_type(llvm_type) {}
    };

    JITModule() {}
    JITModule(const std::map<std::string, Symbol> &exports);

    const std::map<std::string, Symbol> &exports() const;

    /** A pointer to the raw halide function. It's true type depends
     * on the Argument vector passed to CodeGen::compile. Image
     * parameters become (buffer_t *), and scalar parameters become
     * pointers to the appropriate values. The final argument is a
     * pointer to the buffer_t defining the output. This will be NULL for
     * a JITModule which has not yet been compiled or one that is not
     * a Halide Func compilation at all. */
    void *main_function() const;

    /** A slightly more type-safe wrapper around the raw halide
     * module. Takes it arguments as an array of pointers that
     * correspond to the arguments to \ref main_function . This will
     * be NULL for a JITModule which has not yet been compiled or one
     * that is not a Halide Func compilation at all. */
    int (*jit_wrapper_function() const)(const void **);

    // TODO: This should likely be a constructor.
    /** Take an llvm module and compile it. The requested exports will
        be available via the Exports method. */
    void compile_module(CodeGen *cg,
                        llvm::Module *mod, const std::string &function_name,
                        const std::vector<JITModule> &dependencies,
                        const std::vector<std::string> &requested_exports);

    /** Make extern declarations fo rall exports of this JITModule in another llvm::Module */
    void make_externs(llvm::Module *mod);

    /** Encapsulate device (GPU) and buffer interactions. */
    int copy_to_dev(struct buffer_t *buf) const;
    int copy_to_host(struct buffer_t *buf) const;
    int dev_free(struct buffer_t *buf) const;
};

typedef int (*halide_task)(void *user_context, int, uint8_t *);

struct JITCustomAllocator {
    void *(*custom_malloc)(void *, size_t);
    void (*custom_free)(void *, void *);
    JITCustomAllocator() : custom_malloc(NULL), custom_free(NULL) {}
    JITCustomAllocator(void *(*custom_malloc)(void *, size_t),
                       void (*custom_free)(void *, void *)) : custom_malloc(custom_malloc), custom_free(custom_free) {
    }
};

struct JITHandlers {
    void (*custom_print)(void *, const char *);
    JITCustomAllocator custom_allocator;
    int (*custom_do_task)(void *, halide_task, int, uint8_t *);
    int (*custom_do_par_for)(void *, halide_task, int, int, uint8_t *);
    void (*custom_error)(void *, const char *);
    int32_t (*custom_trace)(void *, const halide_trace_event *);
    JITHandlers() : custom_print(NULL), custom_do_task(NULL), custom_do_par_for(NULL),
                    custom_error(NULL), custom_trace(NULL) {
    }
};

struct JITUserContext {
    void *user_context;
    JITHandlers handlers;
};

class JITSharedRuntime {
public:
    // Note only the first CodeGen passed in here is used. The same shared runtime is used for all JIT.
    static JITModule &get(CodeGen *cg, const Target &target);
    static void init_jit_user_context(JITUserContext &jit_user_context, void *user_context, const JITHandlers &handlers);
    static JITHandlers set_default_handlers(const JITHandlers &handlers);
    static void release_all();
};
}
}

#endif
