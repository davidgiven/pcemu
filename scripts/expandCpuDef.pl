use Text::Template;

%macros = { };

while (<>) {
  if (/^MACRO ([\w\d]+)/) {
    $name = $1;
    @lines = undef;

    while (<>) {
      push @lines, $_;
      if (/^\}/) {
	last;
      }
    }
    $macros{$name} = [ @lines ];
  }
  elsif (/^IMPLEMENT\s+([\w\d]+)\s*(.*);/) {
    unless (defined $macros{$1}) {
      die "No macro definition for $1\n";
    }
    @lines = @{ $macros{$1} };

    %args = { };

    foreach $arg (split /,/, $2) {
      $arg =~ /([\w\d]+)=(.+)/;
      $args{$1} = $2;
    }

    $tmpl = Text::Template->new(TYPE => 'ARRAY', SOURCE => \@lines, DELIMITERS => ['#{', '}#']);
    $out = $tmpl->fill_in(HASH => \%args);

    print "$out\n";
  }
  else {
    print "$_";
  }
}
