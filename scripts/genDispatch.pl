while (<>) {
  if (/^static .+ ([\w\d]+)\(void\)/) {
    $fun = $1;
  }
  elsif (/Opcode: ([\w\d,]+)/) {
    @opcodes = split /,/, $1;
    foreach $opcode (@opcodes) {
      print "        case $opcode:    $fun(); break;\n";
    }
  }
}
