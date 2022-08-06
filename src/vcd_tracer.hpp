/* 
 *  C++ VCD Tracer Library
 *
 *  For more information see https://github.com/nakane1chome/cpp-vcd-tracer
 *
 * Copyright (c) 2022, Philip Mulholland
 * All rights reserved.
 * 
 * Using the  BSD 3-Clause License
 *
 * See LICENSE for license details.
 */

#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#ifndef SIMPLE_VCD_HPP
#define SIMPLE_VCD_HPP

/**
   Phases:

   As VCD is format used for hardware design there is a number phases used to build a representation of the system to be traced.

   - Analysis  - The compiler parsed and builds the program.
   - Elaboration - At run time the program registers values with the tracer.
   - Run time tracing -

 */
namespace vcd_tracer {

    /** Set this to true to log to stderr */
    constexpr bool SIMPLE_VCD_DEBUG = false;

    /** A class to generate VCD a sequence of unique variable identifiers.
        The identifier is composed of printable ASCII characters from ! to ~ (decimal 33 to 126).
    */
    class identifier_generator {
      public:
        /** Generate the next VCD identifier.
         * @retval VCD identifer.
         */
        constexpr const char *next(void);

      private:
        std::array<char, 16> _identifier{ 0 };
        size_t _size{ 0 };
    };


    /**  This class represents te interface between classes in the VCD tracer module.
         As the VCD tracer is implemented using the dependency injection style these functions and types are used instead of a class hierachy.
    */
    class scope_fn {
      public:
        //! The sequence determines what order variables will be dumped in.
        using sequence_t = std::uint64_t;
        //! A sequence may be optional if there are no variables to dump.
        using optional_sequence_t = std::optional<std::uint64_t>;
        // A dump sequence defines
        struct dump_sequence_t {
            optional_sequence_t dumped;
            optional_sequence_t next;
        };
        //! The type signature of a function used to write a variable to file.
        //! It returns information on what value has been dumped and when it should be called again.
        using dumper_fn = std::function<dump_sequence_t(std::ostream &, bool)>;
        //! The type signature of a function used to ??
        using updater_fn = std::function<void(dumper_fn fn)>;
        //! The type signature of a function used to add a variable to be traced. The parameter define the information reqired VCD by a VCD header.
        using add_fn = std::function<
            struct value_context(std::string_view var_name,
                                 std::string_view var_type,
                                 unsigned int bit_size,
                                 dumper_fn fn)>;
        //! The type signature of a function used to register a variable and it's dumper.
        using register_fn = std::function<
            struct value_context(std::string_view var_name,
                                 dumper_fn fn)>;
        //! A constant to represent the end of a set of variables to be dumped.
        static constexpr dump_sequence_t end_sequence{ {}, {} };
        //! A constant to represent an empty sequence.
        static const sequence_t nop_sequence{ 0 };
        //! A stub function to be used in place of a real dumper function.
        static dump_sequence_t nop_dump(std::ostream &out, bool start) {
            (void)out;
            (void)start;
            return end_sequence;
        }
        //! A stub function to be used in place of a real update function.
        static void nop_update(dumper_fn fn) {
            (void)fn;
        }
    };

    /** Represent the context of a value to be traced.
     */
    struct value_context {
        //! The identifier of the value
        std::string identifier;
        //! The update function of the value.
        scope_fn::updater_fn updater;
    };


    /** Represent a module within a hierarchical scope.
        A module has a parent and children modules.
        This class only captures the child modules, on the assumption we will traverse from the top module.
        It is expected this class is only used during the elaboration phase, and can be freed once all variables are assigned identifers.
     */
    class module_instance {
      public:
        /** Define a hierarchical module context by it's instance name
            @param name The name of this instance.
         */
        module_instance(const std::string_view name)
            : instance_name(name) {
        }
        //! The name of this module instance.
        const std::string instance_name;
        //! This string buffer will capture the VCD header information, it is written to file at the end of elaboration.
        std::ostringstream vcd_scope;
        //! The children of this module instance.
        std::vector<std::shared_ptr<class module_instance>> children;
    };

