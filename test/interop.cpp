/*
 *  C++ VCD Tracer Library Interoperability test
 *
 *  For more information see https://github.com/nakane1chome/cpp-vcd-tracer
 *
 * Copyright (c) 2026, Philip Mulholland
 * All rights reserved.
 *
 * Using the  BSD 3-Clause License
 *
 * See LICENSE for license details.
 */

#include "../src/vcd_tracer.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <fstream>

/** Generic data cycling base class */
class data_cycle_base {
protected:
  data_cycle_base() {
    generators.emplace_back(this);
  }
public:
  virtual void hdr(std::ostream &csv_out) = 0;
  virtual void advance() = 0;
  virtual void write_csv(std::ostream &csv_out) = 0;
  virtual const std::string &get_name() const = 0;
  static std::vector<data_cycle_base *> generators;
};

std::vector<data_cycle_base *> data_cycle_base::generators;

/** Type specialized data cycling base class */
template<typename TRACE_T, typename DATA_T, size_t N>
struct data_cycle : data_cycle_base {
  template<typename... Args>
  data_cycle(vcd_tracer::module &parent, std::string_view var_name, Args&&... args)
  : data_cycle_base()
  , data{{static_cast<DATA_T>(args)...}}
  , name(var_name)
  {
    parent.elaborate(var, var_name);
  }
  std::array<DATA_T, N> data;
  TRACE_T var;
  size_t index{0};
  std::string name;

  void hdr(std::ostream &csv_out) override {
    csv_out << name;
  }

  const std::string &get_name() const override {
    return name;
  }

  void advance() override {
    DATA_T v = data[index++];
    index = index % N;
    var.set(v);
  }

  void write_csv(std::ostream &csv_out) override {
    // Write the current value (index was already advanced)
    size_t cur = (index == 0) ? N - 1 : index - 1;
    csv_out << +data[cur];
  }
};

static constexpr unsigned int CYCLES = 20;
static constexpr unsigned int TICK_NS = 1;

int main(int argc, const char **argv) {

    const std::string vcd_fout_name{ (argc > 1) ? argv[1] : "interop.vcd" };
    const std::string csv_fout_name{ (argc > 2) ? argv[2] : "interop.csv" };

    // The top module defines the base of the signal hierarchy
    vcd_tracer::top dumper("root");

    // Create module hierarchy
    vcd_tracer::module l0(dumper.root, "l0");
    vcd_tracer::module l0_l0(l0, "l0-l0");
    vcd_tracer::module l0_l1(l0, "l0-l1");
    vcd_tracer::module l1(dumper.root, "l1");
    vcd_tracer::module l1_l0(l1, "l1-l0");
    vcd_tracer::module l0_l0_l0(l0_l0, "l0-l0-l0");
    vcd_tracer::module l0_l0_l0_l0(l0_l0_l0, "l0-l0-l0-l0");

    data_cycle<vcd_tracer::value<bool>, bool, 2> b1{l0, "b1", false, true};
    data_cycle<vcd_tracer::value<double>, double, 4> dbl{l1, "dbl", 0.0, 1.5, -3.25, 100.0};
    data_cycle<vcd_tracer::value<float>, float, 3> flt{l0_l0, "flt", 0.0f, 2.5f, -1.0f};
    data_cycle<vcd_tracer::value<uint8_t, 1>, uint8_t, 2> u1{l0_l1, "u1", 0x0, 0x1};
    data_cycle<vcd_tracer::value<uint8_t, 2>, uint8_t, 4> u2{l1_l0, "u2", 0x0, 0x1, 0x2, 0x3};
    data_cycle<vcd_tracer::value<uint8_t, 4>, uint8_t, 4> u4{l0_l0_l0, "u4", 0x0, 0x5, 0xA, 0xF};
    data_cycle<vcd_tracer::value<uint8_t>, uint8_t, 3> u8{l0, "u8", 0x00, 0x7F, 0xFF};
    data_cycle<vcd_tracer::value<uint16_t, 9>, uint16_t, 3> u9{l0, "u9", 0x000, 0x100, 0x1FF};
    data_cycle<vcd_tracer::value<uint16_t, 15>, uint16_t, 3> u15{l0, "u15", 0x0000, 0x3FFF, 0x7FFF};
    data_cycle<vcd_tracer::value<uint16_t>, uint16_t, 3> u16{l0, "u16", 0x0000, 0x8000, 0xFFFF};
    data_cycle<vcd_tracer::value<uint32_t, 17>, uint32_t, 3> u17{l0, "u17", 0x00000, 0x10000, 0x1FFFF};
    data_cycle<vcd_tracer::value<uint32_t, 24>, uint32_t, 3> u24{l0_l0_l0_l0, "u24", 0x000000, 0x7FFFFF, 0xFFFFFF};
    data_cycle<vcd_tracer::value<uint32_t, 31>, uint32_t, 3> u31{l0, "u31", 0x00000000, 0x3FFFFFFF, 0x7FFFFFFF};
    data_cycle<vcd_tracer::value<uint32_t>, uint32_t, 3> u32{l0, "u32", 0x00000000u, 0x80000000u, 0xFFFFFFFFu};
    data_cycle<vcd_tracer::value<uint64_t, 33>, uint64_t, 3> u33{l0, "u33", 0x0ull, 0x100000000ull, 0x1FFFFFFFFull};
    data_cycle<vcd_tracer::value<uint64_t>, uint64_t, 3> u64{l0, "u64", 0x0ull, 0x7FFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};

    // Open files for output
    {
        std::ofstream vcd_fout(vcd_fout_name);
        std::ofstream csv_fout(csv_fout_name);

        // Finalize signals before tracing
        // The VCD format does not allow dynamic signal definition.
        dumper.finalize_header(vcd_fout,
                               std::chrono::system_clock::from_time_t(0));

        // Sort generators by name to match pyvcd's alphabetical column order
        auto sorted = data_cycle_base::generators;
        std::sort(sorted.begin(), sorted.end(),
                  [](const data_cycle_base *a, const data_cycle_base *b) {
                      return a->get_name() < b->get_name();
                  });

        csv_fout << "time";
        for (auto *g : sorted) {
          csv_fout << ",";
          g->hdr(csv_fout);
        }
        csv_fout << "\n";

        // time_update_abs(i) first dumps pending changes at the
        // current tracepoint, then advances tracepoint to i.
        // So values set before time_update_abs(i) appear at
        // time i-1 in the VCD.
        //
        // vcd_to_csv skips time 0 and records state when the
        // next timestamp appears.
        //
        // Strategy: advance, then call time_update_abs(i+1).
        // The advance values get dumped at time i.
        // Write CSV for time i with those same values.
        // First call at i=0 dumps at time 0 (skipped by vcd_to_csv).

        for (unsigned int i = 0; i < CYCLES; i++) {

          // Set signals for this timestep
          for (auto *g : data_cycle_base::generators) {
            g->advance();
          }

          // Dump to VCD — values appear at tracepoint i
          dumper.time_update_abs(vcd_fout, std::chrono::nanoseconds{ TICK_NS * (i + 1) });

          // Write CSV (skip time 0 to match vcd_to_csv behavior)
          if (i > 0) {
            csv_fout << i;
            for (auto *g : sorted) {
              csv_fout << ",";
              g->write_csv(csv_fout);
            }
            csv_fout << "\n";
          }

        }

        // vcd_to_csv appends a final row for the last timestamp
        // with accumulated state. Write matching row.
        csv_fout << CYCLES;
        for (auto *g : sorted) {
          csv_fout << ",";
          g->write_csv(csv_fout);
        }
        csv_fout << "\n";

    }

    return 0;
}
