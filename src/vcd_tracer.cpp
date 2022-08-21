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

#include <cmath>
#include <iomanip>
#include <type_traits>

#include "vcd_tracer.hpp"

namespace vcd_tracer {

    using namespace std::literals::chrono_literals;


    // File header that does not change.
    constexpr const char *STATIC_VCD_HEADER =
        "$version\n"
        "   C++ Simple VCD Logger\n"
        "$end\n";

    /** Type trait strucuture to translate a C++ chrono type to a
     * string representation for the VCD header.
     * @tparam T A std::chrono::duration specialized type.
     */
    template<typename T>
    struct vcd_timescale {
        static constexpr const char *value = "??";
    };
    template<>
    struct vcd_timescale<std::chrono::nanoseconds> {
        static constexpr const char *value = "ns";
    };
    template<>
    struct vcd_timescale<std::chrono::microseconds> {
        static constexpr const char *value = "us";
    };
    template<>
    struct vcd_timescale<std::chrono::milliseconds> {
        static constexpr const char *value = "ms";
    };
    template<>
    struct vcd_timescale<std::chrono::seconds> {
        static constexpr const char *value = "s";
    };

    // ------------------------------------------
    // Module

    void module::finalize_header(std::ostream &out) {
        if (_context != nullptr) {
            finalize_header(out, _context);
        }
        _context.reset();
    }

    void module::finalize_header(
        std::ostream &out,
        std::shared_ptr<const module_instance> context) {
        // Module and var definitions have been done, collect the
        // submodules and end the definition.
        out << context->vcd_scope.str();
        for (const auto &child : context->children) {
            finalize_header(out, child);
        }
        out << "$upscope $end\n";
    }


    // ------------------------------------------------------------------------
    // Top

    top::top(std::string_view name)
        : root(
            // This function will register any variable in the child
            // hierarchy with this top module.
            [identifier_generator = _identifier_generator, var_map = _var_map](const std::string_view full_path,
                                                                               scope_fn::dumper_fn fn) -> value_context {
                // Allocate a new identifier
                const std::string identifier = identifier_generator->next();
                // Register this new varaible - the path and function to write values to the trace.
                var_map->identifier_map[identifier] = full_path;
                var_map->dumper_map[identifier] = fn;
                // Create a function that allows the registration in this class to be reset by the variable destructor.
                auto updater = [identifier, var_map](scope_fn::dumper_fn fn) -> void {
                    var_map->dumper_map[identifier] = fn;
                };
                return value_context{ identifier, updater };
            },
            name) {
    }

    void top::log_time(std::ostream &out,
                       scope_fn::sequence_t new_time,
                       bool force,
                       std::string_view reason) {
        if (force || (new_time != _tracepoint)) {
            // Insert a new time point into the VCD trace.
            out << "#" << new_time << "\n";
            _tracepoint = new_time;
            // For debugging log what caused this update
            if constexpr (SIMPLE_VCD_DEBUG) {
                out << "$comment LOG TIME " << reason << "$end\n";
            }
        }
        else {
            // For debugging log events that have not caused time to update
            if constexpr (SIMPLE_VCD_DEBUG) {
                out << "$comment NO LOG TIME " << reason << "$end\n";
            }
        }
    }


    void top::finalize_header(std::ostream &out, std::chrono::time_point<std::chrono::system_clock> date) {
        // Create the VCD header
        std::time_t start_time = std::chrono::system_clock::to_time_t(date);
        out << "$date\n"
            << "   " << std::asctime(std::gmtime(&start_time))
            << "$end\n";
        out << "$timescale\n"
            << "   1" << vcd_timescale<top::time_base>::value << "\n"
            << "$end\n";
        out << STATIC_VCD_HEADER;
        // Write out the design hierarchy
        root.finalize_header(out);
        out << "$enddefinitions $end\n";
        // Default values
        log_time(out, 0, true, "finalize header");
        // Log the initial state
        _timestamp = 0;
        time_update_core(out);
    }