    /** A type trait like structure used to determine the size in bits of a C++ type.
     */
    template<typename T>
    struct bit_size {
        //! Use the sizeof() the underlying type, and convert to 8 bits per byte.
        static constexpr unsigned int value = sizeof(T) * 8;
    };
    /** Specialized type trait to indicate boolean values are traced as one bit regardless of the C++ storage size.
     */
    template<>
    struct bit_size<bool> {
        // 1 bit
        static constexpr unsigned int value = 1;
    };
    /** A type trait like structure used to determine the var type
     */
    template<typename T>
    struct vcd_var_type {
        static constexpr const char *value = "wire";
    };
    template<>
    struct vcd_var_type<float> {
        static constexpr const char *value = "real";
    };
    template<>
    struct vcd_var_type<double> {
        static constexpr const char *value = "real";
    };

    /** Define an index that is used to managed reading back traced values to file.
        Values may be traced to file in a different order to which they are traced to the buffer.
    */
    template<int INDEX_TRACE_DEPTH>
    struct index {
        //! Write index when values are traced to memory
        int write{ -1 };
        //! Read index when values are read from memory and traced to file.
        int read{ 0 };
    };

    /** Specialie te index for tracing a history of depth 1.
     */
    template<>
    struct index<1> {
        //! Write index to indicate if a value has been traced.
        int write{ -1 };
    };

    /** In VCD any value can have a state beyond it's known value (as defined by it's type).
     */
    enum class value_state {
        //! A value that is unknown. It will be traced as an 'x' value.
        unknown_x,
        //! A value that has not been driven. It will be traced as an 'z' value.
        undriven_z,
        //! A normal known value.
        known,
    };

    /** A structure to represent a sample that has been traced with state and sequence context.
        - The state can be set directly, or determined to me known when a value is set.
        - The sequence is recorded from a global sequence number.

        This structure can be further specialized when less information is required to be traced.
    */
    template<typename SAMPLE_T, scope_fn::sequence_t *SAMPLE_CUR_SEQ>
    struct sample {
        //! The global sequence of the value, used to determine the order of tracing.
        scope_fn::sequence_t sequence{ static_cast<scope_fn::sequence_t>(-1) };
        //! The state of the value.
        value_state state{ value_state::unknown_x };
        //! The value to be traced if the state is known.
        SAMPLE_T value;
        /** Set a value and record a sequence.
            @param v A value that will be traced.
        */
        void set(SAMPLE_T v) {
            sequence = *SAMPLE_CUR_SEQ;
            state = value_state::known;
            value = v;
        }
        /** Set the state and record a sequence.
            @param S The new state.
        */
        void set_state(value_state S) {
            sequence = *SAMPLE_CUR_SEQ;
            state = S;
        }
    };

    /** A structure to represent a sample that has been traced with state.
        - The state can be set directly, or determined to me known when a value is set.

        This structure is specialized to remove tracing of sequence numbers.

    */
    template<typename SAMPLE_T>
    struct sample<SAMPLE_T, nullptr> {
        //! The state of the value.
        value_state state{ value_state::unknown_x };
        //! The value to be traced if the state is known.
        SAMPLE_T value;
        /** Set a value .
            @param v A value that will be traced.
        */
        void set(SAMPLE_T v) {
            state = value_state::known;
            value = v;
        }
        /** Set the state.
            @param S The new state, such as unknown.
        */
        void set_state(value_state S) {
            state = S;
        }
    };


    /** Representation a value to be traced without it's type information.

        In a vcd header this will represent a $var declaration (but it will not generate the header)
        In the vcd body this class will provide the trace values.

        The lifetime of this class is the full trace lifetime.

     */
    class value_base {
      public:
        value_base(value_base &&) = delete;
        value_base(const value_base &) = delete;
        value_base &operator=(value_base &&) = delete;
        value_base &operator=(const value_base &) = delete;

        /** Clean up a traced variable
         */
        virtual ~value_base(void) {
            // Make sure the dumper function is invalidated so the
            // disposed of dumper function is not called.
            _scope.updater(scope_fn::nop_dump);
        }

