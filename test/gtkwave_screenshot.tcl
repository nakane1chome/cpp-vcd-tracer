# GTKWave batch screenshot script: loads VCD, adds all signals, saves screenshot.
# Usage: [xvfb-run] gtkwave input.vcd -S gtkwave_screenshot.tcl
# Set GTKWAVE_PNG_OUT environment variable to specify output path.
# Requires: xwd on PATH, ffmpeg or python3+Pillow for PNG conversion.

set png_out $::env(GTKWAVE_PNG_OUT)
set xwd_tmp "${png_out}.xwd"

# Add all signals
set num_facs [gtkwave::getNumFacs]
set all_signals {}
for {set i 0} {$i < $num_facs} {incr i} {
    lappend all_signals [gtkwave::getFacName $i]
}
gtkwave::addSignalsFromList $all_signals
gtkwave::/Time/Zoom/Zoom_Best_Fit
gtkwave::/Edit/Sort/Alphabetize_All

# Allow rendering to complete, then capture and quit
after 1000 {
    # Capture root window (under xvfb-run this is just the GTKWave window)
    exec xwd -root -screen -out $::xwd_tmp

    # Convert XWD to PNG
    if {![catch {exec ffmpeg -y -i $::xwd_tmp $::png_out} err]} {
        file delete $::xwd_tmp
    } elseif {![catch {exec python3 -c "
from PIL import Image
img = Image.open('$::xwd_tmp')
img.save('$::png_out')
" } err]} {
        file delete $::xwd_tmp
    }
    # If both fail, leave the XWD file

    gtkwave::/File/Quit
}