    void top::time_update_delta(std::ostream &out, time_base delta) {
        if constexpr (SIMPLE_VCD_DEBUG) {
            out << "$comment DELTA TIME " << delta.count() << " $end\n";
        }
        // Log out the updated variables
        time_update_core(out);
        // Now the variables have been dumped to capture the state UP TO this time
        // log the time
        const auto delta_count = delta.count();
        _timestamp += delta_count;
        if (_timestamp <= _tracepoint) {
            // If the timestamp has fallen behind the tracepoint, move it forward
            _timestamp = _tracepoint;
            if constexpr (SIMPLE_VCD_DEBUG) {
                out << "$comment SYNC ABS TIME WITH TRACEPOINT " << _tracepoint << " $end\n";
            }
        }
        // Format the current time for VCD
        log_time(out, _timestamp, false, "DELTA");
    }

    void top::time_update_abs(std::ostream &out, time_base new_timestamp) {
        if constexpr (SIMPLE_VCD_DEBUG) {
            out << "$comment ABS TIME " << new_timestamp.count() << " $end\n";
        }
        // Log out the updated variables
        time_update_core(out);
        // Now the variables have been dumped to capture the state UP TO this time
        // log the time
        auto new_timestamp_count = new_timestamp.count();
        if (new_timestamp_count >= _timestamp) {
            if (new_timestamp_count <= static_cast<long int>(_tracepoint)) {
                // If the timestamp has fallen behind the tracepoint, move it forward
                new_timestamp_count = _tracepoint;
                if constexpr (SIMPLE_VCD_DEBUG) {
                    out << "$comment SYNC ABS TIME WITH TRACEPOINT " << _tracepoint << " $end\n";
                }
            }
            _timestamp = new_timestamp_count;
            // Format the current time for VCD
            log_time(out, _timestamp, false, "ABS");
        }
        else {
            if constexpr (SIMPLE_VCD_DEBUG) {
                out << "$comment WARNING - backwards time " << new_timestamp.count() << "$end\n";
            }
        }
    }

    void top::time_update_core(std::ostream &out) {
        bool first_unset = true;
        scope_fn::sequence_t first_sequence;
        // First pass - find order of next sample
        std::map<scope_fn::sequence_t, std::vector<std::string>> status;
        if constexpr (SIMPLE_VCD_DEBUG) {
            out << "$comment first pass $end\n";
        }
        // Each variable could be traced out of order to the global timestamp
        // Find the initial time point of the trace variable
        for (auto [identifier, dump_fn] : _var_map->dumper_map) {
            const auto sequence = dump_fn(out, true);
            if (sequence.next.has_value()) {
                status[sequence.next.value()].push_back(identifier);
                if constexpr (SIMPLE_VCD_DEBUG) {
                    out << "$comment first pass found: "
                        << identifier << " @ "
                        << sequence.next.value() << " $end\n";
                }
            }
            if (sequence.dumped.has_value()) {
                if (first_unset || first_sequence > sequence.dumped.value()) {
                    first_unset = false;
                    first_sequence = sequence.dumped.value();
                }
            }
        }
        if constexpr (SIMPLE_VCD_DEBUG) {
            out << "$comment second pass " << status.size() << " $end\n";
        }
        // Second pass - trace buffer values.
        while (status.size() > 0) {
            const auto [sequence, identifiers] = *status.begin();
            if (first_unset) {
                first_sequence = sequence;
                first_unset = false;
            }
            const auto delta = (sequence - first_sequence);
            log_time(out, _timestamp + delta, false, "seq");
            if constexpr (SIMPLE_VCD_DEBUG) {
                out << "$comment seq=" << sequence << ", delta=" << delta << " $end\n";
            }
            for (const auto &identifier : identifiers) {
                const auto done_sequence = _var_map->dumper_map[identifier](out, false);
                if (done_sequence.next.has_value()) {
                    status[done_sequence.next.value()].push_back(identifier);
                    if constexpr (SIMPLE_VCD_DEBUG) {
                        out << "$comment second pass found: "
                            << identifier << " @ "
                            << sequence << " -> "
                            << done_sequence.next.value() << " $end\n";
                    }
                }
                else {
                    if constexpr (SIMPLE_VCD_DEBUG) {
                        out << "$comment second pass not found: "
                            << identifier << " @ "
                            << sequence << " $end\n";
                    }
                }
            }
            status.erase(status.begin());
        }
    }