      protected:
        /** Create a new value to be traced, defining it at compile time.
            A value cannot be created without type information, so this must be called by a concrete child class constructora.

            @param bit_size  The size, in bits, of this value.
            @param add_fn    The function used to add the variable to the parent hiearchy.
            @param var_name  The name of the value to be traced.
            @param dumper_fn The specialized function that can be used to dump this value to file.
        */
        value_base(const unsigned int bit_size,
                   const char *var_type, 
                   scope_fn::add_fn add_fn,
                   const std::string_view var_name,
                   scope_fn::dumper_fn dumper_fn)
            : _scope(add_fn(var_name, var_type, bit_size, dumper_fn)) {
        }
        /** Create a new value to be traced without defining it at compile time.
            @param bit_size  The size, in bits, of this value.

             elaborate_base() must be called after this.
        */
        value_base(unsigned int bit_size)
            // Define a default do nothing trace context
            : _scope{ "", scope_fn::nop_update } {
            (void)bit_size;
        }

        /**
            @param bit_size  The size, in bits, of this value.
            @param add_fn    The function used to add the variable to the parent hiearchy.
            @param var_name  The name of the value to be traced.
            @param dumper_fn The specialized function that can be used to dump this value to file.
         */
        void elaborate_base(const unsigned int bit_size,
                            const char *var_type,
                            scope_fn::add_fn add_fn,
                            const std::string_view var_name,
                            scope_fn::dumper_fn dumper_fn) {
            auto new_scope = add_fn(var_name, var_type, bit_size, dumper_fn);
            _scope.identifier = new_scope.identifier;
            _scope.updater = new_scope.updater;
        }

      public:
      protected:
        // This is the context required to trace the variable.
        value_context _scope;

      protected:
        // A common dumper function.
        template<typename T>
        void dump(std::ostream &out, const size_t bit_size, const value_state state, const T value) const;
    };// value_base


    /** Typed value for tracing representation.
        @tparam BIT_SIZE - The size in bits of the type.
        @tparam TRACE_DEPTH - When set to more than one a buffer of values can be accumulated before writing to file.
        @tparam CUR_SEQ - This is a pointer to a global sequence counter.

        In a vcd header this will represent a $var declaration (but it will not generate the header)
        In the vcd body this class will provide the trace values.

        The lifetime of this class is the full trace lifetime.
     */
    template<typename T,
             unsigned int BIT_SIZE = bit_size<T>::value,
             int TRACE_DEPTH = 1,
             scope_fn::sequence_t *CUR_SEQ = nullptr>
    class value : public value_base {
      private:
        // The write index, and read index for buffered traces.
        index<TRACE_DEPTH> _idx{ 0 };
        // The values will be stored directly in this instance.
        std::array<sample<T, CUR_SEQ>, static_cast<size_t>(TRACE_DEPTH)> _samples;

