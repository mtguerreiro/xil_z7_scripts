set xsa_out "plat"
set jobs    4

if { $::argc > 0 } {
  for {set i 0} {$i < $::argc} {incr i} {
    set option [string trim [lindex $::argv $i]]
    switch -regexp -- $option {
      "--p"     { incr i; set project [lindex $::argv $i] }
      "--o"     { incr i; set xsa_out [lindex $::argv $i] }
      "--j"     { incr i; set jobs [lindex $::argv $i] }
      default {
        if { [regexp {^-} $option] } {
          puts "ERROR: Unknown option '$option' specified.\n"
          return 1
        }
      }
    }
  }
}

open_project ${project}

puts "\nLaunching synthesis with $jobs jobs\n"
reset_run synth_1
launch_runs synth_1 -jobs ${jobs}
wait_on_run synth_1

puts "\nLaunching implementation\n"
launch_runs impl_1 -to_step write_bitstream -jobs ${jobs}
wait_on_run impl_1

puts "\nWriting platform\n"
write_hw_platform -fixed -include_bit -force ${xsa_out}.xsa

puts "\nXSA exported to $xsa_out.xsa\n"
exit
