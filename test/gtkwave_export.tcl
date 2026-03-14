# GTKWave batch export script: reads loaded VCD and writes CSV
# Usage: gtkwave input.vcd -S gtkwave_export.tcl
# Set GTKWAVE_CSV_OUT environment variable to specify output path.

set csv_out $::env(GTKWAVE_CSV_OUT)

set num_facs [gtkwave::getNumFacs]
set max_time [gtkwave::getMaxTime]

# Enumerate all signals and add to viewer
set all_signals {}
for {set i 0} {$i < $num_facs} {incr i} {
    lappend all_signals [gtkwave::getFacName $i]
}
gtkwave::addSignalsFromList $all_signals

# Build signal info: leaf name, bit width (0 for non-vectors)
set leaf_names {}
set bit_widths {}
foreach sig $all_signals {
    # Extract leaf name and bit width from "root.l0.u9[8:0]"
    set leaf $sig
    set bits 0
    set bracket [string first "\[" $leaf]
    if {$bracket >= 0} {
        # Parse bit range [MSB:LSB] to get width
        set range [string range $leaf [expr {$bracket + 1}] end-1]
        set parts [split $range ":"]
        set msb [lindex $parts 0]
        set lsb [lindex $parts 1]
        set bits [expr {$msb - $lsb + 1}]
        set leaf [string range $leaf 0 [expr {$bracket - 1}]]
    }
    # Take last component after "."
    set dot [string last "." $leaf]
    if {$dot >= 0} {
        set leaf [string range $leaf [expr {$dot + 1}] end]
    }
    lappend leaf_names $leaf
    lappend bit_widths $bits
}

# Sort by leaf name, keeping parallel arrays in sync
set indices {}
for {set i 0} {$i < [llength $leaf_names]} {incr i} {
    lappend indices $i
}
set sorted_indices [lsort -command {apply {{a b} {
    set na [lindex $::leaf_names $a]
    set nb [lindex $::leaf_names $b]
    return [string compare $na $nb]
}}} $indices]

# Convert a value string to unsigned decimal.
# GTKWave may return hex or binary depending on bit width.
# Heuristic: if string length == bit width and all chars are 0/1, it's binary.
proc to_decimal {val bits} {
    if {$bits == 0} {
        # Scalar or real — return as-is
        return $val
    }
    set len [string length $val]
    if {$len == $bits && [regexp {^[01]+$} $val]} {
        # Binary representation
        set result 0
        foreach bit [split $val ""] {
            set result [expr {$result * 2 + $bit}]
        }
        return $result
    }
    # Hex representation
    scan $val %llx result
    return $result
}

# Write CSV
set fp [open $csv_out w]

# Header
puts -nonewline $fp "time"
foreach idx $sorted_indices {
    puts -nonewline $fp ",[lindex $leaf_names $idx]"
}
puts $fp ""

# Data rows: time 1 to max_time
for {set t 1} {$t <= $max_time} {incr t} {
    gtkwave::setMarker $t
    puts -nonewline $fp $t
    foreach idx $sorted_indices {
        set sig [lindex $all_signals $idx]
        set bits [lindex $bit_widths $idx]
        set val [gtkwave::getTraceValueAtMarkerFromName $sig]
        puts -nonewline $fp ",[to_decimal $val $bits]"
    }
    puts $fp ""
}

close $fp
gtkwave::/File/Quit