      public:
        /** Instanciate an uninitialized value. The state will be set to unknown
            The name and scope need to be set later via elaborate().
         */
        value(void)
            : value_base(BIT_SIZE) {
            _idx.write = -1;
            _samples[0].set_state(value_state::unknown_x);
        }
        /** Instanciate a trace value with an initialized value. The state will be set to known.
            @param default_value Initial value to be traced at time 0.
            The name and scope need to be set later via elaborate().
         */
        value(const T default_value)
            : value_base(BIT_SIZE) {
            _idx.write = -1;
            _samples[0].set(default_value);
        }
        /** Instanciate named and scoped trace value with an uninitialized value. The state will be set to unknown
            @param var_name variable name
            @param add_fn Funtion to register this trace variable with it's scope.
        */
        value(scope_fn::add_fn add_fn,
              const std::string_view var_name)
            : value_base(BIT_SIZE,
                         vcd_var_type<T>::value,
                         add_fn,
                         var_name,
                         [this](std::ostream &out, bool start) -> scope_fn::dump_sequence_t {
                             return this->dump(out, start);
                         }) {
            _idx.write = -1;
        }
        /** Instanciate named and scoped trace value with an initialized value. The state will be set to known.
            @param var_name variable name
            @param add_fn Funtion to register this trace variable with it's scope.
            @param default_value Initial value to be traced at time 0.
        */
        value(scope_fn::add_fn add_fn,
              const std::string_view var_name,
              const T default_value)
            : value_base(BIT_SIZE,
                         vcd_var_type<T>::value,
                         add_fn,
                         var_name,
                         [this](std::ostream &out, bool start) -> scope_fn::dump_sequence_t {
                             return this->dump(out, start);
                         }) {
            _idx.write = -1;
            _samples[0].set(default_value);
        }
        /** Elaborate a value by settings it's name and scope
            @param add_fn Funtion to register this trace variable with it's scope.
            @param default_value Initial value to be traced at time 0.
        */
        void elaborate(scope_fn::add_fn add_fn,
                       const std::string_view var_name) {
            elaborate_base(BIT_SIZE,
                           vcd_var_type<T>::value,
                           add_fn,
                           var_name,
                           std::bind(&value<T, BIT_SIZE, TRACE_DEPTH, CUR_SEQ>::dump, this, std::placeholders::_1, std::placeholders::_2));
        }
        /** Set a variables state to a compile time defined value
         */
        template<value_state S>
        void set_state(void) {
            if constexpr (TRACE_DEPTH == 1) {
                // Case for unbuffered trace.
                if (_samples[0].state != S) {
                    // Set the state and flag that it has been updated via _idx.write
                    _samples[0].set_state(S);
                    _idx.write = 1;
                }
            }
            else {
                // Case for buffered write
                // Only update the trace if
                // 1. The write index was the default value (-1), OR
                // 2. The most recent trace state does not matcha
                if ((_idx.write == -1) || ((_idx.write < TRACE_DEPTH) && (_samples[static_cast<size_t>(_idx.write % TRACE_DEPTH)].state != S))) {
                    // The trace needs updating
                    if ((_idx.write == -1) || (_samples[static_cast<size_t>(_idx.write % TRACE_DEPTH)].sequence != *CUR_SEQ)) {
                        // A new timestamp, or uninitialized write index.
                        // The index should be moved.
                        _idx.write++;
                    }
                    if (_idx.write < TRACE_DEPTH) {
                        // Set the value if the buffer has space.
                        _samples[static_cast<size_t>(_idx.write % TRACE_DEPTH)].set_state(S);
                    }
                }
            }
        }
        /** Assign this trace variable to the unknown (X) state
         */
        void unknown(void) {
            set_state<value_state::unknown_x>();
        }
        /** Assign this trace variable to the undriven (Z) state
         */
        void undriven(void) {
            set_state<value_state::undriven_z>();
        }

        /** A helper function to determine if a change is traced.
            @retval true The sample has changed.
        */
        inline bool sample_changed(const T new_sample, const T prev_sample) {
            return new_sample != prev_sample;
        }
        /** Set a value to be traced
            @param v The new value to be traced.
        */
        void set(const T v) {
            if constexpr (TRACE_DEPTH == 1) {
                // Case for unbuffered trace.
                if (sample_changed(v, _samples[0].value) || (_samples[0].state != value_state::known)) {
                    // Set the value and flag that it has been updated via _idx.write
                    _samples[0].set(v);
                    _idx.write = 1;
                }
            }
            else {
                // Case for buffered trace.
                // Only update the trace if
                // 1. The write index was the default value (-1), OR
                // 2. The most recent trace value does not match
                if ((_idx.write == -1) || ((_idx.write < TRACE_DEPTH) && ((sample_changed(v, _samples[static_cast<size_t>(_idx.write % TRACE_DEPTH)].value)) || _samples[static_cast<size_t>(_idx.write % TRACE_DEPTH)].state != value_state::known))) {
                    if ((_idx.write == -1) || (_samples[static_cast<size_t>(_idx.write % TRACE_DEPTH)].sequence != *CUR_SEQ)) {
                        // Move the write index, due to an uninitialized index or timestamp change.
                        _idx.write++;
                    }
                    if (_idx.write < TRACE_DEPTH) {
                        // Save the sampe value
                        _samples[static_cast<size_t>(_idx.write % TRACE_DEPTH)].set(v);
                    }
                }
            }
        }

