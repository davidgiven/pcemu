%opcodes = { };

while (<>) {
  if (/^static .+ ([\w\d]+)\(void\)/) {
    $fun = $1;
  }
  elsif (/\/\* (\w+): ([\w\d,]+)\s*$/) {
    %vals = { };
    $vals{$1} = $2;
    while (<>) {
      if (/\*\//) {
	foreach $opcode (split /,/, $vals{'Opcode'}) {
	  $opcodes{oct($opcode)} = { %vals };
	}

	last;
      }
      elsif (/(\w+): ([\w\d,]+)/) {
	$vals{$1} = $2;
      }
      else {
	die "$.:Invalid line $_";
      }
    }
  }
}

print "static const UINT32 cpu_attrs[] =\n{\n";

for ($i = 0; $i < 256; $i++) {
  $attr = $opcodes{$i}{'Attr'};

  unless ($attr) {
    print "    NO_ATTRS, /* $i */\n";
  }
  else {
    %attrs = indexUp($attr);
    @attrList = ( );
    if ($attrs{'IsJump'}) {
      push @attrList, "IS_JUMP";
    }
    if ($attrs{'IsShortJump'}) {
      push @attrList, "IS_SHORT_JUMP";
    }
    if ($attrs{'IsPrefix'}) {
      push @attrList, "IS_PREFIX";
    }
    if ($attrs{'HasModRMRMB'}) {
      push @attrList, "HAS_MODRM";
      push @attrList, "HAS_MODRMRMB";
    }
    if ($attrs{'HasModRMRMW'}) {
      push @attrList, "HAS_MODRM";
      push @attrList, "HAS_MODRMRMW";
    }
    if (scalar @attrList) {
      print "    " . join(' | ', @attrList) . ", /* $i */\n";
    }
    else {
      print "    NO_ATTRS, /* $i */\n";
    }
  }
}

print "    0\n};\n\n";

print "static const UINT32 cpu_length[] =\n{\n";

for ($i = 0; $i < 256; $i++) {
  $length = $opcodes{$i}{'Length'};

  unless ($length) {
    print "    0,\n";
  }
  else {
    print "    $length, /* $i */\n";
  }
}

print "    0\n};\n\n";

print "static const UINT16 changes_flags[] =\n{\n";

for ($i = 0; $i < 256; $i++) {
  $flags = $opcodes{$i}{'ChangesFlags'};

  unless ($flags) {
    print "    FLAGS_ALL,\n";
  }
  else {
    @flagList = ( );
    foreach $flag (split /,/, $flags) {
      push @flagList, "FLAG_$flag";
    }
    print "    " . join(' | ', @flagList) . ", /* $i */\n";
  }
}

print "    0\n};\n\n";

print "static const UINT16 uses_flags[] =\n{\n";

for ($i = 0; $i < 256; $i++) {
  $flags = $opcodes{$i}{'UsesFlags'};

  unless ($flags) {
    print "    FLAGS_ALL,\n";
  }
  else {
    @flagList = ( );
    foreach $flag (split /,/, $flags) {
      push @flagList, "FLAG_$flag";
    }
    print "    " . join(' | ', @flagList) . ", /* $i */\n";
  }
}

print "    0\n};\n\n";

sub indexUp($) {
  $_ = shift;
  my $part;
  my %ret = { };
  foreach $part (split /,/) {
    $ret{$part} = $part;
  }

  return %ret;
}
