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
	die "Invalid line $_";
      }
    }
  }
}

for ($i = 0; $i < 256; $i++) {
  $attr = $opcodes{$i}{'Attr'};

  unless ($attr) {
    print "    NO_ATTRS,\n";
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
      print "    " . join(' | ', @attrList) . ",\n";
    }
    else {
      print "    NO_ATTRS,\n";
    }
  }
}

sub indexUp($) {
  $_ = shift;
  my $part;
  my %ret = { };
  foreach $part (split /,/) {
    $ret{$part} = $part;
  }

  return %ret;
}