      private:
        /** Dump values from this trace variable to file.
            @param out    Output stream to write VCD values to
            @param start  Flag to indicate this is the first write.
            @retval scope_fn::end_sequence When there are no more values to read,
            @retval {dumped index, next index}  When there are more values to read, the NEXT sequence.
        */
        scope_fn::dump_sequence_t dump(std::ostream &out, bool start);
    };// value

    /** A class to represent a module instance scope.

        This class represents a '$scope module ' declaration in a VCD header.

        The lifetime of this class can be restricted to the elaboration phase.

    */
    class module {
      public:
        /** Declare a module instance, and provide the parent scope via a registration function.
            @param register_fn   Provide the scope via this function.
            @param instance_name The name of this module instance.
        */
        module(scope_fn::register_fn register_fn,
               std::string_view instance_name) :_register_fn{ register_fn },
            _context{ std::make_shared<module_instance>(instance_name) } {
            _context->vcd_scope << "$scope module " << instance_name << " $end\n";
        }
        /** Declare a module instance, and provide the parent scope a reference to another module.
            @param parent   Provide the scope via this reference.
            @param instance_name The name of this module instance.
        */
        module(module &parent,
               std::string_view instance_name) :_register_fn{ parent.get_register_fn() },
            _context{ std::make_shared<module_instance>(instance_name) } {
            _context->vcd_scope << "$scope module " << instance_name << " $end\n";
            parent._context->children.push_back(_context);
        }

      private:
        /** Declare a module instance from within a different module instance,
            @param register_fn   Provide the scope via this function.
            @param_context parent   Provide the scope via this pointer.
            @param instance_name The name of this module instance.
        */
        module(scope_fn::register_fn register_fn,
               std::string_view instance_name,
               std::weak_ptr<module_instance> parent_context) :_register_fn{ register_fn },
            _context{ std::make_shared<module_instance>(instance_name) } {
            _context->vcd_scope << "$scope module " << instance_name << " $end\n";
            if (auto use_parent_context = parent_context.lock()) {
                use_parent_context->children.push_back(_context);
            }
        }

      public:
        /** Elaborate a trace variable within this module. The varible should have been instanciated WITHOUT scope.
            @tparam BIT_SIZE The size in bits of the trace variable.
            @tparam TRACE_DEPTH The size of the trace buffer.
            @tparam CUR_SEQ The pointer to the sequence counter.
            @param var A trace variable already declared.
            @param var_name The name of the variable to be used when tracing.
        */
        template<typename T,
                 unsigned int BIT_SIZE,
                 int TRACE_DEPTH,
                 scope_fn::sequence_t *CUR_SEQ>
        void elaborate(value<T, BIT_SIZE, TRACE_DEPTH, CUR_SEQ> &var, const std::string_view var_name) {
            var.elaborate(get_add_fn(), var_name);
        }
        /** Get the function to be provided to a trace variable declaration to give it scoped withing this module instance
         */
        [[nodiscard]] scope_fn::add_fn get_add_fn(void) {
            return [this](std::string_view var_name,
                          std::string_view var_type,
                          const unsigned int bit_size,
                          scope_fn::dumper_fn fn) -> value_context {
                return this->add_var(var_name, var_type, bit_size, fn);
            };
        }
        /**
           template<typename T,
           unsigned int BIT_SIZE,
           int TRACE_DEPTH,
           scope_fn::sequence_t *CUR_SEQ>
           value<T, BIT_SIZE, TRACE_DEPTH, CUR_SEQ> get_value(const std::string_view var_name) {
           return value<T,BIT_SIZE, TRACE_DEPTH, CUR_SEQ>(get_add_fn(), var_name);
           }**/
        /** Create a new child module within the scope of this module instance
            @param child_name The instance name of the new module.
        */
        [[nodiscard]] module get_module(const std::string_view child_name) {
            return module(get_register_fn(), child_name, _context);
        }
        /** Write the VCD header to the output buffer
         */
        void finalize_header(std::ostream &out);