    void top::finalize_trace(std::ostream &out) {
        // Flush any lingering values
        time_update_delta(out, std::chrono::nanoseconds(1));
        // Allow some time at the end for viewing the final value
        time_update_delta(out, std::chrono::microseconds(1));
    }

    // ------------------------------------------------------------------------
    // Value

    template<typename T>
    void value_base::dump(std::ostream &out,
                          const size_t bit_size,
                          const value_state state,
                          const T value) const {
        if constexpr (SIMPLE_VCD_DEBUG) {
            out << "$comment " << std::hex << value << std::dec << " " << _scope.identifier << " $end\n";
        }
        if constexpr (std::is_floating_point_v<T>) {
            // Emulate .%16g
            std::array<char, 32> format_buf;
            ::snprintf(format_buf.data(), format_buf.size(), "r%-.16g ", static_cast<double>(value));
            out << format_buf.data() << _scope.identifier << "\n";
        }
        else if constexpr (std::is_same_v<T, bool>) {
            if (state == value_state::unknown_x) {
                out << "x" << _scope.identifier << "\n";
            }
            else if (state == value_state::undriven_z) {
                out << "z" << _scope.identifier << "\n";
            }
            else {
                out << (value ? "1" : "0") << _scope.identifier << "\n";
            }
        }
        else {
            out << "b";
            if (state == value_state::unknown_x) {
                out << "x";
            }
            else if (state == value_state::undriven_z) {
                out << "z";
            }
            else {
                T mask = static_cast<T>(1) << (bit_size - 1);
                bool prev_bit = (value & mask) != 0;
                bool compress = true;
                for (int i = bit_size - 2; i >= 0; i--) {
                    mask = mask >> 1;
                    const bool this_bit = (value & mask) != 0;
                    if (compress && (this_bit == prev_bit)) {
                        // Compress the left bits until a change is present..
                    }
                    else {
                        compress = false;
                        out << (prev_bit ? "1" : "0");
                    }
                    prev_bit = this_bit;
                }
                out << (prev_bit ? "1" : "0");
            }
            out << " " << _scope.identifier << "\n";
        }
    }

    // ------------------------------------------------------------------------
    // Template instanciations of value<>

    template void value_base::dump<bool>(std::ostream &out, const size_t bit_size, const value_state state, const bool value) const;

    template void value_base::dump<int>(std::ostream &out, const size_t bit_size, const value_state state, const int value) const;
    template void value_base::dump<unsigned int>(std::ostream &out, const size_t bit_size, const value_state state, const unsigned int value) const;

    template void value_base::dump<char>(std::ostream &out, const size_t bit_size, const value_state state, const char value) const;
    template void value_base::dump<unsigned char>(std::ostream &out, const size_t bit_size, const value_state state, const unsigned char value) const;

    template void value_base::dump<short>(std::ostream &out, const size_t bit_size, const value_state state, const short value) const;
    template void value_base::dump<unsigned short>(std::ostream &out, const size_t bit_size, const value_state state, const unsigned short value) const;

    template void value_base::dump<long>(std::ostream &out, const size_t bit_size, const value_state state, const long value) const;
    template void value_base::dump<unsigned long>(std::ostream &out, const size_t bit_size, const value_state state, const unsigned long value) const;

    template void value_base::dump<float>(std::ostream &out, const size_t bit_size, const value_state state, const float value) const;
    template void value_base::dump<double>(std::ostream &out, const size_t bit_size, const value_state state, const double value) const;


};// namespace vcd_tracer
