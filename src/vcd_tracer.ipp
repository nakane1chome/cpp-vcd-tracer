/*  -*- c++ -*-
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

#ifndef SIMPLE_VCD_IPP
#define SIMPLE_VCD_IPP

namespace vcd_tracer {

    // ------------------------------------------
    // Identifier Generator

    // VCD identifiers use a subset of ascii. We will assign
    // identifiers incrementally between these characters.
    constexpr char VCD_NAME_START = '!';
    constexpr char VCD_NAME_END = 'z';

    constexpr const char *identifier_generator::next(void) {
        // Generate each combination within the range of:
        // VCD_NAME_START..VCD_NAME_END
        // Append a column when each combination has been used up.
        if (_size == _identifier.size()) {
            return _identifier.data();
        }
        if (_size == 0) {
            _identifier[0] = VCD_NAME_START;
            _identifier[1] = '\0';
            _size = 1;
            return _identifier.data();
        }
        else {
            size_t i = _size;
            // Increment the last characters
            do {
                i--;
                if (_identifier[i] == VCD_NAME_END) {
                    _identifier[i] = VCD_NAME_START;
                }
                else {
                    // Return the valid identifier.
                    _identifier[i]++;
                    return _identifier.data();
                }
            } while (i != 0);
            // increment the next
            // Add a new column, initialize with starting character.
            _identifier[_size] = VCD_NAME_START;
            _size++;
            _identifier[_size] = '\0';
            return _identifier.data();
        }
    }


    // ------------------------------------------
    // Dumper

    template<typename T,
             unsigned int BIT_SIZE,
             int TRACE_DEPTH,
             scope_fn::sequence_t *CUR_SEQ>
    scope_fn::dump_sequence_t value<T, BIT_SIZE, TRACE_DEPTH, CUR_SEQ>::dump(
        std::ostream &out, bool start) {
        if constexpr (TRACE_DEPTH == 1) {
            // Case for unbuffered trace.
            (void)start;
            if (_idx.write) {
                // Dump the value and reset the state
                value_base::dump<T>(out, BIT_SIZE, _samples[0].state, _samples[0].value);
                _idx.write = 0;
            }
            // No sequence is recorded.
            return scope_fn::end_sequence;
        }
        else {
            // Case for buffered trace
            if (start) {
                // Reset the read index at the start of the dump sequence.
                _idx.read = 0;
            }
            if (_idx.read > _idx.write) {
                // No more values to read
                // Reset the write
                _idx.write = -1;
                return scope_fn::end_sequence;
            }
            // Sample to read.
            const size_t read_index = static_cast<size_t>(_idx.read % TRACE_DEPTH);
            if (start) {
                // dont read it, just return the position
                return { {}, _samples[read_index].sequence };
            }
            // Dump a single value
            value_base::dump<T>(out, BIT_SIZE, _samples[read_index].state, _samples[read_index].value);
            // Update the read pointer
            _idx.read++;
            // Test for a buffer with or without global sequencing
            using NullType = std::integral_constant<decltype(CUR_SEQ), nullptr>;
            using ActualType = std::integral_constant<decltype(CUR_SEQ), CUR_SEQ>;
            if constexpr (std::is_same_v<ActualType, NullType>) {
                // No sequence is recorded.
                return { _samples[read_index].sequence, {} };
            }
            else {
                // Find the next location to read.
                if (_idx.read > _idx.write) {
                    _idx.write = -1;
                    // No more values to read
                    return { _samples[read_index].sequence, {} };
                }
                else {
                    // The sequence of the next value
                    return { _samples[read_index].sequence,
                             _samples[static_cast<size_t>(_idx.read % TRACE_DEPTH)].sequence };
                }
            }
        }
    }// dump()

}// namespace vcd_tracer

#endif// #ifndef SIMPLE_VCD_IPP
