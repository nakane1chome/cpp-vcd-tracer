/*  
 *  C++ VCD Tracer Library Signal Definition and Tracing Example.
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


#include "../src/vcd_tracer.hpp"
#include <array>
#include <cmath>
#include <string>
#include <fstream>

std::array<uint32_t, 8192> memory {0};
static constexpr double WAVE_FREQ_HZ=1e6;
static constexpr double WAVE_AMPL_V=4.5;
static constexpr double WAVE_BIAS_V=5.0;
static constexpr unsigned int CYCLES = 10000;
static constexpr unsigned int TICK_NS = 1;

int main(int argc, const char **argv) {

    const std::string fout_name{ (argc > 1) ? argv[1] : "signals.vcd" };

    // Define the signals we want to trace. Only one sample is
    // buffered, so each interation needs to write to disk.
    vcd_tracer::value<bool> clock1;
    vcd_tracer::value<bool> clock2;
    vcd_tracer::value<double> sine_wave;
    vcd_tracer::value<uint16_t> addr;
    vcd_tracer::value<uint32_t> data;
    vcd_tracer::value<uint8_t, 4> burst;
    vcd_tracer::value<bool> wr_rd_n;


    // The top module defines the base of the signal hierarchy
    // It also owns 
    vcd_tracer::top dumper("root");

    // Define a module hierarchy and associate signals with it
    // This elaboration is indepdent of the signal definition above
    // so the modules can be deallocated 
    {
        // Create two child domains.
        // Any number
        vcd_tracer::module digital(dumper.root, "digital");
        vcd_tracer::module bus(digital, "bus");
        vcd_tracer::module analog(dumper.root, "analog");
        
        // The elaboration of signals inside of the module hierarchy
        digital.elaborate(clock1, "clk");
        analog.elaborate(sine_wave, "wave");
        bus.elaborate(clock2, "clk");
        bus.elaborate(addr, "addr");
        bus.elaborate(data, "data");
        bus.elaborate(burst, "burst");
        bus.elaborate(wr_rd_n, "wr_strb");
    }

    // Open a file for output
    {
        std::ofstream fout(fout_name);

        // Finalize signals before tracing11
        // The VCD format does not allow dynamic signal definition.
        dumper.finalize_header(fout, 
                               std::chrono::system_clock::from_time_t(0));

        unsigned int mem_addr = 0;
        burst.set(1);
        
        for (unsigned int i=0; i<CYCLES; i++) {

            // div2
            clock1.set(i & 0x1);
            // div4
            clock2.set((i >> 1) & 0x1);

            // Waveform
            const double seconds = static_cast<double>(i) * 1e-9 * TICK_NS;
            sine_wave.set( WAVE_BIAS_V + (WAVE_AMPL_V * sin(seconds * WAVE_FREQ_HZ * 2.0 * M_PI)));
            
            // Memory read write
            if ((i % 100)==20) {
                // Write
                wr_rd_n.set(true);
                mem_addr = static_cast<unsigned int>(i % memory.size());
                memory[mem_addr] = i * 0x98764321 + 0x33442677;
            }
            if ((i % 100)==21) {
                wr_rd_n.set(false);
            }
            if ((i % 100)==20) {
                // read
                addr.set(static_cast<uint16_t>(i % memory.size()));
            }
            addr.set(static_cast<uint16_t>(mem_addr&0xFFFF));
            data.set(memory[mem_addr]);

            
            dumper.time_update_abs(fout, std::chrono::nanoseconds{ TICK_NS * i });
            
        }

    }

    return 0;
}
