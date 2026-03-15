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
#include <cfloat>
#include <cmath>
#include <climits>
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
    if constexpr (std::is_floating_point_v<DATA_T>) {
      // Match the VCD library's %-.16g precision
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%-.16g", static_cast<double>(data[cur]));
      csv_out << buf;
    } else {
      csv_out << +data[cur];
    }
  }
};

static constexpr unsigned int CYCLES = 40;
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

    // Corner case: empty module (no signals, no children)
    vcd_tracer::module empty(dumper.root, "empty");

    // Corner case: pass-through module (only sub-scopes, no direct signals)
    vcd_tracer::module passthru(dumper.root, "passthru");
    vcd_tracer::module passthru_inner(passthru, "inner");

    // Corner case: single-char module name
    vcd_tracer::module m_a(dumper.root, "a");

    // Corner case: name starting with digit (sanitizer should handle)
    vcd_tracer::module m_1st(dumper.root, "1st");

    // Corner case: name with multiple special chars (tests sanitizer)
    vcd_tracer::module m_special(dumper.root, "foo.bar@baz");

    // Corner case: duplicate module names at different levels
    vcd_tracer::module dup_a(dumper.root, "dup");
    vcd_tracer::module dup_b(l0, "dup");

    // Corner case: deep nesting (8 levels below root)
    vcd_tracer::module deep1(dumper.root, "d1");
    vcd_tracer::module deep2(deep1, "d2");
    vcd_tracer::module deep3(deep2, "d3");
    vcd_tracer::module deep4(deep3, "d4");
    vcd_tracer::module deep5(deep4, "d5");
    vcd_tracer::module deep6(deep5, "d6");
    vcd_tracer::module deep7(deep6, "d7");
    vcd_tracer::module deep8(deep7, "d8");

    data_cycle<vcd_tracer::value<bool>, bool, 2> b1{l0, "b1", false, true};

    // Signals in corner-case modules
    data_cycle<vcd_tracer::value<uint8_t>, uint8_t, 3> pt_sig{passthru_inner, "pt_sig",
        0x00, 0xAA, 0x55};
    data_cycle<vcd_tracer::value<uint8_t, 4>, uint8_t, 2> a_sig{m_a, "a_sig", 0x3, 0xC};
    data_cycle<vcd_tracer::value<uint16_t>, uint16_t, 2> dig_sig{m_1st, "dig_sig",
        0x1234, 0xABCD};
    data_cycle<vcd_tracer::value<uint8_t>, uint8_t, 2> spc_sig{m_special, "spc_sig",
        0x5A, 0xA5};
    data_cycle<vcd_tracer::value<uint8_t>, uint8_t, 3> dup_r{dup_a, "dup_r", 0x11, 0x22, 0x33};
    data_cycle<vcd_tracer::value<uint8_t>, uint8_t, 3> dup_l{dup_b, "dup_l", 0x44, 0x55, 0x66};
    data_cycle<vcd_tracer::value<uint32_t>, uint32_t, 4> deep_sig{deep8, "deep_sig",
        0xDEADBEEFu, 0xCAFEBABEu, 0x12345678u, 0x5A5A5A5Au};

    // Corner case: module same name as a signal (module "u8" alongside signal "u8")
    vcd_tracer::module m_u8(dumper.root, "u8");
    data_cycle<vcd_tracer::value<uint8_t>, uint8_t, 2> u8m_sig{m_u8, "u8m_sig", 0xDE, 0xAD};
    data_cycle<vcd_tracer::value<double>, double, 8> dbl{l1, "dbl",
        0.0, 1.5, -3.25, 100.0,
        DBL_MIN, DBL_MAX, -DBL_MAX, DBL_EPSILON};
    data_cycle<vcd_tracer::value<float>, float, 7> flt{l0_l0, "flt",
        0.0f, 2.5f, -1.0f,
        FLT_MIN, FLT_MAX, -FLT_MAX, FLT_EPSILON};
    data_cycle<vcd_tracer::value<uint8_t, 1>, uint8_t, 2> u1{l0_l1, "u1", 0x0, 0x1};
    data_cycle<vcd_tracer::value<uint8_t, 2>, uint8_t, 4> u2{l1_l0, "u2", 0x0, 0x1, 0x2, 0x3};
    data_cycle<vcd_tracer::value<uint8_t, 4>, uint8_t, 6> u4{l0_l0_l0, "u4",
        0x0, 0x5, 0xA, 0xF, 0x5, 0xA};
    data_cycle<vcd_tracer::value<uint8_t, 7>, uint8_t, 6> u7{l0, "u7",
        0x00, 0x7F, 0x01, 0x55, 0x2A, 0x5A};
    data_cycle<vcd_tracer::value<uint8_t>, uint8_t, 7> u8{l0, "u8",
        0x00, 0x7F, 0xFF, 0x01, 0x55, 0xAA, 0x5A};
    data_cycle<vcd_tracer::value<uint16_t, 9>, uint16_t, 6> u9{l0, "u9",
        0x000, 0x100, 0x1FF, 0x155, 0x0AA, 0x15A};
    data_cycle<vcd_tracer::value<uint16_t, 15>, uint16_t, 6> u15{l0, "u15",
        0x0000, 0x3FFF, 0x7FFF, 0x5555, 0x2AAA, 0x5A5A};
    data_cycle<vcd_tracer::value<uint16_t>, uint16_t, 7> u16{l0, "u16",
        0x0000, 0x8000, 0xFFFF, 0x0001, 0x5555, 0xAAAA, 0x5A5A};
    data_cycle<vcd_tracer::value<uint32_t, 17>, uint32_t, 6> u17{l0, "u17",
        0x00000, 0x10000, 0x1FFFF, 0x15555, 0x0AAAA, 0x15A5A};
    data_cycle<vcd_tracer::value<uint32_t, 24>, uint32_t, 6> u24{l0_l0_l0_l0, "u24",
        0x000000, 0x7FFFFF, 0xFFFFFF, 0x555555, 0xAAAAAA, 0x5A5A5A};
    data_cycle<vcd_tracer::value<uint32_t, 31>, uint32_t, 7> u31{l0, "u31",
        0x00000000, 0x3FFFFFFF, 0x7FFFFFFF, 0x00000001,
        0x55555555, 0x2AAAAAAA, 0x5A5A5A5A};
    data_cycle<vcd_tracer::value<uint32_t>, uint32_t, 7> u32{l0, "u32",
        0x00000000u, 0x80000000u, 0xFFFFFFFFu, 0x00000001u,
        0x55555555u, 0xAAAAAAAAu, 0x5A5A5A5Au};
    data_cycle<vcd_tracer::value<uint64_t, 33>, uint64_t, 7> u33{l0, "u33",
        0x0ull, 0x100000000ull, 0x1FFFFFFFFull, 0x1ull,
        0x155555555ull, 0x0AAAAAAAAull, 0x15A5A5A5Aull};
    data_cycle<vcd_tracer::value<uint64_t>, uint64_t, 8> u64{l0, "u64",
        0x0ull, 0x7FFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull,
        0x1ull, 0x8000000000000000ull,
        0x5555555555555555ull, 0xAAAAAAAAAAAAAAAAull, 0x5A5A5A5A5A5A5A5Aull};

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