      private:
        // This function passed a new variable up in the heirarchy to be assinged a global identifier and registered.
        // TODO should be in _context so we can remove the scaffoling of the module() class
        scope_fn::register_fn _register_fn;
        std::shared_ptr<module_instance> _context;

      private:
        void finalize_header(std::ostream &out,
                             std::shared_ptr<const module_instance> context);

        /** A function that can be passed to a new trace variable to declare it within the scope of this module instance
         */
        [[nodiscard]] value_context add_var(std::string_view var_name,
                                            std::string_view var_type,
                                            const unsigned int bit_size,
                                            scope_fn::dumper_fn fn) {
            std::string child_path = _context->instance_name + "." + std::string(var_name);
            auto value_context = _register_fn(child_path, fn);
            _context->vcd_scope
                << "$var " << var_type
                << " " << bit_size
                << " " << value_context.identifier
                << " " << var_name
                << " $end\n";
            return value_context;
        }
        /** Get the function that can be used to register a variable within this module
         */
        [[nodiscard]] scope_fn::register_fn get_register_fn(void) {
            return [this](std::string_view child_path,
                          scope_fn::dumper_fn fn) -> value_context {
                return this->register_var(child_path, fn);
            };
        }
        /** Get the function that can be used to register a variable within this module
            @param child_path The path of the variable. It will be added to the full instance path to be registered at the next level.
            @param fn         The function that will be used to dump a given variable to file.
        */
        [[nodiscard]] value_context register_var(std::string_view child_path,
                                                 scope_fn::dumper_fn fn) {
            std::string this_path = _context->instance_name + "." + std::string(child_path);
            auto value_context = _register_fn(this_path, fn);
            return value_context;
        }
    };// module

    /** A class to represent the top scope of a trace.

        This will corrospond to a single VCD trace file.

        This class manages the elaboration and trace phase.

        This class manages the trace time.

    */
    class top {
      public:
        top(std::string_view name);

        //! This is the time resolution of the trace.
        using time_base = std::chrono::nanoseconds;

        // time_base trace_interval {time_base::min()};

        /** End the elaboration phase and write the VCD file header to a file.
            Once this is done no new trace variables can be added.
            @param out Trace output.
            @param date The date to be set in the header $date field.
        */
        void finalize_header(std::ostream &out,
                             std::chrono::time_point<std::chrono::system_clock> date);
        /** Update the timestamp of the trace with a delta time to the previous timestamp
            This will result in an output to the trace file of the stored data.
            @param out Trace output.
            @param delta The change in time since this function was last called.
        */
        void time_update_delta(std::ostream &out, time_base delta);

        /** Update the timestamp of the trace with am absolute time.
            This will result in an output to the trace file of the stored data.
            @param out Trace output.
            @param timestamp The trace will be moved to this timestamp.
        */
        void time_update_abs(std::ostream &out, time_base timestamp);

        /** Flush  the remaining trace
            @param out Trace output.
        */
        void finalize_trace(std::ostream &out);

      private:

        struct map_data {
            // Map identifiers to variable names
            std::map<std::string, std::string> identifier_map;
            // Map idenfiers to dump functions.
            std::map<std::string, scope_fn::dumper_fn> dumper_map;
        } ;

      private:
        /** This function will do the bulk of the dumping af variables. */
        void time_update_core(std::ostream &out);

      private:
        // The most recently traced time
        scope_fn::sequence_t _tracepoint;
        // The most recently updated time.
        scope_fn::sequence_t _timestamp;
        // Write out a time in the VCD trace format
        void log_time(std::ostream &out, scope_fn::sequence_t new_time, bool force, std::string_view reason);
        // Creation of identifiers
        std::shared_ptr<identifier_generator> _identifier_generator = std::make_shared<identifier_generator>();
        // Mapping of registers to identifiers and functions
        std::shared_ptr<map_data> _var_map = std::make_shared<map_data>();

      public : 
        //! The root module in the design hiearchy
        module root;


    };// topc

}// namespace vcd_tracer

#endif

#include "vcd_tracer.ipp"
